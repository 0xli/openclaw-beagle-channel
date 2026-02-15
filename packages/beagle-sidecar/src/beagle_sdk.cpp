#include "beagle_sdk.h"

#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <unordered_set>
#include <vector>

#include <string.h>

#if BEAGLE_SDK_STUB

bool BeagleSdk::start(const BeagleSdkOptions& options, BeagleIncomingCallback on_incoming) {
  (void)on_incoming;
  std::cerr << "[beagle-sdk] start stub. data_dir=" << options.data_dir << "\n";
  return true;
}

void BeagleSdk::stop() {
  std::cerr << "[beagle-sdk] stop stub.\n";
}

bool BeagleSdk::send_text(const std::string& peer, const std::string& text) {
  std::cerr << "[beagle-sdk] send_text stub. peer=" << peer << " text=" << text << "\n";
  return true;
}

bool BeagleSdk::send_media(const std::string& peer,
                           const std::string& caption,
                           const std::string& media_path,
                           const std::string& media_url,
                           const std::string& media_type,
                           const std::string& filename) {
  std::cerr << "[beagle-sdk] send_media stub. peer=" << peer
            << " caption=" << caption
            << " media_path=" << media_path
            << " media_url=" << media_url
            << " media_type=" << media_type
            << " filename=" << filename << "\n";
  return true;
}

BeagleStatus BeagleSdk::status() const {
  BeagleStatus s;
  s.ready = true;
  s.connected = true;
  return s;
}

#else

extern "C" {
#include <carrier.h>
#include <carrier_config.h>
#include <carrier_session.h>
#include <carrier_filetransfer.h>
}

namespace {
constexpr size_t kMaxBeaglechatFileBytes = 5 * 1024 * 1024;

static std::string log_ts() {
  std::time_t now = std::time(nullptr);
  std::tm tm_buf{};
  localtime_r(&now, &tm_buf);
  char out[32];
  if (std::strftime(out, sizeof(out), "%Y-%m-%d %H:%M:%S", &tm_buf) == 0) return "";
  return std::string(out);
}

static void log_line(const std::string& msg) {
  std::cerr << "[" << log_ts() << "] " << msg << "\n";
}

struct FriendState {
  std::string friendid;
  std::string name;
  std::string gender;
  std::string phone;
  std::string email;
  std::string description;
  std::string region;
  std::string label;
  int status = 0;
  int presence = 0;
};

struct DbConfig {
  bool enabled = false;
  std::string host = "localhost";
  int port = 3306;
  std::string user = "beagle";
  std::string password = "A1anSn00py";
  std::string database = "beagle";
};

struct ProfileInfo {
  std::string name;
  std::string gender;
  std::string phone;
  std::string email;
  std::string description;
  std::string region;
};

struct RuntimeState {
  Carrier* carrier = nullptr;
  BeagleIncomingCallback on_incoming;
  std::thread loop_thread;
  std::mutex state_mu;
  std::string persistent_location;
  std::string welcome_message;
  std::string profile_path;
  std::string welcome_state_path;
  std::string db_config_path;
  std::string friend_state_path;
  std::string friend_event_log_path;
  std::string media_dir;
  std::string user_id;
  std::string address;
  BeagleStatus status;
  std::unordered_set<std::string> welcomed_peers;
  std::map<std::string, FriendState> friend_state;
  DbConfig db;
};

struct TransferContext {
  RuntimeState* state = nullptr;
  CarrierFileTransfer* ft = nullptr;
  bool is_sender = false;
  bool connected = false;
  bool completed = false;
  std::string peer;
  std::string fileid;
  std::string filename;
  std::string media_type;
  std::string caption;
  std::string source_path;
  std::string target_path;
  uint64_t expected_size = 0;
  uint64_t transferred = 0;
  std::ifstream source;
  std::ofstream target;
};

static std::mutex g_ft_mu;
static std::map<CarrierFileTransfer*, std::shared_ptr<TransferContext>> g_transfers;

static bool file_exists(const std::string& path) {
  std::ifstream f(path);
  return f.good();
}

static void ensure_dir(const std::string& path) {
  if (path.empty()) return;
  struct stat st;
  if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) return;
  mkdir(path.c_str(), 0755);
}

static unsigned long long file_size_bytes(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) return 0;
  if (!S_ISREG(st.st_mode)) return 0;
  return static_cast<unsigned long long>(st.st_size);
}

static std::string basename_of(const std::string& path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) return path;
  return path.substr(pos + 1);
}

static std::string sanitize_filename(const std::string& name) {
  std::string out;
  out.reserve(name.size());
  for (char c : name) {
    if (c == '/' || c == '\\' || c == '\0') {
      out.push_back('_');
    } else {
      out.push_back(c);
    }
  }
  if (out.empty()) return "file.bin";
  return out;
}

static std::string lowercase(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static std::string infer_media_type_from_filename(const std::string& filename) {
  std::string lower = lowercase(filename);
  if (lower.size() >= 4 && lower.rfind(".jpg") == lower.size() - 4) return "image/jpeg";
  if (lower.size() >= 5 && lower.rfind(".jpeg") == lower.size() - 5) return "image/jpeg";
  if (lower.size() >= 4 && lower.rfind(".png") == lower.size() - 4) return "image/png";
  if (lower.size() >= 4 && lower.rfind(".gif") == lower.size() - 4) return "image/gif";
  if (lower.size() >= 5 && lower.rfind(".webp") == lower.size() - 5) return "image/webp";
  if (lower.size() >= 4 && lower.rfind(".mp4") == lower.size() - 4) return "video/mp4";
  if (lower.size() >= 4 && lower.rfind(".mp3") == lower.size() - 4) return "audio/mpeg";
  if (lower.size() >= 4 && lower.rfind(".wav") == lower.size() - 4) return "audio/wav";
  if (lower.size() >= 4 && lower.rfind(".pdf") == lower.size() - 4) return "application/pdf";
  return "application/octet-stream";
}

static std::string json_quote(const std::string& in) {
  std::string out;
  out.reserve(in.size() + 2);
  out.push_back('"');
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out.push_back(c);
        }
    }
  }
  out.push_back('"');
  return out;
}

static bool read_file_binary(const std::string& path, std::vector<unsigned char>& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  in.seekg(0, std::ios::end);
  std::streamoff len = in.tellg();
  if (len <= 0) return false;
  in.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(len));
  in.read(reinterpret_cast<char*>(out.data()), len);
  return static_cast<bool>(in);
}

static bool parse_json_string_field(const std::string& json,
                                    const std::string& key,
                                    std::string& value) {
  std::string marker = "\"" + key + "\"";
  size_t pos = json.find(marker);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos + marker.size());
  if (pos == std::string::npos) return false;
  while (++pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {}
  if (pos >= json.size() || json[pos] != '"') return false;

  std::string out;
  bool escaped = false;
  for (++pos; pos < json.size(); ++pos) {
    char c = json[pos];
    if (escaped) {
      switch (c) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        default: out.push_back(c); break;
      }
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') {
      value = out;
      return true;
    }
    out.push_back(c);
  }
  return false;
}

static bool parse_json_u64_field(const std::string& json,
                                 const std::string& key,
                                 uint64_t& value) {
  std::string marker = "\"" + key + "\"";
  size_t pos = json.find(marker);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos + marker.size());
  if (pos == std::string::npos) return false;
  while (++pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {}
  if (pos >= json.size() || !std::isdigit(static_cast<unsigned char>(json[pos]))) return false;
  uint64_t out = 0;
  for (; pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos])); ++pos) {
    out = out * 10 + static_cast<uint64_t>(json[pos] - '0');
  }
  value = out;
  return true;
}

struct PackedFilePayload {
  std::string filename;
  std::string content_type;
  uint64_t declared_size = 0;
  const unsigned char* bytes = nullptr;
  size_t bytes_len = 0;
};

static int b64_value(unsigned char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

static bool base64_decode(const std::string& in, std::vector<unsigned char>& out) {
  out.clear();
  out.reserve((in.size() * 3) / 4);
  int val = 0;
  int valb = -8;
  for (unsigned char c : in) {
    if (std::isspace(c)) continue;
    if (c == '=') break;
    int d = b64_value(c);
    if (d < 0) return false;
    val = (val << 6) + d;
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return !out.empty();
}

struct InlineJsonMedia {
  std::string filename;
  std::string media_type;
  std::vector<unsigned char> bytes;
};

static std::string trim_data_url_prefix(const std::string& data) {
  size_t comma = data.find(',');
  if (comma == std::string::npos) return data;
  std::string prefix = lowercase(data.substr(0, comma));
  if (prefix.find("base64") != std::string::npos) return data.substr(comma + 1);
  return data;
}

static bool has_file_extension(const std::string& filename) {
  size_t slash = filename.find_last_of("/\\");
  size_t dot = filename.find_last_of('.');
  return dot != std::string::npos && (slash == std::string::npos || dot > slash + 1);
}

static bool decode_inline_json_media_payload(const void* msg, size_t len, InlineJsonMedia& out) {
  if (!msg || len < 8 || len > (kMaxBeaglechatFileBytes * 2)) return false;
  std::string body(static_cast<const char*>(msg), len);
  while (!body.empty() && (body.back() == '\0' || std::isspace(static_cast<unsigned char>(body.back())))) {
    body.pop_back();
  }
  size_t start = 0;
  while (start < body.size() && std::isspace(static_cast<unsigned char>(body[start]))) start++;
  if (start > 0) body.erase(0, start);
  if (body.empty() || body.front() != '{' || body.back() != '}') return false;

  std::string type;
  if (!parse_json_string_field(body, "type", type)) return false;
  type = lowercase(type);
  if (type != "image" && type != "file") return false;

  std::string data;
  if (!parse_json_string_field(body, "data", data) || data.empty()) return false;

  std::string filename;
  parse_json_string_field(body, "fileName", filename);
  if (filename.empty()) parse_json_string_field(body, "filename", filename);
  std::string ext;
  parse_json_string_field(body, "fileExtension", ext);
  if (!ext.empty() && ext[0] != '.') ext = "." + ext;
  if (!filename.empty() && !ext.empty() && !has_file_extension(filename)) filename += ext;
  if (filename.empty()) {
    filename = (type == "image" ? "image" : "file");
    if (!ext.empty()) filename += ext;
  }
  filename = sanitize_filename(filename);

  std::string media_type;
  if (!parse_json_string_field(body, "mediaType", media_type) || media_type.empty()) {
    media_type = infer_media_type_from_filename(filename);
  }

  std::vector<unsigned char> decoded;
  std::string b64 = trim_data_url_prefix(data);
  if (!base64_decode(b64, decoded)) return false;
  if (decoded.size() > kMaxBeaglechatFileBytes) return false;

  out.filename = filename;
  out.media_type = media_type;
  out.bytes.swap(decoded);
  return true;
}

static bool decode_beaglechat_file_payload(const void* msg, size_t len, PackedFilePayload& out) {
  if (!msg || len < 5) return false;
  const unsigned char* p = static_cast<const unsigned char*>(msg);
  uint32_t meta_len = (static_cast<uint32_t>(p[0]) << 24)
                    | (static_cast<uint32_t>(p[1]) << 16)
                    | (static_cast<uint32_t>(p[2]) << 8)
                    | static_cast<uint32_t>(p[3]);
  if (meta_len == 0 || meta_len > 4096) return false;
  if (static_cast<size_t>(meta_len) + 4 > len) return false;

  std::string meta(reinterpret_cast<const char*>(p + 4), meta_len);
  std::string type;
  if (!parse_json_string_field(meta, "type", type) || type != "file") return false;

  std::string filename;
  if (!parse_json_string_field(meta, "filename", filename) || filename.empty()) return false;

  std::string content_type;
  if (!parse_json_string_field(meta, "contentType", content_type) || content_type.empty()) {
    content_type = infer_media_type_from_filename(filename);
  }

  uint64_t declared_size = 0;
  parse_json_u64_field(meta, "size", declared_size);

  out.filename = sanitize_filename(filename);
  out.content_type = content_type;
  out.declared_size = declared_size;
  out.bytes = p + 4 + meta_len;
  out.bytes_len = len - 4 - meta_len;
  return true;
}

static bool encode_beaglechat_file_payload(const std::string& filename,
                                           const std::string& content_type,
                                           const std::vector<unsigned char>& data,
                                           std::vector<unsigned char>& out) {
  std::ostringstream meta;
  meta << "{"
       << "\"type\":\"file\","
       << "\"filename\":" << json_quote(filename) << ","
       << "\"contentType\":" << json_quote(content_type.empty() ? "application/octet-stream" : content_type) << ","
       << "\"size\":" << data.size()
       << "}";
  std::string meta_json = meta.str();
  if (meta_json.empty() || meta_json.size() > 4096) return false;

  uint32_t meta_len = static_cast<uint32_t>(meta_json.size());
  out.resize(4 + meta_len + data.size());
  out[0] = static_cast<unsigned char>((meta_len >> 24) & 0xFF);
  out[1] = static_cast<unsigned char>((meta_len >> 16) & 0xFF);
  out[2] = static_cast<unsigned char>((meta_len >> 8) & 0xFF);
  out[3] = static_cast<unsigned char>(meta_len & 0xFF);
  std::memcpy(out.data() + 4, meta_json.data(), meta_json.size());
  if (!data.empty()) std::memcpy(out.data() + 4 + meta_json.size(), data.data(), data.size());
  return true;
}

static bool read_file(const std::string& path, std::string& out) {
  std::ifstream in(path);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

static bool write_file(const std::string& path, const std::string& data) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) return false;
  out << data;
  return static_cast<bool>(out);
}

static std::string json_escape(const std::string& in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

static void ensure_profile_file(RuntimeState* state);

static bool append_line(const std::string& path, const std::string& line) {
  std::ofstream out(path, std::ios::app);
  if (!out) return false;
  out << line << "\n";
  return static_cast<bool>(out);
}

static bool extract_json_string(const std::string& body, const std::string& key, std::string& out) {
  std::string needle = "\"" + key + "\"";
  size_t pos = body.find(needle);
  if (pos == std::string::npos) return false;
  pos = body.find(':', pos + needle.size());
  if (pos == std::string::npos) return false;
  pos = body.find('"', pos);
  if (pos == std::string::npos) return false;
  size_t end = body.find('"', pos + 1);
  if (end == std::string::npos) return false;
  out = body.substr(pos + 1, end - pos - 1);
  return true;
}

static bool extract_json_int(const std::string& body, const std::string& key, int& out) {
  std::string needle = "\"" + key + "\"";
  size_t pos = body.find(needle);
  if (pos == std::string::npos) return false;
  pos = body.find(':', pos + needle.size());
  if (pos == std::string::npos) return false;
  pos++;
  while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) pos++;
  size_t end = pos;
  while (end < body.size() && (body[end] == '-' || std::isdigit(static_cast<unsigned char>(body[end])))) end++;
  if (end == pos) return false;
  out = std::atoi(body.substr(pos, end - pos).c_str());
  return true;
}

static bool extract_json_bool(const std::string& body, const std::string& key, bool& out) {
  std::string needle = "\"" + key + "\"";
  size_t pos = body.find(needle);
  if (pos == std::string::npos) return false;
  pos = body.find(':', pos + needle.size());
  if (pos == std::string::npos) return false;
  pos++;
  while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) pos++;
  if (body.compare(pos, 4, "true") == 0) {
    out = true;
    return true;
  }
  if (body.compare(pos, 5, "false") == 0) {
    out = false;
    return true;
  }
  return false;
}

static std::string default_profile_json() {
  return std::string("{\n")
      + "  \"welcomeMessage\": \"Hi! I'm the Beagle OpenClaw bot. Send a message to start.\",\n"
      + "  \"profile\": {\n"
      + "    \"name\": \"Snoopy\",\n"
      + "    \"gender\": \"2218\",\n"
      + "    \"phone\": \"Claw Bot to Help\",\n"
      + "    \"email\": \"SOL:,ETH:\",\n"
      + "    \"description\": \"Ask me anything about beagle chat, Tell me who your are\",\n"
      + "    \"region\": \"California\",\n"
      + "    \"carrierUserId\": \"\",\n"
      + "    \"carrierAddress\": \"\",\n"
      + "    \"startedAt\": \"\"\n"
      + "  }\n"
      + "}\n";
}

static std::string iso8601_utc_now() {
  std::time_t now = std::time(nullptr);
  std::tm tm{};
  gmtime_r(&now, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

static bool find_json_object_bounds(const std::string& body,
                                    const std::string& key,
                                    size_t& start,
                                    size_t& end) {
  std::string needle = "\"" + key + "\"";
  size_t key_pos = body.find(needle);
  if (key_pos == std::string::npos) return false;
  size_t brace = body.find('{', key_pos + needle.size());
  if (brace == std::string::npos) return false;
  int depth = 0;
  for (size_t i = brace; i < body.size(); ++i) {
    if (body[i] == '{') depth++;
    else if (body[i] == '}') {
      depth--;
      if (depth == 0) {
        start = brace;
        end = i;
        return true;
      }
    }
  }
  return false;
}

static bool replace_json_string_value(std::string& body,
                                      size_t obj_start,
                                      size_t obj_end,
                                      const std::string& key,
                                      const std::string& value) {
  std::string needle = "\"" + key + "\"";
  size_t pos = body.find(needle, obj_start);
  if (pos == std::string::npos || pos > obj_end) return false;
  pos = body.find(':', pos + needle.size());
  if (pos == std::string::npos || pos > obj_end) return false;
  pos = body.find('"', pos);
  if (pos == std::string::npos || pos > obj_end) return false;
  size_t end = body.find('"', pos + 1);
  if (end == std::string::npos || end > obj_end) return false;
  body.replace(pos + 1, end - pos - 1, json_escape(value));
  return true;
}

static bool insert_json_string_value(std::string& body,
                                     size_t obj_start,
                                     size_t obj_end,
                                     const std::string& key,
                                     const std::string& value) {
  size_t pos = obj_end;
  while (pos > obj_start && std::isspace(static_cast<unsigned char>(body[pos - 1]))) pos--;
  bool needs_comma = (pos > obj_start + 1 && body[pos - 1] != '{');
  std::string insert = (needs_comma ? "," : "");
  insert += "\n    \"" + key + "\": \"" + json_escape(value) + "\"";
  body.insert(obj_end, insert);
  return true;
}

static bool upsert_profile_field(std::string& body,
                                 const std::string& key,
                                 const std::string& value,
                                 bool only_if_missing) {
  if (value.empty()) return false;
  size_t obj_start = 0;
  size_t obj_end = 0;
  if (!find_json_object_bounds(body, "profile", obj_start, obj_end)) return false;
  std::string existing;
  if (extract_json_string(body.substr(obj_start, obj_end - obj_start + 1), key, existing)) {
    if (existing == value) return false;
    if (only_if_missing && !existing.empty()) return false;
    return replace_json_string_value(body, obj_start, obj_end, key, value);
  }
  return insert_json_string_value(body, obj_start, obj_end, key, value);
}

static std::string load_wallet_public_key() {
  const char* home = std::getenv("HOME");
  if (!home) return std::string();
  std::string path = std::string(home) + "/.openclaw/workspace/licode_wallet.json";
  std::string body;
  if (!read_file(path, body)) return std::string();
  std::string pubkey;
  extract_json_string(body, "publicKey", pubkey);
  return pubkey;
}

static void ensure_profile_metadata(RuntimeState* state,
                                    const std::string& user_id,
                                    const std::string& address) {
  if (!state || state->profile_path.empty()) return;
  ensure_profile_file(state);
  std::string body;
  if (!read_file(state->profile_path, body)) return;

  bool changed = false;
  changed |= upsert_profile_field(body, "carrierUserId", user_id, false);
  changed |= upsert_profile_field(body, "carrierAddress", address, false);

  std::string started_at;
  if (!extract_json_string(body, "startedAt", started_at) || started_at.empty()) {
    changed |= upsert_profile_field(body, "startedAt", iso8601_utc_now(), true);
  }

  std::string email;
  extract_json_string(body, "email", email);
  std::string wallet = load_wallet_public_key();
  if (!wallet.empty()) {
    bool placeholder = email.empty()
        || email.find("SOL:") != std::string::npos
        || email.find("ETH:") != std::string::npos;
    if (placeholder && email != wallet) {
      changed |= upsert_profile_field(body, "email", wallet, false);
    }
  }

  if (changed) {
    write_file(state->profile_path, body);
  }
}

static std::string default_db_json() {
  return std::string("{\n")
      + "  \"enabled\": false,\n"
      + "  \"host\": \"localhost\",\n"
      + "  \"port\": 3306,\n"
      + "  \"user\": \"beagle\",\n"
      + "  \"password\": \"A1anSn00py\",\n"
      + "  \"database\": \"beagle\"\n"
      + "}\n";
}

static void ensure_profile_file(RuntimeState* state) {
  if (!state || state->profile_path.empty()) return;
  if (file_exists(state->profile_path)) return;
  if (!write_file(state->profile_path, default_profile_json())) {
    log_line(std::string("[beagle-sdk] failed to write default profile to ") + state->profile_path);
  }
}

static void ensure_db_file(RuntimeState* state) {
  if (!state || state->db_config_path.empty()) return;
  if (file_exists(state->db_config_path)) return;
  if (!write_file(state->db_config_path, default_db_json())) {
    log_line(std::string("[beagle-sdk] failed to write default db config to ") + state->db_config_path);
  }
}

static void load_profile(RuntimeState* state, ProfileInfo& profile) {
  if (!state) return;
  ensure_profile_file(state);
  std::string body;
  if (!read_file(state->profile_path, body)) return;
  extract_json_string(body, "welcomeMessage", state->welcome_message);
  extract_json_string(body, "name", profile.name);
  extract_json_string(body, "gender", profile.gender);
  extract_json_string(body, "phone", profile.phone);
  extract_json_string(body, "email", profile.email);
  extract_json_string(body, "description", profile.description);
  extract_json_string(body, "region", profile.region);
}

static void load_db_config(RuntimeState* state, DbConfig& db) {
  if (!state) return;
  ensure_db_file(state);
  std::string body;
  if (!read_file(state->db_config_path, body)) return;
  extract_json_bool(body, "enabled", db.enabled);
  extract_json_string(body, "host", db.host);
  extract_json_int(body, "port", db.port);
  extract_json_string(body, "user", db.user);
  extract_json_string(body, "password", db.password);
  extract_json_string(body, "database", db.database);
}

static void load_welcomed_peers(RuntimeState* state) {
  if (!state || state->welcome_state_path.empty()) return;
  std::ifstream in(state->welcome_state_path);
  if (!in) return;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) state->welcomed_peers.insert(line);
  }
}

static std::string sanitize_tsv(const std::string& in) {
  std::string out = in;
  for (char& c : out) {
    if (c == '\t' || c == '\n' || c == '\r') c = ' ';
  }
  return out;
}

static void load_friend_state(RuntimeState* state) {
  if (!state || state->friend_state_path.empty()) return;
  std::ifstream in(state->friend_state_path);
  if (!in) return;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
      if (c == '\t') {
        fields.push_back(current);
        current.clear();
      } else {
        current.push_back(c);
      }
    }
    fields.push_back(current);
    if (fields.size() < 10) continue;
    FriendState fs;
    fs.friendid = fields[0];
    fs.name = fields[1];
    fs.gender = fields[2];
    fs.phone = fields[3];
    fs.email = fields[4];
    fs.description = fields[5];
    fs.region = fields[6];
    fs.label = fields[7];
    fs.status = std::atoi(fields[8].c_str());
    fs.presence = std::atoi(fields[9].c_str());
    if (!fs.friendid.empty()) state->friend_state[fs.friendid] = std::move(fs);
  }
}

static void save_friend_state(RuntimeState* state) {
  if (!state || state->friend_state_path.empty()) return;
  std::ostringstream out;
  for (const auto& kv : state->friend_state) {
    const FriendState& fs = kv.second;
    out << sanitize_tsv(fs.friendid) << "\t"
        << sanitize_tsv(fs.name) << "\t"
        << sanitize_tsv(fs.gender) << "\t"
        << sanitize_tsv(fs.phone) << "\t"
        << sanitize_tsv(fs.email) << "\t"
        << sanitize_tsv(fs.description) << "\t"
        << sanitize_tsv(fs.region) << "\t"
        << sanitize_tsv(fs.label) << "\t"
        << fs.status << "\t"
        << fs.presence << "\n";
  }
  write_file(state->friend_state_path, out.str());
}

static std::string sql_escape(const std::string& in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    if (c == '\\' || c == '\'') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

static std::string shell_escape(const std::string& in) {
  std::string out = "'";
  for (char c : in) {
    if (c == '\'') {
      out += "'\"'\"'";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

static int mysql_exec(const DbConfig& db, const std::string& sql) {
  if (!db.enabled) return 0;
  std::ostringstream cmd;
  cmd << "mysql --protocol=TCP"
      << " --host=" << shell_escape(db.host)
      << " --port=" << db.port
      << " --user=" << shell_escape(db.user)
      << " --password=" << shell_escape(db.password);
  if (!db.database.empty()) cmd << " --database=" << shell_escape(db.database);
  cmd << " --execute=" << shell_escape(sql);
  int rc = std::system(cmd.str().c_str());
  return rc;
}

static void ensure_db(RuntimeState* state, const DbConfig& db) {
  if (!state || !db.enabled) return;
  std::string create_db = "CREATE DATABASE IF NOT EXISTS " + db.database + ";";
  {
    std::ostringstream cmd;
    cmd << "mysql --protocol=TCP"
        << " --host=" << shell_escape(db.host)
        << " --port=" << db.port
        << " --user=" << shell_escape(db.user)
        << " --password=" << shell_escape(db.password)
        << " --execute=" << shell_escape(create_db);
    std::system(cmd.str().c_str());
  }

  const char* schema =
      "CREATE TABLE IF NOT EXISTS beagle_friend_info ("
      "friendid VARCHAR(128) PRIMARY KEY,"
      "name VARCHAR(128),"
      "gender VARCHAR(64),"
      "phone VARCHAR(64),"
      "email VARCHAR(256),"
      "description TEXT,"
      "region VARCHAR(128),"
      "label VARCHAR(128),"
      "status INT,"
      "presence INT,"
      "updated_at DATETIME"
      ");"
      "CREATE TABLE IF NOT EXISTS beagle_friend_info_history ("
      "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
      "friendid VARCHAR(128),"
      "name VARCHAR(128),"
      "gender VARCHAR(64),"
      "phone VARCHAR(64),"
      "email VARCHAR(256),"
      "description TEXT,"
      "region VARCHAR(128),"
      "label VARCHAR(128),"
      "status INT,"
      "presence INT,"
      "changed_at DATETIME"
      ");"
      "CREATE TABLE IF NOT EXISTS beagle_friend_events ("
      "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
      "friendid VARCHAR(128),"
      "event_type VARCHAR(32),"
      "status INT,"
      "presence INT,"
      "ts DATETIME"
      ");";
  int rc = mysql_exec(db, schema);
  if (rc != 0) {
    log_line(std::string("[beagle-sdk] mysql schema init failed rc=") + std::to_string(rc));
  }
}

static std::string now_mysql_ts() {
  std::time_t now = std::time(nullptr);
  std::tm tm_buf{};
  localtime_r(&now, &tm_buf);
  char out[32];
  if (std::strftime(out, sizeof(out), "%Y-%m-%d %H:%M:%S", &tm_buf) == 0) return "";
  return std::string(out);
}

static void log_friend_event(RuntimeState* state,
                             const std::string& friendid,
                             const std::string& event_type,
                             int status,
                             int presence) {
  std::string ts = now_mysql_ts();
  if (!state->friend_event_log_path.empty()) {
    std::ostringstream line;
    line << ts << "\t" << friendid << "\t" << event_type
         << "\tstatus=" << status << "\tpresence=" << presence;
    append_line(state->friend_event_log_path, line.str());
  }
  if (state->db.enabled) {
    std::ostringstream sql;
    sql << "INSERT INTO beagle_friend_events(friendid,event_type,status,presence,ts) VALUES('"
        << sql_escape(friendid) << "','"
        << sql_escape(event_type) << "',"
        << status << ","
        << presence << ",'"
        << sql_escape(ts) << "');";
    mysql_exec(state->db, sql.str());
  }
}

static bool friend_info_equals(const FriendState& fs, const CarrierFriendInfo* info) {
  if (!info) return true;
  const CarrierUserInfo& ui = info->user_info;
  if (fs.name != ui.name) return false;
  if (fs.gender != ui.gender) return false;
  if (fs.phone != ui.phone) return false;
  if (fs.email != ui.email) return false;
  if (fs.description != ui.description) return false;
  if (fs.region != ui.region) return false;
  if (fs.label != info->label) return false;
  if (fs.status != static_cast<int>(info->status)) return false;
  if (fs.presence != static_cast<int>(info->presence)) return false;
  return true;
}

static FriendState from_friend_info(const std::string& friendid, const CarrierFriendInfo* info) {
  FriendState fs;
  fs.friendid = friendid;
  if (!info) return fs;
  const CarrierUserInfo& ui = info->user_info;
  fs.name = ui.name;
  fs.gender = ui.gender;
  fs.phone = ui.phone;
  fs.email = ui.email;
  fs.description = ui.description;
  fs.region = ui.region;
  fs.label = info->label;
  fs.status = static_cast<int>(info->status);
  fs.presence = static_cast<int>(info->presence);
  return fs;
}

static void store_friend_info(RuntimeState* state,
                              const std::string& friendid,
                              const CarrierFriendInfo* info) {
  if (!state || friendid.empty() || !info) return;
  std::lock_guard<std::mutex> lock(state->state_mu);
  auto it = state->friend_state.find(friendid);
  FriendState next = from_friend_info(friendid, info);
  if (it != state->friend_state.end() && friend_info_equals(it->second, info)) return;
  state->friend_state[friendid] = next;
  save_friend_state(state);

  if (state->db.enabled) {
    std::string ts = now_mysql_ts();
    const CarrierUserInfo& ui = info->user_info;
    std::ostringstream upsert;
    upsert << "REPLACE INTO beagle_friend_info(friendid,name,gender,phone,email,description,region,label,status,presence,updated_at) VALUES('"
           << sql_escape(friendid) << "','"
           << sql_escape(ui.name) << "','"
           << sql_escape(ui.gender) << "','"
           << sql_escape(ui.phone) << "','"
           << sql_escape(ui.email) << "','"
           << sql_escape(ui.description) << "','"
           << sql_escape(ui.region) << "','"
           << sql_escape(info->label) << "',"
           << static_cast<int>(info->status) << ","
           << static_cast<int>(info->presence) << ",'"
           << sql_escape(ts) << "');";
    mysql_exec(state->db, upsert.str());

    std::ostringstream history;
    history << "INSERT INTO beagle_friend_info_history(friendid,name,gender,phone,email,description,region,label,status,presence,changed_at) VALUES('"
            << sql_escape(friendid) << "','"
            << sql_escape(ui.name) << "','"
            << sql_escape(ui.gender) << "','"
            << sql_escape(ui.phone) << "','"
            << sql_escape(ui.email) << "','"
            << sql_escape(ui.description) << "','"
            << sql_escape(ui.region) << "','"
            << sql_escape(info->label) << "',"
            << static_cast<int>(info->status) << ","
            << static_cast<int>(info->presence) << ",'"
            << sql_escape(ts) << "');";
    mysql_exec(state->db, history.str());
  }
}

static void update_friend_status(RuntimeState* state,
                                 const std::string& friendid,
                                 int status,
                                 int presence,
                                 bool log_event) {
  if (!state || friendid.empty()) return;
  std::lock_guard<std::mutex> lock(state->state_mu);
  auto it = state->friend_state.find(friendid);
  if (it == state->friend_state.end()) {
    FriendState fs;
    fs.friendid = friendid;
    fs.status = status >= 0 ? status : 0;
    fs.presence = presence >= 0 ? presence : 0;
    state->friend_state[friendid] = fs;
    save_friend_state(state);
    if (log_event) log_friend_event(state, friendid, status ? "online" : "offline", status, presence);
    return;
  }
  int next_status = status >= 0 ? status : it->second.status;
  int next_presence = presence >= 0 ? presence : it->second.presence;
  bool changed = (it->second.status != next_status);
  bool presence_changed = (it->second.presence != next_presence);
  if (!changed && !presence_changed) return;
  it->second.status = next_status;
  it->second.presence = next_presence;
  save_friend_state(state);
  if (log_event && changed) log_friend_event(state, friendid, next_status ? "online" : "offline", next_status, next_presence);
}

static void apply_profile(RuntimeState* state, const ProfileInfo& profile) {
  if (!state || !state->carrier) return;
  CarrierUserInfo info;
  if (carrier_get_self_info(state->carrier, &info) < 0) {
    std::memset(&info, 0, sizeof(info));
  }
  if (!profile.name.empty()) std::strncpy(info.name, profile.name.c_str(), sizeof(info.name) - 1);
  if (!profile.gender.empty()) std::strncpy(info.gender, profile.gender.c_str(), sizeof(info.gender) - 1);
  if (!profile.phone.empty()) std::strncpy(info.phone, profile.phone.c_str(), sizeof(info.phone) - 1);
  if (!profile.email.empty()) std::strncpy(info.email, profile.email.c_str(), sizeof(info.email) - 1);
  if (!profile.description.empty()) std::strncpy(info.description, profile.description.c_str(), sizeof(info.description) - 1);
  if (!profile.region.empty()) std::strncpy(info.region, profile.region.c_str(), sizeof(info.region) - 1);

  int rc = carrier_set_self_info(state->carrier, &info);
  if (rc < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] set self info failed: 0x" << std::hex << carrier_get_error() << std::dec;
    log_line(msg.str());
  } else {
    log_line("[beagle-sdk] self info updated");
  }
}

static void send_welcome_once(RuntimeState* state, const std::string& peer, const char* reason) {
  if (!state || peer.empty() || !state->carrier) return;
  {
    std::lock_guard<std::mutex> lock(state->state_mu);
    if (state->welcomed_peers.find(peer) != state->welcomed_peers.end()) return;
  }

  const std::string msg = state->welcome_message.empty()
      ? "Hi! I'm the Beagle OpenClaw bot. Send a message to start."
      : state->welcome_message;
  uint32_t msgid = 0;
  int rc = carrier_send_friend_message(state->carrier,
                                       peer.c_str(),
                                       msg.data(),
                                       msg.size(),
                                       &msgid,
                                       nullptr,
                                       nullptr);
  if (rc < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] welcome message failed (" << reason
        << "): 0x" << std::hex << carrier_get_error() << std::dec;
    log_line(msg.str());
  } else {
    {
      std::lock_guard<std::mutex> lock(state->state_mu);
      state->welcomed_peers.insert(peer);
    }
    if (!state->welcome_state_path.empty()) {
      append_line(state->welcome_state_path, peer);
    }
    log_line(std::string("[beagle-sdk] welcome message sent (") + reason + ") to " + peer);
  }
}

static void register_transfer(const std::shared_ptr<TransferContext>& ctx) {
  if (!ctx || !ctx->ft) return;
  std::lock_guard<std::mutex> lock(g_ft_mu);
  g_transfers[ctx->ft] = ctx;
}

static std::shared_ptr<TransferContext> get_transfer(CarrierFileTransfer* ft) {
  if (!ft) return nullptr;
  std::lock_guard<std::mutex> lock(g_ft_mu);
  auto it = g_transfers.find(ft);
  if (it == g_transfers.end()) return nullptr;
  return it->second;
}

static std::shared_ptr<TransferContext> take_transfer(CarrierFileTransfer* ft) {
  if (!ft) return nullptr;
  std::lock_guard<std::mutex> lock(g_ft_mu);
  auto it = g_transfers.find(ft);
  if (it == g_transfers.end()) return nullptr;
  auto ctx = it->second;
  g_transfers.erase(it);
  return ctx;
}

static void emit_incoming_file_event(const std::shared_ptr<TransferContext>& ctx) {
  if (!ctx || !ctx->state || !ctx->state->on_incoming) return;
  BeagleIncomingMessage incoming;
  incoming.peer = ctx->peer;
  incoming.text = "";
  incoming.media_path = ctx->target_path;
  incoming.media_type = ctx->media_type;
  incoming.filename = ctx->filename;
  incoming.size = static_cast<unsigned long long>(ctx->transferred);
  incoming.ts = static_cast<long long>(std::time(nullptr));
  ctx->state->on_incoming(incoming);

  std::ostringstream line;
  line << "[beagle-sdk] received file from " << ctx->peer
       << " file=" << ctx->filename
       << " size=" << ctx->transferred
       << " path=" << ctx->target_path;
  log_line(line.str());
}

static void filetransfer_state_changed_callback(CarrierFileTransfer* ft,
                                                FileTransferConnection state,
                                                void* context) {
  (void)context;
  auto ctx = get_transfer(ft);
  if (!ctx) return;
  ctx->connected = (state == FileTransferConnection_connected);

  if (state == FileTransferConnection_connected && !ctx->is_sender && !ctx->fileid.empty()) {
    carrier_filetransfer_pull(ft, ctx->fileid.c_str(), 0);
  }

  if (state == FileTransferConnection_closed || state == FileTransferConnection_failed) {
    auto done = take_transfer(ft);
    if (!done) return;
    if (done->source.is_open()) done->source.close();
    if (done->target.is_open()) done->target.close();
    carrier_filetransfer_close(ft);
  }
}

static void filetransfer_file_callback(CarrierFileTransfer* ft,
                                       const char* fileid,
                                       const char* filename,
                                       uint64_t size,
                                       void* context) {
  (void)context;
  auto ctx = get_transfer(ft);
  if (!ctx) return;
  if (fileid && *fileid) ctx->fileid = fileid;
  if (filename && *filename) ctx->filename = sanitize_filename(filename);
  if (size > 0) ctx->expected_size = size;
  if (ctx->media_type.empty()) ctx->media_type = infer_media_type_from_filename(ctx->filename);
  if (!ctx->is_sender && ctx->connected && !ctx->fileid.empty()) {
    carrier_filetransfer_pull(ft, ctx->fileid.c_str(), 0);
  }
}

static void filetransfer_pull_callback(CarrierFileTransfer* ft,
                                       const char* fileid,
                                       uint64_t offset,
                                       void* context) {
  (void)context;
  auto ctx = get_transfer(ft);
  if (!ctx || !ctx->is_sender) return;
  if (!fileid || ctx->fileid != fileid) return;

  if (!ctx->source.is_open()) {
    ctx->source.open(ctx->source_path, std::ios::binary);
    if (!ctx->source) {
      carrier_filetransfer_cancel(ft, ctx->fileid.c_str(), -1, "open source failed");
      return;
    }
  }
  ctx->source.clear();
  ctx->source.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!ctx->source.good()) {
    carrier_filetransfer_cancel(ft, ctx->fileid.c_str(), -1, "seek source failed");
    return;
  }

  char buf[64 * 1024];
  while (ctx->source.good()) {
    ctx->source.read(buf, sizeof(buf));
    std::streamsize got = ctx->source.gcount();
    if (got <= 0) break;
    ssize_t sent = carrier_filetransfer_send(ft,
                                             ctx->fileid.c_str(),
                                             reinterpret_cast<const uint8_t*>(buf),
                                             static_cast<size_t>(got));
    if (sent < 0) {
      carrier_filetransfer_cancel(ft, ctx->fileid.c_str(), -1, "send chunk failed");
      return;
    }
    ctx->transferred += static_cast<uint64_t>(sent);
  }

  carrier_filetransfer_send(ft, ctx->fileid.c_str(), nullptr, 0);
}

static bool filetransfer_data_callback(CarrierFileTransfer* ft,
                                       const char* fileid,
                                       const uint8_t* data,
                                       size_t length,
                                       void* context) {
  (void)context;
  auto ctx = get_transfer(ft);
  if (!ctx || ctx->is_sender) return false;
  if (!fileid || ctx->fileid != fileid) return false;
  if (!ctx->target.is_open()) return false;

  if (length == 0) {
    ctx->target.flush();
    ctx->target.close();
    ctx->completed = true;
    emit_incoming_file_event(ctx);
    return false;
  }

  ctx->target.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(length));
  if (!ctx->target.good()) return false;
  ctx->transferred += static_cast<uint64_t>(length);
  return true;
}

static void filetransfer_pending_callback(CarrierFileTransfer* ft,
                                          const char* fileid,
                                          void* context) {
  (void)ft;
  (void)fileid;
  (void)context;
}

static void filetransfer_resume_callback(CarrierFileTransfer* ft,
                                         const char* fileid,
                                         void* context) {
  (void)ft;
  (void)fileid;
  (void)context;
}

static void filetransfer_cancel_callback(CarrierFileTransfer* ft,
                                         const char* fileid,
                                         int status,
                                         const char* reason,
                                         void* context) {
  (void)fileid;
  (void)context;
  std::ostringstream msg;
  msg << "[beagle-sdk] file transfer canceled status=" << status
      << " reason=" << (reason ? reason : "");
  log_line(msg.str());
  auto ctx = take_transfer(ft);
  if (!ctx) return;
  if (ctx->source.is_open()) ctx->source.close();
  if (ctx->target.is_open()) ctx->target.close();
  carrier_filetransfer_close(ft);
}

static CarrierFileTransferCallbacks build_filetransfer_callbacks() {
  CarrierFileTransferCallbacks cbs;
  std::memset(&cbs, 0, sizeof(cbs));
  cbs.state_changed = filetransfer_state_changed_callback;
  cbs.file = filetransfer_file_callback;
  cbs.pull = filetransfer_pull_callback;
  cbs.data = filetransfer_data_callback;
  cbs.pending = filetransfer_pending_callback;
  cbs.resume = filetransfer_resume_callback;
  cbs.cancel = filetransfer_cancel_callback;
  return cbs;
}

static void filetransfer_connect_callback(Carrier* carrier,
                                          const char* address,
                                          const CarrierFileTransferInfo* fileinfo,
                                          void* context) {
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !carrier || !address || !fileinfo) return;

  auto ctx = std::make_shared<TransferContext>();
  ctx->state = state;
  ctx->is_sender = false;
  ctx->peer = address;
  ctx->fileid = fileinfo->fileid;
  ctx->filename = sanitize_filename(fileinfo->filename);
  ctx->expected_size = fileinfo->size;
  ctx->media_type = infer_media_type_from_filename(ctx->filename);

  ensure_dir(state->media_dir);
  std::ostringstream path;
  path << state->media_dir << "/" << std::time(nullptr) << "_" << ctx->filename;
  ctx->target_path = path.str();
  ctx->target.open(ctx->target_path, std::ios::binary | std::ios::trunc);
  if (!ctx->target) {
    log_line(std::string("[beagle-sdk] failed to open target file: ") + ctx->target_path);
    return;
  }

  CarrierFileTransferCallbacks cbs = build_filetransfer_callbacks();
  ctx->ft = carrier_filetransfer_new(carrier, address, fileinfo, &cbs, ctx.get());
  if (!ctx->ft) {
    std::ostringstream msg;
    msg << "[beagle-sdk] carrier_filetransfer_new(receiver) failed: 0x" << std::hex
        << carrier_get_error() << std::dec;
    log_line(msg.str());
    ctx->target.close();
    return;
  }
  register_transfer(ctx);
  if (carrier_filetransfer_accept_connect(ctx->ft) < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] carrier_filetransfer_accept_connect failed: 0x" << std::hex
        << carrier_get_error() << std::dec;
    log_line(msg.str());
    take_transfer(ctx->ft);
    carrier_filetransfer_close(ctx->ft);
    ctx->target.close();
  }
}

bool friend_list_callback(const CarrierFriendInfo* info, void* context);

void friend_message_callback(Carrier* carrier,
                             const char* from,
                             const void* msg,
                             size_t len,
                             int64_t timestamp,
                             bool offline,
                             void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !state->on_incoming) return;

  BeagleIncomingMessage incoming;
  incoming.peer = from ? from : "";
  PackedFilePayload file_payload;
  if (decode_beaglechat_file_payload(msg, len, file_payload)) {
    if (file_payload.bytes_len > kMaxBeaglechatFileBytes) {
      incoming.filename = file_payload.filename;
      incoming.media_type = file_payload.content_type;
      incoming.size = static_cast<unsigned long long>(file_payload.bytes_len);
      incoming.text = "[file rejected: exceeds 5MB beaglechat payload limit]";
      log_line(std::string("[beagle-sdk] rejected incoming beaglechat file from ")
               + incoming.peer + " file=" + incoming.filename
               + " size=" + std::to_string(file_payload.bytes_len));
    } else {
    ensure_dir(state->media_dir);
    std::ostringstream path;
    path << state->media_dir << "/" << std::time(nullptr) << "_" << file_payload.filename;
    std::ofstream out(path.str(), std::ios::binary | std::ios::trunc);
    if (out) {
      out.write(reinterpret_cast<const char*>(file_payload.bytes),
                static_cast<std::streamsize>(file_payload.bytes_len));
      out.close();
      incoming.media_path = path.str();
      incoming.filename = file_payload.filename;
      incoming.media_type = file_payload.content_type;
      incoming.size = static_cast<unsigned long long>(file_payload.bytes_len);
      incoming.text.clear();
      log_line(std::string("[beagle-sdk] received beaglechat file from ")
               + incoming.peer + " file=" + incoming.filename
               + " size=" + std::to_string(file_payload.bytes_len));
    } else {
      log_line(std::string("[beagle-sdk] failed to persist incoming file from ")
               + incoming.peer + " file=" + file_payload.filename);
      incoming.text.assign(static_cast<const char*>(msg), len);
    }
    }
  } else {
    InlineJsonMedia inline_media;
    if (decode_inline_json_media_payload(msg, len, inline_media)) {
      ensure_dir(state->media_dir);
      std::ostringstream path;
      path << state->media_dir << "/" << std::time(nullptr) << "_" << inline_media.filename;
      std::ofstream out(path.str(), std::ios::binary | std::ios::trunc);
      if (out) {
        out.write(reinterpret_cast<const char*>(inline_media.bytes.data()),
                  static_cast<std::streamsize>(inline_media.bytes.size()));
        out.close();
        incoming.media_path = path.str();
        incoming.filename = inline_media.filename;
        incoming.media_type = inline_media.media_type;
        incoming.size = static_cast<unsigned long long>(inline_media.bytes.size());
        incoming.text.clear();
        log_line(std::string("[beagle-sdk] received inline json media from ")
                 + incoming.peer + " file=" + incoming.filename
                 + " size=" + std::to_string(inline_media.bytes.size()));
      } else {
        log_line(std::string("[beagle-sdk] failed to persist inline json media from ")
                 + incoming.peer + " file=" + inline_media.filename);
        incoming.text.assign(static_cast<const char*>(msg), len);
      }
    } else {
      if (len > 1024) {
        log_line(std::string("[beagle-sdk] inline json media decode miss peer=") + incoming.peer
                 + " len=" + std::to_string(len));
      }
      incoming.text.assign(static_cast<const char*>(msg), len);
    }
  }
  incoming.ts = timestamp;
  state->on_incoming(incoming);

  {
    std::lock_guard<std::mutex> lock(state->state_mu);
    state->status.last_peer = incoming.peer;
    if (offline) {
      state->status.offline_count++;
      state->status.last_offline_ts = timestamp;
    } else {
      state->status.online_count++;
      state->status.last_online_ts = timestamp;
    }
  }

  std::ostringstream line;
  line << "[beagle-sdk] message (" << (offline ? "offline" : "online")
       << ") from " << incoming.peer;
  if (!incoming.media_path.empty()) {
    line << " [file] " << incoming.filename << " (" << incoming.size << " bytes)";
  } else {
    line << ": " << incoming.text;
  }
  log_line(line.str());
}

void friend_request_callback(Carrier* carrier,
                             const char* userid,
                             const CarrierUserInfo* info,
                             const char* hello,
                             void* context) {
  (void)info;
  (void)hello;
  auto* state = static_cast<RuntimeState*>(context);
  if (!carrier || !userid) return;
  int rc = carrier_accept_friend(carrier, userid);
  if (rc < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] accept friend failed: 0x" << std::hex << carrier_get_error() << std::dec;
    log_line(msg.str());
  } else {
    log_line(std::string("[beagle-sdk] accepted friend: ") + userid);
    send_welcome_once(state, userid, "accepted");
  }
}

void connection_status_callback(Carrier* carrier,
                                CarrierConnectionStatus status,
                                void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (state) {
    std::lock_guard<std::mutex> lock(state->state_mu);
    state->status.connected = (status == CarrierConnectionStatus_Connected);
  }
  log_line(std::string("[beagle-sdk] connection status: ")
           + (status == CarrierConnectionStatus_Connected ? "connected" : "disconnected"));
}

void ready_callback(Carrier* carrier, void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (state) {
    std::lock_guard<std::mutex> lock(state->state_mu);
    state->status.ready = true;
  }
  log_line("[beagle-sdk] ready");
  if (carrier && state) {
    int rc = carrier_get_friends(carrier, friend_list_callback, state);
    if (rc < 0) {
      std::ostringstream msg;
      msg << "[beagle-sdk] carrier_get_friends failed: 0x" << std::hex << carrier_get_error() << std::dec;
      log_line(msg.str());
    } else {
      log_line("[beagle-sdk] carrier_get_friends ok");
    }
  }
}

void friend_connection_callback(Carrier* carrier,
                                const char* friendid,
                                CarrierConnectionStatus status,
                                void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  log_line(std::string("[beagle-sdk] friend ") + (friendid ? friendid : "")
           + " is " + (status == CarrierConnectionStatus_Connected ? "online" : "offline"));
  if (status == CarrierConnectionStatus_Connected && friendid) {
    send_welcome_once(state, friendid, "online");
  }
  if (friendid) {
    update_friend_status(state,
                         friendid,
                         status == CarrierConnectionStatus_Connected ? 1 : 0,
                         -1,
                         true);
  }
}

void friend_info_callback(Carrier* carrier,
                          const char* friendid,
                          const CarrierFriendInfo* info,
                          void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !friendid || !info) return;
  log_line(std::string("[beagle-sdk] friend info update for ") + friendid);
  store_friend_info(state, friendid, info);
}

void friend_added_callback(Carrier* carrier,
                           const CarrierFriendInfo* info,
                           void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !info) return;
  log_line(std::string("[beagle-sdk] friend added ") + info->user_info.userid);
  store_friend_info(state, info->user_info.userid, info);
}

void friend_presence_callback(Carrier* carrier,
                              const char* friendid,
                              CarrierPresenceStatus presence,
                              void* context) {
  (void)carrier;
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !friendid) return;
  update_friend_status(state, friendid, -1, static_cast<int>(presence), false);
}

bool friend_list_callback(const CarrierFriendInfo* info, void* context) {
  auto* state = static_cast<RuntimeState*>(context);
  if (!state || !info) return false;
  const char* fid = info->user_info.userid;
  if (!fid || !*fid) return true;
  log_line(std::string("[beagle-sdk] friend list item ") + fid);
  store_friend_info(state, fid, info);
  return true;
}

void friend_invite_callback(Carrier* carrier,
                            const char* from,
                            const char* bundle,
                            const void* data,
                            size_t len,
                            void* context) {
  (void)carrier;
  (void)bundle;
  auto* state = static_cast<RuntimeState*>(context);
  std::string payload;
  if (data && len) payload.assign(static_cast<const char*>(data), len);
  log_line(std::string("[beagle-sdk] invite from ") + (from ? from : "") + " data=" + payload);

  if (!state || !state->on_incoming || !from) return;
  BeagleIncomingMessage incoming;
  incoming.peer = from;
  incoming.text = payload.empty() ? "[invite]" : payload;
  incoming.ts = static_cast<long long>(std::time(nullptr));
  state->on_incoming(incoming);

  {
    std::lock_guard<std::mutex> lock(state->state_mu);
    state->status.last_peer = incoming.peer;
    state->status.online_count++;
    state->status.last_online_ts = incoming.ts;
  }
}
} // namespace

static RuntimeState g_state;

bool BeagleSdk::start(const BeagleSdkOptions& options, BeagleIncomingCallback on_incoming) {
  if (options.config_path.empty()) {
    log_line("[beagle-sdk] missing config file path");
    return false;
  }

  CarrierOptions opts;
  if (!carrier_config_load(options.config_path.c_str(), nullptr, &opts)) {
    log_line(std::string("[beagle-sdk] failed to load config: ") + options.config_path);
    return false;
  }

  if (!options.data_dir.empty()) {
    g_state.persistent_location = options.data_dir;
    ensure_dir(g_state.persistent_location);
    // carrier_config_free() will free this field, so allocate with strdup.
    opts.persistent_location = strdup(g_state.persistent_location.c_str());
    g_state.profile_path = g_state.persistent_location + "/beagle_profile.json";
    g_state.welcome_state_path = g_state.persistent_location + "/welcomed_peers.txt";
    g_state.db_config_path = g_state.persistent_location + "/beagle_db.json";
    g_state.friend_state_path = g_state.persistent_location + "/friend_state.tsv";
    g_state.friend_event_log_path = g_state.persistent_location + "/friend_events.log";
    g_state.media_dir = g_state.persistent_location + "/media";
    ensure_dir(g_state.media_dir);
  } else {
    g_state.media_dir = "./media";
    ensure_dir(g_state.media_dir);
  }

  CarrierCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.connection_status = connection_status_callback;
  callbacks.ready = ready_callback;
  callbacks.self_info = nullptr;
  callbacks.friend_connection = friend_connection_callback;
  callbacks.friend_info = friend_info_callback;
  callbacks.friend_presence = friend_presence_callback;
  callbacks.friend_message = friend_message_callback;
  callbacks.friend_request = friend_request_callback;
  callbacks.friend_added = friend_added_callback;
  callbacks.friend_invite = friend_invite_callback;

  g_state.on_incoming = std::move(on_incoming);

  Carrier* carrier = carrier_new(&opts, &callbacks, &g_state);
  carrier_config_free(&opts);
  if (!carrier) {
    std::ostringstream msg;
    msg << "[beagle-sdk] carrier_new failed: 0x" << std::hex << carrier_get_error() << std::dec;
    log_line(msg.str());
    return false;
  }

  g_state.carrier = carrier;

  if (carrier_filetransfer_init(g_state.carrier, filetransfer_connect_callback, &g_state) < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] carrier_filetransfer_init failed: 0x" << std::hex
        << carrier_get_error() << std::dec;
    log_line(msg.str());
  }

  char buf[CARRIER_MAX_ADDRESS_LEN + 1] = {0};
  char idbuf[CARRIER_MAX_ID_LEN + 1] = {0};
  carrier_get_userid(carrier, idbuf, sizeof(idbuf));
  carrier_get_address(carrier, buf, sizeof(buf));
  g_state.user_id = idbuf;
  g_state.address = buf;
  user_id_ = g_state.user_id;
  address_ = g_state.address;

  log_line(std::string("[beagle-sdk] User ID: ") + user_id_);
  log_line(std::string("[beagle-sdk] Address: ") + address_);

  ensure_profile_metadata(&g_state, g_state.user_id, g_state.address);

  ProfileInfo profile;
  load_profile(&g_state, profile);
  load_welcomed_peers(&g_state);
  apply_profile(&g_state, profile);

  load_db_config(&g_state, g_state.db);
  ensure_db(&g_state, g_state.db);
  load_friend_state(&g_state);

  g_state.loop_thread = std::thread([]() {
    int rc = carrier_run(g_state.carrier, 10);
    if (rc != 0) {
      std::ostringstream msg;
      msg << "[beagle-sdk] carrier_run failed: 0x" << std::hex << carrier_get_error() << std::dec;
      log_line(msg.str());
    }
  });

  return true;
}

void BeagleSdk::stop() {
  if (!g_state.carrier) return;
  carrier_filetransfer_cleanup(g_state.carrier);
  carrier_kill(g_state.carrier);
  if (g_state.loop_thread.joinable()) g_state.loop_thread.join();
  {
    std::lock_guard<std::mutex> lock(g_ft_mu);
    for (auto& it : g_transfers) {
      if (it.second && it.second->source.is_open()) it.second->source.close();
      if (it.second && it.second->target.is_open()) it.second->target.close();
      if (it.first) carrier_filetransfer_close(it.first);
    }
    g_transfers.clear();
  }
  g_state.carrier = nullptr;
}

bool BeagleSdk::send_text(const std::string& peer, const std::string& text) {
  if (!g_state.carrier) return false;
  uint32_t msgid = 0;
  int rc = carrier_send_friend_message(g_state.carrier,
                                       peer.c_str(),
                                       text.data(),
                                       text.size(),
                                       &msgid,
                                       nullptr,
                                       nullptr);
  if (rc < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] send_text failed: 0x" << std::hex << carrier_get_error() << std::dec;
    log_line(msg.str());
    return false;
  }
  log_line(std::string("[beagle-sdk] send_text ok msgid=") + std::to_string(msgid) + " peer=" + peer);
  return true;
}

bool BeagleSdk::send_media(const std::string& peer,
                           const std::string& caption,
                           const std::string& media_path,
                           const std::string& media_url,
                           const std::string& media_type,
                           const std::string& filename) {
  if (!g_state.carrier) return false;

  if (media_path.empty()) {
    std::string payload;
    if (!caption.empty()) payload += caption;
    if (!media_url.empty()) {
      if (!payload.empty()) payload += "\n";
      payload += media_url;
    }
    if (!filename.empty()) {
      if (!payload.empty()) payload += "\n";
      payload += "filename: " + filename;
    }
    if (!media_type.empty()) {
      if (!payload.empty()) payload += "\n";
      payload += "mediaType: " + media_type;
    }
    return send_text(peer, payload);
  }

  unsigned long long size = file_size_bytes(media_path);
  if (size == 0) {
    log_line(std::string("[beagle-sdk] send_media invalid file path: ") + media_path);
    return false;
  }
  std::string send_filename = sanitize_filename(!filename.empty() ? filename : basename_of(media_path));
  std::string send_media_type = !media_type.empty() ? media_type : infer_media_type_from_filename(send_filename);
  std::vector<unsigned char> file_bytes;
  if (!read_file_binary(media_path, file_bytes)) {
    log_line(std::string("[beagle-sdk] send_media failed to read file: ") + media_path);
    return false;
  }
  if (file_bytes.size() > kMaxBeaglechatFileBytes) {
    log_line(std::string("[beagle-sdk] send_media file too large for beaglechat payload: ")
             + media_path + " size=" + std::to_string(file_bytes.size())
             + " max=" + std::to_string(kMaxBeaglechatFileBytes));
    return false;
  }

  std::vector<unsigned char> payload;
  if (!encode_beaglechat_file_payload(send_filename, send_media_type, file_bytes, payload)) {
    log_line("[beagle-sdk] send_media failed to pack beaglechat payload");
    return false;
  }

  if (!caption.empty()) {
    send_text(peer, caption);
  }

  uint32_t msgid = 0;
  int rc = carrier_send_friend_message(g_state.carrier,
                                       peer.c_str(),
                                       payload.data(),
                                       payload.size(),
                                       &msgid,
                                       nullptr,
                                       nullptr);
  if (rc < 0) {
    std::ostringstream msg;
    msg << "[beagle-sdk] send_media(beaglechat payload) failed: 0x" << std::hex
        << carrier_get_error() << std::dec;
    log_line(msg.str());
    return false;
  }

  log_line(std::string("[beagle-sdk] send_media(beaglechat payload) ok msgid=")
           + std::to_string(msgid)
           + " peer=" + peer
           + " file=" + send_filename
           + " size=" + std::to_string(size));
  return true;
}

#if !BEAGLE_SDK_STUB
BeagleStatus BeagleSdk::status() const {
  std::lock_guard<std::mutex> lock(g_state.state_mu);
  return g_state.status;
}
#endif

#endif

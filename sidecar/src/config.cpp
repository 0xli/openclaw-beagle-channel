#include "types.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

Config Config::fromJson(const json& j) {
    Config config;
    
    if (j.contains("port")) config.port = j["port"];
    if (j.contains("host")) config.host = j["host"];
    if (j.contains("authToken")) config.authToken = j["authToken"];
    if (j.contains("logLevel")) config.logLevel = j["logLevel"];
    
    if (j.contains("carrier")) {
        const auto& carrierJson = j["carrier"];
        if (carrierJson.contains("bootstrapNodes")) {
            config.carrier.bootstrapNodes = carrierJson["bootstrapNodes"].get<std::vector<std::string>>();
        }
        if (carrierJson.contains("dataDir")) {
            config.carrier.dataDir = carrierJson["dataDir"];
        }
    }
    
    return config;
}

Config Config::fromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }
    
    json j;
    file >> j;
    return fromJson(j);
}

json Message::toJson() const {
    return json{
        {"messageId", messageId},
        {"peerId", peerId},
        {"text", text},
        {"timestamp", timestamp},
        {"chatId", chatId}
    };
}

Message Message::fromJson(const json& j) {
    Message msg;
    msg.messageId = j.value("messageId", "");
    msg.peerId = j.value("peerId", "");
    msg.text = j.value("text", "");
    msg.timestamp = j.value("timestamp", 0);
    msg.chatId = j.value("chatId", "");
    return msg;
}

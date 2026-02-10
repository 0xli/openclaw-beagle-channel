#pragma once

#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Configuration for the Beagle Sidecar
 */
struct Config {
    int port = 8765;
    std::string host = "127.0.0.1";
    std::string authToken;
    
    struct CarrierConfig {
        std::vector<std::string> bootstrapNodes;
        std::string dataDir = "/var/lib/beagle-sidecar";
    } carrier;
    
    std::string logLevel = "info";
    
    static Config fromJson(const json& j);
    static Config fromFile(const std::string& path);
};

/**
 * Message structure for Carrier messages
 */
struct Message {
    std::string messageId;
    std::string peerId;
    std::string text;
    int64_t timestamp;
    std::string chatId;
    
    json toJson() const;
    static Message fromJson(const json& j);
};

/**
 * Carrier client interface
 */
class CarrierClient {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    virtual ~CarrierClient() = default;
    
    virtual bool initialize(const Config::CarrierConfig& config) = 0;
    virtual void shutdown() = 0;
    
    virtual bool sendMessage(const std::string& peerId, const std::string& text, std::string& outMessageId) = 0;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    virtual bool isConnected() const = 0;
    virtual std::string getNodeId() const = 0;
    
    static std::unique_ptr<CarrierClient> create();
};

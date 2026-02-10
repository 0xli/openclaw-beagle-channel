#pragma once

#include "types.h"
#include <httplib.h>
#include <memory>
#include <vector>
#include <mutex>

/**
 * WebSocket connection for event streaming
 */
struct WebSocketConnection {
    int id;
    // In a real implementation, this would hold the WebSocket connection
    void* ws;
};

/**
 * HTTP/WebSocket server for the sidecar API
 */
class SidecarServer {
public:
    SidecarServer(const Config& config, std::shared_ptr<CarrierClient> carrier);
    ~SidecarServer();
    
    bool start();
    void stop();
    
private:
    Config config_;
    std::shared_ptr<CarrierClient> carrier_;
    std::unique_ptr<httplib::Server> httpServer_;
    
    std::vector<WebSocketConnection> wsConnections_;
    std::mutex wsMutex_;
    
    void setupRoutes();
    bool authenticate(const httplib::Request& req);
    
    void handleSend(const httplib::Request& req, httplib::Response& res);
    void handleHealth(const httplib::Request& req, httplib::Response& res);
    void handleEvents(const httplib::Request& req, httplib::Response& res);
    
    void broadcastMessage(const Message& message);
};

#include "sidecar_server.h"
#include <iostream>

// Utility functions
extern int64_t getCurrentTimestamp();
extern std::string generateMessageId();

SidecarServer::SidecarServer(const Config& config, std::shared_ptr<CarrierClient> carrier)
    : config_(config), carrier_(carrier) {
    httpServer_ = std::make_unique<httplib::Server>();
}

SidecarServer::~SidecarServer() {
    stop();
}

bool SidecarServer::authenticate(const httplib::Request& req) {
    // If no auth token is configured, allow all requests
    if (config_.authToken.empty()) {
        return true;
    }
    
    // Check Authorization header
    auto auth = req.get_header_value("Authorization");
    std::string expected = "Bearer " + config_.authToken;
    
    return auth == expected;
}

void SidecarServer::setupRoutes() {
    // POST /send - Send a message
    httpServer_->Post("/send", [this](const httplib::Request& req, httplib::Response& res) {
        handleSend(req, res);
    });
    
    // GET /health - Health check
    httpServer_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });
    
    // WebSocket /events - Event stream
    // Note: cpp-httplib doesn't support WebSockets natively
    // This is a simplified implementation - in production, use a proper WebSocket library
    httpServer_->Get("/events", [this](const httplib::Request& req, httplib::Response& res) {
        handleEvents(req, res);
    });
}

void SidecarServer::handleSend(const httplib::Request& req, httplib::Response& res) {
    // Authenticate
    if (!authenticate(req)) {
        res.status = 401;
        res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
        return;
    }
    
    try {
        // Parse request
        json reqJson = json::parse(req.body);
        std::string peerId = reqJson["peerId"];
        std::string text = reqJson["text"];
        
        // Send message via Carrier
        std::string messageId;
        bool success = carrier_->sendMessage(peerId, text, messageId);
        
        if (!success) {
            res.status = 500;
            res.set_content("{\"error\":\"Failed to send message\"}", "application/json");
            return;
        }
        
        // Return response
        json response = {
            {"messageId", messageId},
            {"timestamp", getCurrentTimestamp()},
            {"success", true}
        };
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void SidecarServer::handleHealth(const httplib::Request& req, httplib::Response& res) {
    json response = {
        {"status", "ok"},
        {"version", "1.0.0"},
        {"carrier", {
            {"connected", carrier_->isConnected()},
            {"nodeId", carrier_->getNodeId()}
        }}
    };
    
    res.set_content(response.dump(), "application/json");
}

void SidecarServer::handleEvents(const httplib::Request& req, httplib::Response& res) {
    // Authenticate
    if (!authenticate(req)) {
        res.status = 401;
        res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
        return;
    }
    
    // Note: This is a simplified stub. In production, implement proper WebSocket handling
    // using a library like uWebSockets or Boost.Beast
    
    res.status = 501;
    res.set_content("{\"error\":\"WebSocket endpoint not fully implemented - use dedicated WebSocket library\"}", "application/json");
}

void SidecarServer::broadcastMessage(const Message& message) {
    // TODO: Broadcast to all connected WebSocket clients
    std::lock_guard<std::mutex> lock(wsMutex_);
    
    std::string msgJson = message.toJson().dump();
    
    for (auto& conn : wsConnections_) {
        // In production: send msgJson to conn.ws
        std::cout << "[Server] Broadcasting to WS client " << conn.id << ": " << msgJson << std::endl;
    }
}

bool SidecarServer::start() {
    setupRoutes();
    
    // Set up Carrier message callback
    carrier_->setMessageCallback([this](const Message& msg) {
        std::cout << "[Server] Received message from " << msg.peerId << std::endl;
        broadcastMessage(msg);
    });
    
    std::cout << "[Server] Starting on " << config_.host << ":" << config_.port << std::endl;
    
    // Start server (blocking call)
    return httpServer_->listen(config_.host.c_str(), config_.port);
}

void SidecarServer::stop() {
    if (httpServer_) {
        std::cout << "[Server] Stopping..." << std::endl;
        httpServer_->stop();
    }
}

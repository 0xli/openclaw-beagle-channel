#include "types.h"
#include "sidecar_server.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <getopt.h>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  --config PATH    Path to config file (default: /etc/beagle-sidecar/config.json)\n"
              << "  --debug          Enable debug logging\n"
              << "  --help           Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string configPath = "/etc/beagle-sidecar/config.json";
    bool debug = false;
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "c:dh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                configPath = optarg;
                break;
            case 'd':
                debug = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    std::cout << "=== Beagle Chat Carrier Sidecar v1.0.0 ===" << std::endl;
    std::cout << "[Main] Loading config from: " << configPath << std::endl;
    
    // Load configuration
    Config config;
    try {
        config = Config::fromFile(configPath);
        if (debug) {
            config.logLevel = "debug";
        }
    } catch (const std::exception& e) {
        std::cerr << "[Main] Failed to load config: " << e.what() << std::endl;
        std::cerr << "[Main] Using default configuration" << std::endl;
        // Use default config
    }
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Initialize Carrier client
    auto carrier = CarrierClient::create();
    std::cout << "[Main] Initializing Carrier client..." << std::endl;
    
    if (!carrier->initialize(config.carrier)) {
        std::cerr << "[Main] Failed to initialize Carrier client" << std::endl;
        return 1;
    }
    
    std::cout << "[Main] Carrier initialized successfully" << std::endl;
    
    // Create and start server
    auto server = std::make_unique<SidecarServer>(config, std::move(carrier));
    
    std::cout << "[Main] Starting HTTP server..." << std::endl;
    std::cout << "[Main] API available at http://" << config.host << ":" << config.port << std::endl;
    std::cout << "[Main] Press Ctrl+C to stop" << std::endl;
    
    // Start server in a separate thread since listen() blocks
    std::thread serverThread([&server]() {
        server->start();
    });
    
    // Wait for shutdown signal
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    server->stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "[Main] Shutdown complete" << std::endl;
    return 0;
}

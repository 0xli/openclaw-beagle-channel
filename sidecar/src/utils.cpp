#include "types.h"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

/**
 * Utility functions
 */

std::string generateMessageId() {
    // Generate a simple UUID-like message ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex = "0123456789abcdef";
    std::stringstream ss;
    ss << "msg-";
    for (int i = 0; i < 16; i++) {
        ss << hex[dis(gen)];
    }
    return ss.str();
}

int64_t getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

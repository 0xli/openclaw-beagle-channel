#ifndef CARRIER_WRAPPER_H
#define CARRIER_WRAPPER_H

#include <napi.h>
#include <string>
#include <functional>
#include <memory>

// Forward declarations for Carrier types
typedef struct Carrier Carrier;
typedef struct CarrierCallbacks CarrierCallbacks;

/**
 * C++ wrapper for Elastos Carrier functionality
 * This class provides a bridge between Node.js and the Carrier C library
 */
class CarrierWrapper {
public:
    CarrierWrapper();
    ~CarrierWrapper();

    // Initialization
    bool Initialize(const std::string& dataDir, const std::string& bootstrapNodes);
    void Shutdown();

    // Connection management
    bool Start();
    void Stop();
    bool IsReady() const;

    // User information
    std::string GetAddress() const;
    std::string GetUserId() const;
    bool SetUserInfo(const std::string& name, const std::string& description);

    // Friend management
    bool AddFriend(const std::string& address, const std::string& hello);
    bool RemoveFriend(const std::string& userId);
    bool AcceptFriend(const std::string& userId);

    // Messaging
    bool SendMessage(const std::string& friendId, const std::string& message);

    // Callbacks
    void SetOnConnectionChanged(std::function<void(bool)> callback);
    void SetOnFriendRequest(std::function<void(std::string, std::string, std::string)> callback);
    void SetOnFriendAdded(std::function<void(std::string)> callback);
    void SetOnFriendMessage(std::function<void(std::string, std::string)> callback);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    // Disable copy
    CarrierWrapper(const CarrierWrapper&) = delete;
    CarrierWrapper& operator=(const CarrierWrapper&) = delete;
};

#endif // CARRIER_WRAPPER_H

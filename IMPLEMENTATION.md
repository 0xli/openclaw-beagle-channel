# Implementation Guide

This guide explains how to complete the integration between the Beagle channel and the Elastos Carrier SDK.

## Overview

The current implementation provides a complete structure with mock functionality. To make it fully functional, you need to:

1. Build and install the Elastos Carrier SDK
2. Update the C++ wrapper to use real Carrier API calls
3. Handle Carrier callbacks and events
4. Integrate with OpenClaw's channel system

## Step 1: Build Elastos Carrier SDK

### On Ubuntu

```bash
# Install prerequisites
sudo apt-get update
sudo apt-get install -f build-essential autoconf automake autopoint \
    libtool flex bison libncurses5-dev cmake

# Clone and build
git clone https://github.com/0xli/Elastos.NET.Carrier.Native.SDK.git
cd Elastos.NET.Carrier.Native.SDK

# Build for Linux
mkdir -p build/linux && cd build/linux
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ../..
make
sudo make install

# Verify installation
ls /usr/local/include/carrier.h
ls /usr/local/lib/libelacarrier.*
```

### Copy to Project Dependencies

```bash
cd /path/to/openclaw-beagle-channel
mkdir -p deps/elastos-carrier/{include,lib}

# Copy headers
cp /usr/local/include/carrier*.h deps/elastos-carrier/include/

# Copy libraries
cp /usr/local/lib/libelacarrier.* deps/elastos-carrier/lib/
cp /usr/local/lib/libelasession.* deps/elastos-carrier/lib/
```

## Step 2: Update C++ Wrapper Implementation

The file `src/addon/carrier_wrapper.cc` currently contains stub implementations. Replace them with actual Carrier SDK calls.

### Key Changes Needed

#### 2.1 Include Carrier Headers

```cpp
#include "carrier_wrapper.h"
#include <carrier.h>
#include <carrier_session.h>
#include <iostream>
#include <cstring>
```

#### 2.2 Update Impl Structure

```cpp
struct CarrierWrapper::Impl {
    Carrier* carrier = nullptr;
    std::string dataDir;
    bool ready = false;
    bool running = false;

    std::function<void(bool)> onConnectionChanged;
    std::function<void(std::string, std::string, std::string)> onFriendRequest;
    std::function<void(std::string)> onFriendAdded;
    std::function<void(std::string, std::string)> onFriendMessage;
    
    // Carrier options
    CarrierOptions opts;
};
```

#### 2.3 Implement Carrier Callbacks

```cpp
// Connection status callback
static void on_connection_status(Carrier *w, CarrierConnectionStatus status, void *context) {
    auto impl = static_cast<CarrierWrapper::Impl*>(context);
    bool connected = (status == CarrierConnectionStatus_Connected);
    
    impl->ready = connected;
    if (impl->onConnectionChanged) {
        impl->onConnectionChanged(connected);
    }
}

// Friend request callback
static void on_friend_request(Carrier *w, const char *userid,
                              const CarrierUserInfo *info,
                              const char *hello, void *context) {
    auto impl = static_cast<CarrierWrapper::Impl*>(context);
    
    if (impl->onFriendRequest) {
        std::string userId(userid);
        std::string name(info->name ? info->name : "");
        std::string helloMsg(hello ? hello : "");
        impl->onFriendRequest(userId, name, helloMsg);
    }
}

// Friend message callback
static void on_friend_message(Carrier *w, const char *from,
                              const void *msg, size_t len,
                              int64_t timestamp, bool is_offline,
                              void *context) {
    auto impl = static_cast<CarrierWrapper::Impl*>(context);
    
    if (impl->onFriendMessage && msg && len > 0) {
        std::string friendId(from);
        std::string message(static_cast<const char*>(msg), len);
        impl->onFriendMessage(friendId, message);
    }
}

// Friend added callback
static void on_friend_added(Carrier *w, const CarrierFriendInfo *info, void *context) {
    auto impl = static_cast<CarrierWrapper::Impl*>(context);
    
    if (impl->onFriendAdded && info) {
        std::string userId(info->user_info.userid);
        impl->onFriendAdded(userId);
    }
}
```

#### 2.4 Update Initialize Method

```cpp
bool CarrierWrapper::Initialize(const std::string& dataDir, const std::string& bootstrapNodes) {
    if (pImpl->carrier) {
        return true; // Already initialized
    }

    pImpl->dataDir = dataDir;
    
    // Parse bootstrap nodes from JSON
    // (You'll need to parse the JSON string into the opts.bootstraps array)
    
    // Set up Carrier options
    memset(&pImpl->opts, 0, sizeof(pImpl->opts));
    pImpl->opts.udp_enabled = true;
    pImpl->opts.persistent_location = dataDir.c_str();
    
    // Set up callbacks
    CarrierCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.connection_status = on_connection_status;
    callbacks.friend_request = on_friend_request;
    callbacks.friend_message = on_friend_message;
    callbacks.friend_added = on_friend_added;
    
    // Create Carrier instance
    pImpl->carrier = carrier_new(&pImpl->opts, &callbacks, pImpl.get());
    
    if (!pImpl->carrier) {
        std::cerr << "Failed to create Carrier instance: " 
                  << carrier_get_error() << std::endl;
        return false;
    }
    
    return true;
}
```

#### 2.5 Update Start Method

```cpp
bool CarrierWrapper::Start() {
    if (!pImpl->carrier) {
        return false;
    }
    
    if (pImpl->running) {
        return true;
    }
    
    int rc = carrier_run(pImpl->carrier, 10); // 10ms interval
    if (rc < 0) {
        std::cerr << "Failed to start Carrier: " 
                  << carrier_get_error() << std::endl;
        return false;
    }
    
    pImpl->running = true;
    return true;
}
```

#### 2.6 Update Other Methods

Update `SendMessage`, `AddFriend`, `GetAddress`, etc. to use actual Carrier API calls:

```cpp
bool CarrierWrapper::SendMessage(const std::string& friendId, const std::string& message) {
    if (!pImpl->carrier || !pImpl->ready) {
        return false;
    }
    
    int rc = carrier_send_friend_message(
        pImpl->carrier,
        friendId.c_str(),
        message.c_str(),
        message.length(),
        NULL  // receipt callback
    );
    
    return rc == 0;
}

std::string CarrierWrapper::GetAddress() const {
    if (!pImpl->carrier) {
        return "";
    }
    
    char address[CARRIER_MAX_ADDRESS_LEN + 1];
    char* addr = carrier_get_address(pImpl->carrier, address, sizeof(address));
    
    return addr ? std::string(addr) : "";
}

bool CarrierWrapper::AddFriend(const std::string& address, const std::string& hello) {
    if (!pImpl->carrier) {
        return false;
    }
    
    int rc = carrier_add_friend(
        pImpl->carrier,
        address.c_str(),
        hello.c_str()
    );
    
    return rc == 0;
}
```

## Step 3: Handle Threading

The Carrier SDK requires running its event loop. You have two options:

### Option A: Separate Thread

Create a dedicated thread for the Carrier event loop:

```cpp
bool CarrierWrapper::Start() {
    if (!pImpl->carrier || pImpl->running) {
        return pImpl->running;
    }
    
    pImpl->running = true;
    
    // Start Carrier in a separate thread
    pImpl->carrierThread = std::thread([this]() {
        carrier_run(pImpl->carrier, 10);
    });
    
    return true;
}
```

### Option B: Integration with Node.js Event Loop

Use `uv_async` to integrate with Node.js event loop (more complex but better).

## Step 4: Update Build Configuration

Ensure `binding.gyp` correctly links the Carrier libraries:

```json
{
  "targets": [
    {
      "target_name": "beagle_carrier",
      "sources": [
        "src/addon/beagle_carrier.cc",
        "src/addon/carrier_wrapper.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/usr/local/include",
        "deps/elastos-carrier/include"
      ],
      "libraries": [
        "-L/usr/local/lib",
        "-L<(module_root_dir)/deps/elastos-carrier/lib",
        "-lelacarrier",
        "-lelasession"
      ],
      "ldflags": [
        "-Wl,-rpath,/usr/local/lib",
        "-Wl,-rpath,'$$ORIGIN/../deps/elastos-carrier/lib'"
      ]
    }
  ]
}
```

## Step 5: Testing

### 5.1 Build and Test

```bash
npm run build
node -e "const b = require('./build/Release/beagle_carrier.node'); console.log('Loaded:', b);"
```

### 5.2 Create Test Script

```javascript
const { BeagleCarrier } = require('./dist/src/beagle-carrier.js');

const carrier = new BeagleCarrier();

carrier.initialize({
    dataDir: './test-data',
    bootstrapNodes: '[]' // Use actual bootstrap nodes
});

carrier.onConnectionChanged((connected) => {
    console.log('Connected:', connected);
    if (connected) {
        console.log('Address:', carrier.getAddress());
    }
});

carrier.onMessage((from, msg) => {
    console.log(`Message from ${from}: ${msg}`);
});

carrier.start();
```

## Step 6: Integration with OpenClaw

Once the native addon works, update the OpenClaw integration:

1. Follow OpenClaw's plugin development guide
2. Register the channel in OpenClaw's configuration
3. Implement proper message routing
4. Add pairing and security features
5. Test with actual Beagle clients

## Common Issues and Solutions

### Issue: Cannot find carrier.h

**Solution**: Make sure the Carrier SDK is installed and headers are in the include path.

### Issue: Linking errors

**Solution**: Check that library paths are correct and libraries are installed.

### Issue: Segmentation fault

**Solution**: Ensure proper memory management and callback context handling.

### Issue: Carrier not connecting

**Solution**: Check bootstrap nodes, firewall settings, and network connectivity.

## Next Steps

1. Implement error handling and logging
2. Add support for file transfers
3. Implement group messaging
4. Add session management
5. Create comprehensive tests
6. Optimize performance
7. Add monitoring and metrics

## Resources

- [Elastos Carrier API Reference](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK/tree/master/docs)
- [Node-API Documentation](https://nodejs.org/api/n-api.html)
- [OpenClaw Plugin Development](https://docs.openclaw.ai/plugins)

## Support

If you encounter issues:
1. Check the Elastos Carrier SDK documentation
2. Review the shell example in the SDK repository
3. Create an issue on GitHub with details and logs

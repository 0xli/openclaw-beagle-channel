# Architecture Overview

This document provides a high-level overview of the Beagle Channel architecture and how all components work together.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      OpenClaw Platform                       │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              OpenClaw Agent (AI Core)               │   │
│  └────────────────────┬────────────────────────────────┘   │
│                       │                                      │
│  ┌────────────────────▼────────────────────────────────┐   │
│  │          Channel Plugin System (TypeScript)         │   │
│  │  ┌──────────┬──────────┬──────────┬──────────────┐ │   │
│  │  │ Telegram │ WhatsApp │  Slack   │ Beagle (New!)│ │   │
│  │  └──────────┴──────────┴──────────┴──────┬───────┘ │   │
│  └──────────────────────────────────────────┼─────────┘   │
└─────────────────────────────────────────────┼─────────────┘
                                              │
                 ┌────────────────────────────▼────────┐
                 │  Beagle Channel Plugin (This Repo) │
                 └────────────────────────────┬────────┘
                                              │
        ┌─────────────────────────────────────┼─────────────────┐
        │                                     │                 │
        ▼                                     ▼                 ▼
┌───────────────┐                  ┌──────────────────┐  ┌──────────┐
│ Plugin Entry  │                  │   Runtime Layer   │  │  Config  │
│  (index.ts)   │                  │   (runtime.ts)    │  │ Schema   │
└───────┬───────┘                  └────────┬─────────┘  └──────────┘
        │                                   │
        │                                   │
        │                          ┌────────▼──────────┐
        │                          │  TypeScript Layer │
        │                          │ (beagle-carrier.ts)│
        │                          └────────┬──────────┘
        │                                   │
        │                          ┌────────▼──────────┐
        └──────────────────────────►  Native Addon     │
                                   │    (C++ N-API)    │
                                   └────────┬──────────┘
                                            │
                                   ┌────────▼──────────┐
                                   │ Elastos Carrier   │
                                   │   C Library       │
                                   └────────┬──────────┘
                                            │
                                            ▼
                                   ╔════════════════════╗
                                   ║  Carrier Network   ║
                                   ║  (Decentralized    ║
                                   ║   P2P Network)     ║
                                   ╚═════════┬══════════╝
                                             │
                        ┌────────────────────┼────────────────────┐
                        ▼                    ▼                    ▼
                 ┌────────────┐      ┌────────────┐      ┌────────────┐
                 │   Beagle   │      │  Another   │      │   Future   │
                 │   Chat     │      │  Carrier   │      │  Carrier   │
                 │  Client    │      │  Client    │      │  Clients   │
                 └────────────┘      └────────────┘      └────────────┘
```

## Component Breakdown

### 1. Plugin Entry (`extension/index.ts`)

**Purpose**: Registers the Beagle channel with OpenClaw

**Responsibilities**:
- Implements OpenClaw's plugin interface
- Registers the channel with metadata
- Defines channel capabilities
- Provides lifecycle hooks (start, stop)
- Implements send/receive methods

**Key Functions**:
- `register(api)`: Called by OpenClaw to register the plugin
- `lifecycle.start()`: Initializes the channel
- `lifecycle.stop()`: Cleans up resources
- `send(params)`: Sends messages to Carrier network

### 2. Runtime Layer (`extension/runtime.ts`)

**Purpose**: Manages the Beagle channel runtime and state

**Responsibilities**:
- Initializes the Carrier connection
- Manages event handlers and callbacks
- Maintains connection state
- Handles friend list and pairing
- Routes messages between Carrier and OpenClaw

**Key Features**:
- Singleton runtime instance
- Event-based architecture
- Configuration management
- Bootstrap node management

### 3. TypeScript Wrapper (`src/beagle-carrier.ts`)

**Purpose**: Provides a clean TypeScript API over the native addon

**Responsibilities**:
- Wraps the C++ native addon
- Provides type-safe interfaces
- Handles errors and edge cases
- Falls back to mock implementation
- Manages native module lifecycle

**Key Features**:
- Mock mode for development
- Type definitions
- Error handling
- Promise-based API

### 4. Native Addon (`src/addon/`)

**Purpose**: Bridges Node.js and the Elastos Carrier C library

**Components**:

#### 4.1 `beagle_carrier.cc`
- N-API/Node-API bindings
- Exposes C++ classes to JavaScript
- Manages thread-safe callbacks
- Handles async operations

#### 4.2 `carrier_wrapper.cc/.h`
- C++ wrapper around Carrier C API
- Manages Carrier lifecycle
- Implements event callbacks
- Provides C++ API

**Key Features**:
- Thread-safe callbacks using ThreadSafeFunction
- RAII for resource management
- Event-driven architecture
- Mock implementation for development

### 5. Elastos Carrier SDK (External)

**Purpose**: Provides the core P2P networking functionality

**Features**:
- Decentralized peer discovery
- NAT traversal
- End-to-end encryption
- Friend management
- Message routing
- Session management

## Data Flow

### Outgoing Message Flow

```
OpenClaw Agent
    │
    ├─► Channel Plugin (index.ts)
    │       │
    │       ├─► Runtime (runtime.ts)
    │       │       │
    │       │       ├─► BeagleCarrier (beagle-carrier.ts)
    │       │       │       │
    │       │       │       ├─► Native Addon (beagle_carrier.cc)
    │       │       │       │       │
    │       │       │       │       ├─► Carrier Wrapper (carrier_wrapper.cc)
    │       │       │       │       │       │
    │       │       │       │       │       ├─► Carrier SDK (libcarrier)
    │       │       │       │       │       │       │
    │       │       │       │       │       │       └─► Carrier Network
    │       │       │       │       │       │               │
    │       │       │       │       │       │               └─► Peer Device
```

### Incoming Message Flow

```
Peer Device
    │
    └─► Carrier Network
            │
            └─► Carrier SDK (libcarrier)
                    │
                    ├─► Callback: on_friend_message()
                    │       │
                    │       ├─► Carrier Wrapper (carrier_wrapper.cc)
                    │       │       │
                    │       │       ├─► Native Addon (beagle_carrier.cc)
                    │       │       │       │
                    │       │       │       ├─► ThreadSafeFunction
                    │       │       │       │       │
                    │       │       │       │       ├─► JavaScript Callback
                    │       │       │       │       │       │
                    │       │       │       │       │       ├─► BeagleCarrier (beagle-carrier.ts)
                    │       │       │       │       │       │       │
                    │       │       │       │       │       │       ├─► Runtime (runtime.ts)
                    │       │       │       │       │       │       │       │
                    │       │       │       │       │       │       │       └─► Channel Plugin (index.ts)
                    │       │       │       │       │       │       │               │
                    │       │       │       │       │       │       │               └─► OpenClaw Agent
```

## Key Concepts

### 1. Carrier Address

A unique identifier for a Carrier node, similar to an email address or phone number.

- **Format**: 52-character base58-encoded string
- **Purpose**: Public identifier for adding friends
- **Generation**: Created when Carrier is first initialized
- **Persistence**: Stored in the data directory

### 2. User ID

A shorter identifier derived from the Carrier address.

- **Format**: 45-character base58-encoded string
- **Purpose**: Internal identifier for friends
- **Usage**: Used in API calls and friend management

### 3. Friend Request Flow

1. User A sends friend request with Carrier address of User B
2. User B receives friend request event
3. User B accepts or rejects the request
4. If accepted, both users are now friends
5. Friends can exchange messages

### 4. Message Flow

1. Sender calls `sendMessage(friendId, message)`
2. Message is queued in Carrier
3. Carrier delivers to peer over network
4. Receiver gets `onMessage` event
5. Application processes the message

### 5. Threading Model

- **Carrier Event Loop**: Runs in separate thread or integrated with Node.js
- **Callbacks**: Execute on Carrier thread
- **ThreadSafeFunction**: Safely calls JavaScript from C++ thread
- **Event Queue**: Messages queued for JavaScript processing

## Configuration

### Bootstrap Nodes

Essential for joining the Carrier network:

```json
[
  {
    "ipv4": "IP_ADDRESS",
    "port": PORT_NUMBER,
    "publicKey": "BASE64_PUBLIC_KEY"
  }
]
```

### Data Directory

Stores persistent data:
- Node identity (private key)
- Friend list
- Offline messages
- Configuration

## Security Considerations

### Current Implementation

- ✅ End-to-end encryption (provided by Carrier)
- ✅ Friend-based access control
- ⚠️ Auto-accept disabled by default
- ⚠️ Pairing policy to be implemented

### Production Requirements

1. **Pairing Policy**: Implement OpenClaw's pairing system
2. **Friend Approval**: Require explicit approval for friend requests
3. **Message Validation**: Validate incoming messages
4. **Rate Limiting**: Prevent spam and abuse
5. **Key Management**: Secure storage of Carrier keys

## Extension Points

### For Future Development

1. **Group Messaging**: Add support for Carrier groups
2. **File Transfer**: Implement file sharing via Carrier sessions
3. **Voice/Video**: Add WebRTC support via Carrier
4. **Presence**: Implement rich presence information
5. **Reactions**: Add message reactions
6. **Threading**: Support message threads

## Development Workflow

### Current State (Mock Implementation)

```
TypeScript Code → Compiles → Mock Native Addon → Logs to Console
```

### Future State (Full Implementation)

```
TypeScript Code → Compiles → Real Native Addon → Carrier SDK → Network
```

## Testing Strategy

### Unit Tests
- TypeScript layer mocking
- C++ wrapper unit tests
- Runtime state management

### Integration Tests
- Native addon loading
- Carrier SDK integration
- Event handling

### End-to-End Tests
- Message exchange with real peers
- Friend management
- Connection handling

## Performance Considerations

### Optimization Targets

1. **Message Latency**: < 1 second for direct messages
2. **Memory Usage**: < 50MB per instance
3. **CPU Usage**: < 5% idle, < 20% active
4. **Startup Time**: < 5 seconds to ready

### Bottlenecks

1. Carrier network discovery
2. NAT traversal
3. Message encryption/decryption
4. Thread synchronization

## Troubleshooting Guide

### Common Issues

1. **Native addon won't load**
   - Check Node.js version
   - Verify Carrier SDK installation
   - Check library paths

2. **Connection fails**
   - Verify bootstrap nodes
   - Check firewall settings
   - Test network connectivity

3. **Messages not received**
   - Verify friend relationship
   - Check Carrier connection status
   - Verify friend is online

4. **Memory leaks**
   - Check callback cleanup
   - Verify resource disposal
   - Monitor ThreadSafeFunction lifecycle

## Resources

- OpenClaw Plugin API: [docs.openclaw.ai](https://docs.openclaw.ai)
- Elastos Carrier SDK: [GitHub](https://github.com/0xli/Elastos.NET.Carrier.Native.SDK)
- Node-API Documentation: [nodejs.org](https://nodejs.org/api/n-api.html)
- Project Repository: [GitHub](https://github.com/0xli/openclaw-beagle-channel)

## Next Steps

See `IMPLEMENTATION.md` for detailed integration instructions.

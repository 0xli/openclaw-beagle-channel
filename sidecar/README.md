# Beagle Chat Carrier Sidecar

# Beagle Chat Carrier Sidecar

⚠️ **Current Status**: This directory contains the complete design and structure for the Carrier sidecar daemon. The Carrier SDK integration and WebSocket event streaming are currently **stub implementations** that need to be completed for production use.

This directory contains the design and implementation for the Carrier sidecar daemon.

## Current Implementation Status

### ✅ Complete
- HTTP server with REST API endpoints
- JSON configuration system
- systemd service integration
- Command-line argument parsing
- Graceful shutdown handling
- Health check endpoint
- CMake build system

### ⚠️ TODO - Needs Implementation
1. **Carrier SDK Integration** (`src/carrier_client.cpp`):
   - Replace stub with real Elastos Carrier SDK calls
   - Initialize Carrier node with bootstrap nodes
   - Implement actual message sending via Carrier protocol
   - Set up Carrier event callbacks for incoming messages

2. **WebSocket Event Streaming** (`src/sidecar_server.cpp`):
   - Integrate a WebSocket library (uWebSockets or Boost.Beast)
   - Implement `/events` endpoint with proper WebSocket upgrade
   - Broadcast incoming Carrier messages to connected WebSocket clients

3. **Testing**:
   - End-to-end message flow testing
   - Integration tests with real Carrier network
   - Load testing for multiple concurrent connections

## Overview

The sidecar is a small Ubuntu service that uses the Elastos Carrier SDK (C/C++) to handle the actual messaging. It exposes a local REST API and WebSocket endpoint for the OpenClaw TypeScript plugin to communicate with.

## Architecture

```
┌─────────────────┐         HTTP/WS         ┌──────────────────┐
│  OpenClaw Agent │ ◄──────────────────────► │ Carrier Sidecar  │
│  (TypeScript)   │                          │  (C++/Native)    │
└─────────────────┘                          └──────────────────┘
                                                      │
                                                      │ Carrier SDK
                                                      ▼
                                              ┌──────────────────┐
                                              │ Beagle Chat      │
                                              │ Network          │
                                              └──────────────────┘
```

## API Endpoints

### 1. POST /send
Send a message to a peer.

**Request:**
```json
{
  "peerId": "carrier-peer-id-here",
  "text": "Hello from OpenClaw!"
}
```

**Response:**
```json
{
  "messageId": "msg-uuid-123",
  "timestamp": 1707523041000,
  "success": true
}
```

### 2. GET /health
Health check endpoint.

**Response:**
```json
{
  "status": "ok",
  "version": "1.0.0",
  "carrier": {
    "connected": true,
    "nodeId": "carrier-node-id"
  }
}
```

### 3. WebSocket /events
Subscribe to incoming messages.

**Server → Client Events:**
```json
{
  "messageId": "msg-uuid-456",
  "peerId": "sender-peer-id",
  "text": "Hello from Beagle!",
  "timestamp": 1707523042000,
  "chatId": "chat-123"
}
```

## Security

- **Local-only**: Sidecar binds to `127.0.0.1` only (not exposed to network)
- **Auth Token**: Optional Bearer token authentication
- **TLS**: Optional HTTPS/WSS support for local connections
- **Port**: Default 8765 (configurable)

## Configuration

Create `/etc/beagle-sidecar/config.json`:

```json
{
  "port": 8765,
  "host": "127.0.0.1",
  "authToken": "your-secure-random-token-here",
  "carrier": {
    "bootstrapNodes": [
      "bootstrap1.beaglechat.io:33445",
      "bootstrap2.beaglechat.io:33445"
    ],
    "dataDir": "/var/lib/beagle-sidecar"
  },
  "logLevel": "info"
}
```

## Building

### Dependencies

On Ubuntu, install Elastos Carrier SDK dependencies:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  libssl-dev \
  libsodium-dev \
  git
```

### Clone and Build Carrier SDK

```bash
# Clone the Elastos Carrier SDK
git clone https://github.com/elastos/Elastos.NET.Carrier.Native.SDK.git
cd Elastos.NET.Carrier.Native.SDK

# Build the SDK
mkdir build && cd build
cmake ..
make
sudo make install
```

### Build the Sidecar

```bash
cd sidecar
mkdir build && cd build
cmake ..
make
sudo make install
```

## Running

### As a Service (systemd)

```bash
sudo systemctl start beagle-sidecar
sudo systemctl enable beagle-sidecar
sudo systemctl status beagle-sidecar
```

### Manual Testing

```bash
# Run in foreground with debug logging
beagle-sidecar --config /etc/beagle-sidecar/config.json --debug
```

## Testing

### Test sending a message

```bash
curl -X POST http://localhost:8765/send \
  -H "Authorization: Bearer your-token" \
  -H "Content-Type: application/json" \
  -d '{
    "peerId": "test-peer-id",
    "text": "Test message"
  }'
```

### Test WebSocket events

```bash
# Using websocat (install: cargo install websocat)
websocat ws://localhost:8765/events -H "Authorization: Bearer your-token"
```

## Logging

Logs are written to:
- **systemd journal**: `journalctl -u beagle-sidecar -f`
- **File**: `/var/log/beagle-sidecar/sidecar.log` (if configured)

## Troubleshooting

### Sidecar won't start
- Check Carrier SDK is installed: `ldconfig -p | grep carrier`
- Verify config file syntax: `cat /etc/beagle-sidecar/config.json | jq`
- Check logs: `journalctl -u beagle-sidecar -n 50`

### Can't connect to Carrier network
- Verify bootstrap nodes are reachable
- Check firewall settings (Carrier uses UDP)
- Ensure data directory is writable

### WebSocket disconnects
- Check network stability
- Verify auth token matches
- Review reconnect settings in OpenClaw plugin

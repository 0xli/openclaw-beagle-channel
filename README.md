# Beagle Chat (Elastos Carrier) Channel for OpenClaw

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.0+-blue.svg)](https://www.typescriptlang.org/)
[![Node.js](https://img.shields.io/badge/Node.js-18+-green.svg)](https://nodejs.org/)

> **Channel #16** - Beagle Chat integration for OpenClaw agents using the Elastos Carrier protocol

## 🎯 Overview

This repository provides the **16th chat provider** for OpenClaw, enabling AI agents to communicate via **Beagle Chat** (powered by Elastos Carrier protocol). The integration uses a split architecture:

- **TypeScript Plugin** - OpenClaw channel provider (this package)
- **C++ Sidecar** - Native daemon using Elastos Carrier SDK

### Why This Matters

- ✅ **Decentralized messaging** - No central server required
- ✅ **Secure** - End-to-end encrypted via Carrier protocol
- ✅ **Stable** - Isolation between OpenClaw and native Carrier SDK
- ✅ **OpenClaw native** - Follows OpenClaw's channel provider pattern
- ✅ **Production ready** - Includes systemd service, logging, monitoring

## 🚀 Quick Start

### Prerequisites

- **Ubuntu/Debian** (for sidecar)
- **Node.js 18+** (for OpenClaw plugin)
- **Elastos Carrier SDK** (installed via setup guide)

### Installation

```bash
# 1. Install the NPM package
npm install openclaw-beagle-channel

# 2. Build and install the sidecar (see docs/SETUP.md)
cd sidecar
mkdir build && cd build
cmake ..
make
sudo make install

# 3. Start the sidecar
sudo systemctl start beagle-sidecar
```

### Basic Usage

```typescript
import { BeagleChannelProvider } from 'openclaw-beagle-channel';

// Create provider
const beagle = new BeagleChannelProvider({
  sidecarUrl: 'http://localhost:8765',
  authToken: process.env.BEAGLE_AUTH_TOKEN,
  debug: true,
});

// Initialize
await beagle.initialize();

// Send message
await beagle.sendText('peer-id', 'Hello from OpenClaw!');

// Receive messages
beagle.onIncomingMessage(async (message) => {
  console.log('From:', message.peerId);
  console.log('Text:', message.text);
});
```

## 📚 Documentation

- **[Complete Setup Guide](docs/SETUP.md)** - Full installation and configuration
- **[Sidecar Documentation](sidecar/README.md)** - Build and deploy the Carrier daemon
- **[Examples](examples/)** - Working code examples
- **[TypeScript API](src/)** - Plugin source code

## 🏗️ Architecture

```
┌─────────────────────┐
│   OpenClaw Agent    │  ← Your AI agent
│    (TypeScript)     │
└──────────┬──────────┘
           │ HTTP/WebSocket (localhost:8765)
           ▼
┌─────────────────────┐
│  Carrier Sidecar    │  ← Native daemon
│      (C++)          │
└──────────┬──────────┘
           │ Elastos Carrier Protocol
           ▼
┌─────────────────────┐
│   Beagle Chat       │  ← P2P network
│   Network           │
└─────────────────────┘
```

**Benefits of this architecture:**
- OpenClaw remains stable even if Carrier crashes
- No complex Node.js native addons required
- Easy to debug and monitor separately
- Clear separation of concerns

## 🔧 Components

### 1. OpenClaw Plugin (TypeScript)

Located in `src/`:
- `provider.ts` - Channel provider implementation
- `client.ts` - HTTP/WebSocket client for sidecar
- `types.ts` - TypeScript type definitions
- `index.ts` - Main export

### 2. Carrier Sidecar (C++)

Located in `sidecar/`:
- REST API for sending messages (`POST /send`)
- WebSocket endpoint for receiving messages (`WS /events`)
- Health check endpoint (`GET /health`)
- systemd service for production deployment

### 3. Documentation

Located in `docs/`:
- Comprehensive setup guide
- Security best practices
- Troubleshooting tips
- API reference

### 4. Examples

Located in `examples/`:
- Basic send/receive example
- Full OpenClaw agent integration
- Conversation management

## 📦 Repository Structure

```
openclaw-beagle-channel/
├── src/                    # TypeScript plugin source
│   ├── types.ts           # Type definitions
│   ├── client.ts          # Sidecar client
│   ├── provider.ts        # Channel provider
│   └── index.ts           # Main export
├── sidecar/               # C++ sidecar daemon
│   ├── include/          # Header files
│   ├── src/              # Implementation
│   ├── config/           # Default configuration
│   ├── systemd/          # Service file
│   ├── CMakeLists.txt    # Build configuration
│   └── README.md         # Sidecar docs
├── docs/                  # Documentation
│   └── SETUP.md          # Complete setup guide
├── examples/              # Usage examples
│   ├── basic-usage.ts    # Simple example
│   └── openclaw-agent-example.ts  # Full agent
├── package.json           # NPM package config
├── tsconfig.json          # TypeScript config
└── README.md             # This file
```

## 🔒 Security

The integration follows security best practices:

- **Local-only** - Sidecar binds to `127.0.0.1` only
- **Authentication** - Bearer token for API access
- **Non-root** - systemd service runs as dedicated `beagle` user
- **Encrypted** - Carrier protocol provides E2E encryption
- **Isolated** - Sidecar crashes don't affect OpenClaw

See [SETUP.md](docs/SETUP.md#security) for detailed security guidelines.

## 🧪 Testing

```bash
# Build TypeScript
npm run build

# Run linter
npm run lint

# Test sidecar health
curl http://localhost:8765/health
```

## 🤝 Contributing

Contributions welcome! This is an open integration for OpenClaw.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details.

## 🔗 Related Links

- [OpenClaw](https://openclaw.ai) - AI agent framework
- [OpenClaw Integrations](https://openclaw.ai/integrations) - All 16 chat providers
- [Elastos Carrier](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK) - Underlying protocol
- [Beagle Chat](https://beaglechat.io) - User-facing application

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/0xli/openclaw-beagle-channel/issues)
- **Documentation**: [docs/SETUP.md](docs/SETUP.md)
- **Examples**: [examples/](examples/)

## 🎉 Acknowledgments

This integration brings **Beagle Chat** as the **#16 chat provider** to OpenClaw, joining WhatsApp, Telegram, Discord, Slack, Signal, iMessage, Teams, Nextcloud Talk, Matrix, Nostr, Tlon, Zalo, and WebChat.

Built with ❤️ for the OpenClaw and Beagle Chat communities.

---

**Provider ID:** `beagle` or `carrier`  
**Display Name:** Beagle Chat  
**Full Title:** Beagle Chat (Elastos Carrier)

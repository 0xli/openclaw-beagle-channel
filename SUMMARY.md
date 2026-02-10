# Project Summary

## What This Is

This repository implements a **chat provider/channel** for [OpenClaw](https://github.com/openclaw/openclaw) that enables communication over the **Elastos Carrier** decentralized peer-to-peer network. This allows Beagle chat clients and other Carrier-based applications to interact with OpenClaw AI agents.

## What You Can Do With This

Once fully integrated:

1. **Chat with OpenClaw via Beagle**: Use the Beagle chat app to message your personal AI assistant
2. **Decentralized Communication**: No central servers required - all communication is peer-to-peer
3. **Cross-Platform**: Works on Ubuntu, macOS, and other platforms (Ubuntu is the primary target)
4. **Private & Secure**: End-to-end encrypted communication via the Carrier network

## Current Status

### ✅ What's Done

- **Complete architecture design** with three-layer approach (Plugin → TypeScript → Native C++)
- **Full project structure** following OpenClaw's plugin standards
- **Mock implementation** that demonstrates the API and can be tested immediately
- **Comprehensive documentation** including setup guides, examples, and implementation details
- **Build system** ready for native addon compilation
- **Configuration templates** for easy setup

### 🚧 What Needs Work

The implementation currently uses **mock/stub** code. To make it fully functional:

1. **Build the Elastos Carrier SDK** (C library)
2. **Update C++ wrapper** to use real Carrier API calls instead of stubs
3. **Link native addon** against Carrier libraries
4. **Test with actual Carrier network** and Beagle clients
5. **Integrate with OpenClaw** instance for end-to-end testing

## Quick Start

### For Developers

```bash
# Clone the repository
git clone https://github.com/0xli/openclaw-beagle-channel.git
cd openclaw-beagle-channel

# Run setup
./setup.sh

# Try the examples (uses mock implementation)
npm run build
node examples/basic-usage.js
```

### For Users (Future)

Once fully implemented:

```bash
# Install as OpenClaw extension
npm install -g @openclaw/beagle-channel

# Configure OpenClaw to use Beagle channel
openclaw configure channels.beagle.enabled=true

# Start OpenClaw
openclaw start

# Get your Carrier address and share with friends
openclaw channel beagle address
```

## Architecture

The project uses a **three-layer architecture**:

```
OpenClaw (TypeScript)
    ↕
Channel Plugin (TypeScript)  ← This repository
    ↕
Native Addon (C++ N-API)
    ↕
Elastos Carrier SDK (C Library)
    ↕
Carrier Network (P2P)
```

Each layer has clear responsibilities and well-defined interfaces.

## Documentation

- **[README.md](README.md)**: Full project documentation
- **[QUICKSTART.md](QUICKSTART.md)**: Get started in 5 minutes
- **[IMPLEMENTATION.md](IMPLEMENTATION.md)**: Detailed integration guide
- **[ARCHITECTURE.md](ARCHITECTURE.md)**: System design and data flow
- **[examples/](examples/)**: Working code examples

## Files Overview

```
openclaw-beagle-channel/
├── extension/                    # OpenClaw plugin layer
│   ├── index.ts                 # Plugin registration
│   ├── runtime.ts               # Channel runtime
│   └── openclaw.plugin.json     # Plugin metadata
├── src/
│   ├── addon/                   # Native C++ addon
│   │   ├── beagle_carrier.cc   # N-API bindings
│   │   ├── carrier_wrapper.cc  # C++ wrapper
│   │   └── carrier_wrapper.h   # Header
│   └── beagle-carrier.ts        # TypeScript wrapper
├── examples/                     # Usage examples
├── docs/                         # Documentation
└── config.example.yaml          # Configuration template
```

## Technology Stack

- **OpenClaw**: TypeScript-based AI assistant platform
- **Elastos Carrier**: C-based P2P networking library
- **Node-API**: C++ to JavaScript bridge
- **CMake**: Build system for Carrier SDK
- **node-gyp**: Native addon build tool

## Key Features

### Implemented (Mock)

✅ Plugin registration with OpenClaw
✅ Event-driven architecture
✅ Friend management API
✅ Message sending/receiving API
✅ Configuration system
✅ TypeScript type safety
✅ Example code

### Planned

📋 Real Carrier SDK integration
📋 Actual P2P communication
📋 Pairing and authentication
📋 Media support (images, files)
📋 Group messaging
📋 Advanced Carrier features

## How It Works

### Sending a Message

1. User sends message via OpenClaw interface
2. OpenClaw routes to Beagle channel plugin
3. Plugin calls TypeScript wrapper
4. TypeScript calls native addon
5. Native addon calls Carrier SDK
6. Carrier SDK sends over P2P network
7. Recipient's Carrier receives message
8. Message bubbles back up through the layers

### Receiving a Message

1. Carrier SDK receives message from network
2. Calls C++ callback
3. C++ wrapper triggers ThreadSafeFunction
4. JavaScript callback receives message
5. TypeScript wrapper emits event
6. Runtime processes message
7. Plugin forwards to OpenClaw
8. OpenClaw processes with AI and responds

## Why This Matters

### For OpenClaw Users

- **Privacy**: Communicate without central servers
- **Flexibility**: Choose your messaging platform
- **Control**: Self-hosted, decentralized infrastructure

### For Carrier Users

- **AI Integration**: Chat with AI agents via Beagle
- **Automation**: Build AI-powered chat bots
- **Innovation**: Explore new use cases for P2P networks

### For Developers

- **Learning**: See how to bridge TypeScript and C libraries
- **Template**: Use as a reference for other channels
- **Contribution**: Help build the decentralized web

## Next Steps

### To Complete Implementation

1. Follow **[IMPLEMENTATION.md](IMPLEMENTATION.md)** step-by-step
2. Build the Elastos Carrier SDK for your platform
3. Update the C++ wrapper with real Carrier API calls
4. Test with actual Carrier network
5. Integrate with your OpenClaw instance

### To Contribute

1. Fork the repository
2. Make improvements (tests, features, docs)
3. Submit a pull request
4. Help others in issues and discussions

## Support & Resources

- **Issues**: [GitHub Issues](https://github.com/0xli/openclaw-beagle-channel/issues)
- **OpenClaw**: [openclaw/openclaw](https://github.com/openclaw/openclaw)
- **Elastos Carrier**: [SDK Repository](https://github.com/0xli/Elastos.NET.Carrier.Native.SDK)

## License

MIT License - See [LICENSE](LICENSE) file

## Acknowledgments

- **OpenClaw Team**: For the amazing AI assistant platform
- **Elastos Foundation**: For the Carrier P2P network
- **Contributors**: Everyone who helps improve this project

---

**Ready to get started?** Check out [QUICKSTART.md](QUICKSTART.md)!

**Want to understand the design?** Read [ARCHITECTURE.md](ARCHITECTURE.md)!

**Need to integrate the SDK?** Follow [IMPLEMENTATION.md](IMPLEMENTATION.md)!

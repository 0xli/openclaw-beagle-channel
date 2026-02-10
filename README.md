# OpenClaw Beagle Channel

A channel plugin for [OpenClaw](https://github.com/openclaw/openclaw) that enables communication over the Elastos Carrier network, allowing Beagle chat clients and other Carrier-based applications to interact with OpenClaw agents.

## Overview

This plugin bridges OpenClaw with the Elastos Carrier peer-to-peer network, enabling:

- **Decentralized Communication**: Direct peer-to-peer messaging without central servers
- **Beagle Chat Integration**: Compatible with Beagle and other Elastos Carrier clients
- **Cross-Platform**: Works on Ubuntu, macOS, and other platforms supported by Elastos Carrier
- **Privacy-Focused**: End-to-end encrypted communication over the Carrier network

## Architecture

The plugin consists of three main components:

1. **Native Addon** (C++/N-API): Interfaces with the Elastos Carrier Native SDK
2. **TypeScript Wrapper**: Provides a clean Node.js API over the native addon
3. **OpenClaw Channel Plugin**: Integrates with OpenClaw's channel system

```
┌─────────────────────────────────────┐
│         OpenClaw Agent              │
└──────────────┬──────────────────────┘
               │
               ├─ Channel Plugin (TypeScript)
               │
               ├─ BeagleCarrier Wrapper (TypeScript)
               │
               ├─ Native Addon (C++ N-API)
               │
               └─ Elastos Carrier SDK (C)
                  │
                  └─ Elastos Carrier Network
```

## Prerequisites

### Ubuntu (Primary Target)

```bash
# Build essentials
sudo apt-get update
sudo apt-get install -f build-essential autoconf automake autopoint libtool flex bison libncurses5-dev cmake

# Node.js 22 or higher
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt-get install -y nodejs

# Development tools
sudo apt-get install git python3
```

### Elastos Carrier SDK

The Elastos Carrier Native SDK needs to be built and installed:

```bash
# Clone and build Elastos Carrier SDK
git clone https://github.com/0xli/Elastos.NET.Carrier.Native.SDK.git
cd Elastos.NET.Carrier.Native.SDK
mkdir -p build/linux && cd build/linux
cmake ../..
make
sudo make install
```

## Installation

### From Source

1. Clone this repository:
```bash
git clone https://github.com/0xli/openclaw-beagle-channel.git
cd openclaw-beagle-channel
```

2. Install dependencies:
```bash
npm install
```

3. Build the native addon:
```bash
npm run build
```

## Configuration

The Beagle channel can be configured through OpenClaw's configuration system or environment variables:

### Environment Variables

- `BEAGLE_DATA_DIR`: Directory for Carrier data storage (default: `./data/beagle`)
- `BEAGLE_BOOTSTRAP_NODES`: JSON array of bootstrap nodes (uses defaults if not set)
- `BEAGLE_USER_NAME`: Display name for the Carrier user (default: `OpenClaw Agent`)
- `BEAGLE_USER_DESC`: User description (default: `OpenClaw AI Assistant`)

### OpenClaw Config

Add to your OpenClaw configuration:

```yaml
channels:
  beagle:
    enabled: true
    dataDir: "./data/beagle"
    userName: "My OpenClaw Agent"
    userDescription: "Personal AI Assistant"
```

## Usage

### Getting Your Carrier Address

Once the channel is running, you can get your Carrier address to share with others:

```bash
# The address will be logged when the channel starts
# Look for: "Carrier Address: <your-address>"
```

### Adding Friends

To chat with the OpenClaw agent via Beagle:

1. Start your Beagle chat client
2. Add the OpenClaw agent as a friend using the Carrier address
3. Send a friend request with a greeting
4. Once accepted, you can start chatting with the agent

### Sending Messages

From Beagle chat or another Carrier client:
- Simply send messages to the OpenClaw agent's Carrier ID
- The agent will process your messages and respond

### Auto-Accept Friend Requests (Development)

For development and testing, the plugin currently auto-accepts friend requests. In production, you should implement proper pairing rules following OpenClaw's security model.

## Development

### Project Structure

```
openclaw-beagle-channel/
├── extension/              # OpenClaw plugin implementation
│   ├── index.ts           # Plugin entry point
│   ├── runtime.ts         # Channel runtime management
│   └── openclaw.plugin.json
├── src/
│   ├── addon/             # Native C++ addon
│   │   ├── beagle_carrier.cc
│   │   ├── carrier_wrapper.cc
│   │   └── carrier_wrapper.h
│   └── beagle-carrier.ts  # TypeScript wrapper
├── deps/                  # Elastos Carrier SDK dependencies
├── binding.gyp            # Native addon build configuration
├── package.json
└── tsconfig.json
```

### Building

```bash
# Build TypeScript
npm run build

# Rebuild native addon
npm run rebuild

# Clean build artifacts
npm run clean
```

### Testing

```bash
# Run tests (to be implemented)
npm test
```

## Implementation Status

### ✅ Completed

- [x] Project structure and build configuration
- [x] Native addon scaffolding (C++ N-API)
- [x] TypeScript wrapper for native addon
- [x] OpenClaw channel plugin structure
- [x] Runtime management and event handling
- [x] Basic message sending/receiving interface
- [x] Friend request handling
- [x] Documentation

### 🚧 In Progress

- [ ] Full Elastos Carrier SDK integration
- [ ] Message encryption and security
- [ ] Pairing and authentication system
- [ ] Media support (images, files)
- [ ] Group chat support

### 📋 Planned

- [ ] Comprehensive testing suite
- [ ] Performance optimization
- [ ] macOS and Windows support
- [ ] Advanced Carrier features (sessions, file transfer)
- [ ] UI for managing friends and connections

## Current Implementation Notes

The current implementation includes:

1. **Mock Native Addon**: The C++ addon currently uses stub implementations. To complete the integration, you need to:
   - Link against the Elastos Carrier SDK libraries
   - Implement actual Carrier API calls in `carrier_wrapper.cc`
   - Handle Carrier callbacks and events properly
   - Manage the Carrier event loop

2. **Bootstrap Nodes**: The default bootstrap nodes are placeholders. You should use actual Elastos Carrier bootstrap nodes from the network.

3. **Security**: Implement proper pairing rules, friend request approval, and message encryption according to your security requirements.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

### Development Workflow

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Elastos Carrier Resources

- [Elastos Carrier SDK](https://github.com/0xli/Elastos.NET.Carrier.Native.SDK)
- [API Documentation](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK/tree/master/docs)
- [Example Apps](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK/tree/master/apps)

## License

MIT License

## Acknowledgments

- [OpenClaw](https://github.com/openclaw/openclaw) - The AI assistant platform
- [Elastos Carrier](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK) - Decentralized P2P communication framework

## Support

For questions and support:
- Create an issue on GitHub
- Check the Elastos Carrier documentation
- Refer to OpenClaw channel development guides

# Beagle Chat (Elastos Carrier) Integration for OpenClaw

Complete setup guide for integrating Beagle Chat with OpenClaw.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [Part 1: Carrier Sidecar](#part-1-carrier-sidecar)
  - [Part 2: OpenClaw Plugin](#part-2-openclaw-plugin)
- [Configuration](#configuration)
- [Usage](#usage)
- [Security](#security)
- [Troubleshooting](#troubleshooting)

## Overview

The Beagle Chat integration enables OpenClaw agents to communicate via the Elastos Carrier protocol (Beagle Chat). This integration consists of two components:

1. **Carrier Sidecar** - A native C++ daemon that interfaces with the Elastos Carrier SDK
2. **OpenClaw Plugin** - A TypeScript module that connects OpenClaw to the sidecar

**Display Name:** Beagle Chat  
**Provider ID:** `beagle` or `carrier`  
**Channel Number:** #16 (following 15 existing chat providers)

## Architecture

```
┌─────────────────────┐
│   OpenClaw Agent    │
│    (TypeScript)     │
└──────────┬──────────┘
           │ HTTP/WebSocket
           │ (localhost:8765)
           ▼
┌─────────────────────┐
│  Carrier Sidecar    │
│      (C++)          │
└──────────┬──────────┘
           │ Carrier Protocol
           │
           ▼
┌─────────────────────┐
│   Beagle Chat       │
│   Network           │
└─────────────────────┘
```

**Why this architecture?**
- Keeps OpenClaw stable even if Carrier crashes/restarts
- Avoids complex Node.js native addons
- Carrier SDK (C/C++) is the most stable implementation
- Clear separation of concerns

## Prerequisites

### For Carrier Sidecar (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  libssl-dev \
  libsodium-dev \
  git \
  pkg-config
```

### For OpenClaw Plugin (Node.js)

- Node.js 18+ and npm
- OpenClaw installed and configured

## Installation

### Part 1: Carrier Sidecar

#### 1.1 Install Elastos Carrier SDK

```bash
# Clone the SDK
git clone https://github.com/elastos/Elastos.NET.Carrier.Native.SDK.git
cd Elastos.NET.Carrier.Native.SDK

# Build and install
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

Verify installation:
```bash
ldconfig -p | grep carrier
# Should show: libelacarrier.so
```

#### 1.2 Build the Sidecar

```bash
cd openclaw-beagle-channel/sidecar
mkdir build && cd build
cmake ..
make
sudo make install
```

This installs:
- `/usr/local/bin/beagle-sidecar` - The sidecar executable
- `/lib/systemd/system/beagle-sidecar.service` - systemd service file
- `/etc/beagle-sidecar/config.json` - Default configuration

#### 1.3 Create Sidecar User and Directories

```bash
# Create user
sudo useradd -r -s /bin/false beagle

# Create directories
sudo mkdir -p /var/lib/beagle-sidecar
sudo mkdir -p /var/log/beagle-sidecar

# Set permissions
sudo chown beagle:beagle /var/lib/beagle-sidecar
sudo chown beagle:beagle /var/log/beagle-sidecar
```

#### 1.4 Configure the Sidecar

Edit `/etc/beagle-sidecar/config.json`:

```json
{
  "port": 8765,
  "host": "127.0.0.1",
  "authToken": "YOUR-SECURE-RANDOM-TOKEN-HERE",
  "carrier": {
    "bootstrapNodes": [
      "13.58.208.50:33445",
      "18.217.147.205:33445"
    ],
    "dataDir": "/var/lib/beagle-sidecar"
  },
  "logLevel": "info"
}
```

**Important:** Generate a secure auth token:
```bash
openssl rand -hex 32
```

#### 1.5 Start the Sidecar

```bash
# Enable and start service
sudo systemctl enable beagle-sidecar
sudo systemctl start beagle-sidecar

# Check status
sudo systemctl status beagle-sidecar

# View logs
journalctl -u beagle-sidecar -f
```

Verify it's running:
```bash
curl http://localhost:8765/health
# Should return: {"status":"ok","version":"1.0.0",...}
```

### Part 2: OpenClaw Plugin

#### 2.1 Install the Plugin

```bash
npm install openclaw-beagle-channel
```

Or add to your `package.json`:
```json
{
  "dependencies": {
    "openclaw-beagle-channel": "^1.0.0"
  }
}
```

#### 2.2 Configure OpenClaw

Add to your OpenClaw configuration file (e.g., `openclaw.config.js`):

```javascript
import { BeagleChannelProvider } from 'openclaw-beagle-channel';

export default {
  channels: {
    beagle: {
      provider: new BeagleChannelProvider({
        sidecarUrl: 'http://localhost:8765',
        authToken: 'YOUR-SECURE-RANDOM-TOKEN-HERE', // Same as in sidecar config
        debug: false,
        reconnectInterval: 5000, // ms
      }),
      enabled: true,
    },
  },
};
```

#### 2.3 Initialize in Your Application

```typescript
import { BeagleChannelProvider } from 'openclaw-beagle-channel';

// Create provider
const beagleProvider = new BeagleChannelProvider({
  sidecarUrl: 'http://localhost:8765',
  authToken: process.env.BEAGLE_AUTH_TOKEN,
  debug: process.env.NODE_ENV === 'development',
});

// Initialize
await beagleProvider.initialize();

// Send a message
const result = await beagleProvider.sendText(
  'carrier-peer-id-here',
  'Hello from OpenClaw!'
);
console.log('Message sent:', result.messageId);

// Receive messages
beagleProvider.onIncomingMessage(async (message) => {
  console.log('Received from', message.peerId, ':', message.text);
  
  // Process message with OpenClaw agent
  // await processAgentMessage(message);
});
```

## Configuration

### Sidecar Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `port` | number | `8765` | HTTP server port |
| `host` | string | `"127.0.0.1"` | HTTP server host (keep localhost for security) |
| `authToken` | string | `""` | Bearer token for API authentication |
| `carrier.bootstrapNodes` | array | See config | Carrier network bootstrap nodes |
| `carrier.dataDir` | string | `/var/lib/beagle-sidecar` | Carrier data directory |
| `logLevel` | string | `"info"` | Log level: `debug`, `info`, `warn`, `error` |

### OpenClaw Plugin Configuration Options

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `sidecarUrl` | string | ✅ | Sidecar base URL (e.g., `http://localhost:8765`) |
| `authToken` | string | ❌ | Bearer token (must match sidecar config) |
| `debug` | boolean | ❌ | Enable debug logging |
| `reconnectInterval` | number | ❌ | WebSocket reconnect interval in ms (default: 5000) |

## Usage

### Sending Messages

```typescript
// Send to a specific peer
await beagleProvider.sendText('peer-carrier-id', 'Hello!');

// Get message metadata
const { messageId, timestamp } = await beagleProvider.sendText(
  'peer-carrier-id',
  'Important message'
);
console.log(`Sent message ${messageId} at ${timestamp}`);
```

### Receiving Messages

```typescript
beagleProvider.onIncomingMessage(async (message) => {
  console.log('New message:');
  console.log('  From:', message.peerId);
  console.log('  Text:', message.text);
  console.log('  ID:', message.messageId);
  console.log('  Time:', new Date(message.timestamp));
  
  // Optional: Send auto-reply
  if (message.text.includes('help')) {
    await beagleProvider.sendText(
      message.peerId,
      'I am an OpenClaw agent. How can I help you?'
    );
  }
});
```

### Connection Status

```typescript
// Check if connected
if (beagleProvider.isConnected()) {
  console.log('Connected to Beagle Chat');
} else {
  console.log('Not connected');
}
```

### Graceful Shutdown

```typescript
// Clean shutdown
process.on('SIGTERM', async () => {
  console.log('Shutting down...');
  await beagleProvider.shutdown();
  process.exit(0);
});
```

## Security

### 🔒 Security Best Practices

1. **Local-Only Binding**
   - Sidecar binds to `127.0.0.1` only (not `0.0.0.0`)
   - Not exposed to the network
   - Only accessible from localhost

2. **Authentication Token**
   - Always set a strong `authToken`
   - Generate with: `openssl rand -hex 32`
   - Keep token in environment variables, not in code
   - Rotate tokens periodically

3. **TLS/HTTPS** (Optional but recommended)
   - For production, consider setting up local TLS
   - Use self-signed certificates for localhost
   - Prevents potential local MITM attacks

4. **File Permissions**
   - Config file: `sudo chmod 600 /etc/beagle-sidecar/config.json`
   - Data directory: Owned by `beagle` user only
   - Log files: Restrict access

5. **Firewall Rules**
   - Carrier uses UDP (typically port 33445)
   - Allow outbound UDP to bootstrap nodes
   - Block inbound if not running a bootstrap node

6. **systemd Security**
   - Service runs as dedicated `beagle` user (not root)
   - Limited file system access via `ProtectSystem`
   - Temporary files isolated via `PrivateTmp`

### Security Checklist

- [ ] Strong auth token generated and configured
- [ ] Config file permissions set to 600
- [ ] Sidecar running as non-root user
- [ ] Firewall allows Carrier protocol (UDP)
- [ ] Logs monitored for suspicious activity
- [ ] Token stored in environment variables
- [ ] Regular security updates applied

## Troubleshooting

### Sidecar Issues

**Problem: Sidecar won't start**

```bash
# Check service status
sudo systemctl status beagle-sidecar

# View logs
journalctl -u beagle-sidecar -n 50

# Check if Carrier SDK is installed
ldconfig -p | grep carrier

# Verify config file syntax
cat /etc/beagle-sidecar/config.json | jq
```

**Problem: Can't connect to Carrier network**

```bash
# Test bootstrap node connectivity
nc -zvu 13.58.208.50 33445

# Check firewall
sudo ufw status
sudo iptables -L

# Verify data directory permissions
ls -la /var/lib/beagle-sidecar
```

**Problem: Health check fails**

```bash
# Test local connection
curl http://localhost:8765/health

# Check if port is in use
sudo netstat -tlnp | grep 8765

# Try without auth token
curl -v http://localhost:8765/health
```

### OpenClaw Plugin Issues

**Problem: Can't connect to sidecar**

```javascript
// Check sidecar URL
const config = {
  sidecarUrl: 'http://localhost:8765', // Check this
  debug: true, // Enable debug logs
};

// Test health endpoint
const axios = require('axios');
const health = await axios.get('http://localhost:8765/health');
console.log(health.data);
```

**Problem: Authentication failures**

```javascript
// Verify token matches
console.log('Plugin token:', config.authToken);
// Compare with: cat /etc/beagle-sidecar/config.json | jq .authToken

// Test with curl
curl -H "Authorization: Bearer YOUR-TOKEN" http://localhost:8765/health
```

**Problem: Messages not sending**

```javascript
// Enable debug mode
const provider = new BeagleChannelProvider({
  sidecarUrl: 'http://localhost:8765',
  authToken: process.env.BEAGLE_AUTH_TOKEN,
  debug: true, // See detailed logs
});

// Check sidecar logs while sending
// In another terminal:
journalctl -u beagle-sidecar -f
```

**Problem: WebSocket disconnects**

```javascript
// Increase reconnect interval
const config = {
  reconnectInterval: 10000, // 10 seconds instead of 5
};

// Monitor connection status
setInterval(() => {
  console.log('Connected:', provider.isConnected());
}, 5000);
```

### Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `ECONNREFUSED` | Sidecar not running | Start sidecar: `sudo systemctl start beagle-sidecar` |
| `401 Unauthorized` | Wrong auth token | Verify token matches in both configs |
| `Cannot read property 'sendMessage'` | Not initialized | Call `await provider.initialize()` first |
| `Carrier not connected` | Carrier network issue | Check bootstrap nodes, firewall, logs |
| `EADDRINUSE` | Port already in use | Change port or stop conflicting service |

### Getting Help

If you continue to have issues:

1. **Enable debug logging** in both sidecar and plugin
2. **Collect logs**:
   ```bash
   journalctl -u beagle-sidecar -n 100 > sidecar.log
   ```
3. **Check system resources**:
   ```bash
   df -h  # Disk space
   free -h  # Memory
   ```
4. **File an issue** on GitHub with:
   - OS version
   - Carrier SDK version
   - Plugin version
   - Full error messages
   - Relevant log excerpts

## Next Steps

- ✅ **Integration complete!** Your OpenClaw agent can now use Beagle Chat
- 📖 Read the [API documentation](./API.md) for advanced usage
- 🔧 Explore [example configurations](../examples/)
- 🚀 Deploy to production following the [deployment guide](./DEPLOYMENT.md)
- 📊 Set up [monitoring and metrics](./MONITORING.md)

## License

MIT License - See [LICENSE](../LICENSE) file for details.

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

---

**Channel #16** • Beagle Chat (Elastos Carrier) • OpenClaw Integration

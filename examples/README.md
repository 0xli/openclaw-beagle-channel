# Beagle Chat Integration Examples

This directory contains example implementations for using the Beagle Chat channel with OpenClaw.

## Examples

### 1. Basic Usage (`basic-usage.ts`)

Simple example demonstrating:
- Connecting to the Beagle Chat sidecar
- Sending messages
- Receiving messages
- Auto-reply functionality

**Run:**
```bash
cd examples
npm install
export BEAGLE_AUTH_TOKEN="your-token-here"
export TEST_PEER_ID="optional-peer-id-to-send-test-message"
npm run basic
```

### 2. OpenClaw Agent (`openclaw-agent-example.ts`)

Advanced example showing:
- Full OpenClaw agent integration
- Conversation history management
- Rule-based message processing
- Multi-user support
- Graceful shutdown

**Run:**
```bash
cd examples
npm install
export BEAGLE_AUTH_TOKEN="your-token-here"
npm run agent
```

## Configuration

All examples support these environment variables:

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `SIDECAR_URL` | No | `http://localhost:8765` | Sidecar API base URL |
| `BEAGLE_AUTH_TOKEN` | Yes* | - | Authentication token (*if configured in sidecar) |
| `TEST_PEER_ID` | No | - | Peer ID for sending test messages |

## Quick Start

1. **Start the sidecar** (see main [SETUP.md](../docs/SETUP.md)):
   ```bash
   sudo systemctl start beagle-sidecar
   ```

2. **Install dependencies**:
   ```bash
   cd examples
   npm install
   ```

3. **Set environment variables**:
   ```bash
   export BEAGLE_AUTH_TOKEN="your-secure-token"
   ```

4. **Run an example**:
   ```bash
   npm run basic
   # or
   npm run agent
   ```

## Creating Your Own Integration

Use these examples as templates:

```typescript
import { BeagleChannelProvider } from 'openclaw-beagle-channel';

// 1. Create provider
const provider = new BeagleChannelProvider({
  sidecarUrl: 'http://localhost:8765',
  authToken: process.env.BEAGLE_AUTH_TOKEN,
  debug: true,
});

// 2. Initialize
await provider.initialize();

// 3. Handle incoming messages
provider.onIncomingMessage(async (message) => {
  console.log('Received:', message.text);
  // Your logic here
});

// 4. Send messages
await provider.sendText('peer-id', 'Hello!');

// 5. Clean shutdown
await provider.shutdown();
```

## Testing Tips

### Test Message Flow

Terminal 1 - Run agent:
```bash
npm run agent
```

Terminal 2 - Send test message via API:
```bash
curl -X POST http://localhost:8765/send \
  -H "Authorization: Bearer your-token" \
  -H "Content-Type: application/json" \
  -d '{
    "peerId": "test-peer-123",
    "text": "Hello agent!"
  }'
```

### Monitor Logs

```bash
# Sidecar logs
journalctl -u beagle-sidecar -f

# Agent logs
# Displayed in the terminal running the agent
```

## Next Steps

- Read the [Setup Guide](../docs/SETUP.md) for deployment
- Check the [API Documentation](../docs/API.md) for advanced features
- Explore production configurations in [../docs/](../docs/)

## Troubleshooting

**Connection refused:**
- Ensure sidecar is running: `systemctl status beagle-sidecar`
- Check sidecar URL matches in config

**401 Unauthorized:**
- Verify auth token matches sidecar config
- Check: `cat /etc/beagle-sidecar/config.json | jq .authToken`

**Messages not received:**
- Enable debug mode: Set `debug: true` in provider config
- Check sidecar logs: `journalctl -u beagle-sidecar -f`
- Verify WebSocket connection is established

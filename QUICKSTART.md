# Quick Start Guide

Get up and running with the OpenClaw Beagle Channel plugin in 5 minutes.

## Prerequisites

- Node.js 18+ installed
- npm or yarn package manager

## Installation

### 1. Install the Plugin

```bash
npm install openclaw-beagle-channel
```

Or clone the repository:

```bash
git clone https://github.com/0xli/openclaw-beagle-channel.git
cd openclaw-beagle-channel
npm install
npm run build
```

### 2. Install and Start the Sidecar

```bash
cd sidecar
npm install
npm run build
npm start
```

The sidecar will start on `http://localhost:8080`.

## Basic Usage

### Send a Text Message

```javascript
const { BeagleChannelPlugin } = require('openclaw-beagle-channel');

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

await plugin.sendText({
  channelId: 'beagle',
  peer: 'user-id',
  text: 'Hello from OpenClaw!'
});
```

### Send an Image

```javascript
const { BeagleChannelPlugin, MediaType } = require('openclaw-beagle-channel');

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

await plugin.sendMedia({
  channelId: 'beagle',
  peer: 'user-id',
  mediaPath: '/path/to/image.jpg',
  mediaType: MediaType.Image,
  text: 'Check this out!'
});
```

### Receive Messages

```javascript
const { BeagleChannelPlugin } = require('openclaw-beagle-channel');

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

plugin.onMessage((message) => {
  console.log('Received from:', message.peer);
  console.log('Text:', message.text);
  
  if (message.mediaType) {
    console.log('Media type:', message.mediaType);
    console.log('Media URL:', message.mediaUrl || message.mediaPath);
  }
});

// Keep running
await new Promise(() => {});
```

## Test the Implementation

### 1. Start the Sidecar

Terminal 1:
```bash
cd sidecar
npm run dev
```

### 2. Run the Test Suite

Terminal 2:
```bash
npm test
```

You should see:
```
=== Testing Beagle Channel Plugin ===
✓ Plugin created
✓ Plugin initialized
✓ Text message sent
✓ Media message sent (path)
✓ Media message sent (URL)
=== All tests passed! ===
```

### 3. Manual Testing with curl

```bash
# Send text message
curl -X POST http://localhost:8080/sendText \
  -H "Content-Type: application/json" \
  -d '{"peer": "test-user", "text": "Hello"}'

# Send media message
curl -X POST http://localhost:8080/sendMedia \
  -H "Content-Type: application/json" \
  -d '{
    "peer": "test-user",
    "path": "/tmp/test.jpg",
    "caption": "Test image"
  }'

# Simulate inbound message
curl -X POST http://localhost:8080/simulate-inbound \
  -H "Content-Type: application/json" \
  -d '{
    "peer": "sender",
    "text": "Inbound message",
    "mediaUrl": "https://example.com/image.jpg",
    "mediaType": "image"
  }'
```

## OpenClaw CLI Integration

Once configured in OpenClaw:

```bash
# Send text
openclaw message send --channel beagle --to user-id --text "Hello!"

# Send image
openclaw message send --channel beagle --to user-id \
  --media /path/to/image.jpg \
  --message "Check this out"

# Send document
openclaw message send --channel beagle --to user-id \
  --media /path/to/document.pdf
```

## Next Steps

1. **Integrate with Carrier SDK**: Replace the reference sidecar implementation with actual Carrier SDK calls
2. **Add file storage**: Implement file upload/download for the URL-based MVP
3. **Configure authentication**: Add auth to the sidecar endpoints
4. **Deploy to production**: Deploy the sidecar to your Ubuntu server

## Troubleshooting

### Sidecar won't start
- Check that port 8080 is available: `lsof -i :8080`
- Try a different port: `PORT=8081 npm start`

### Plugin can't connect
- Verify sidecar is running: `curl http://localhost:8080/sendText`
- Check the sidecar URL in plugin constructor

### TypeScript errors
- Run `npm install` in both root and sidecar directories
- Rebuild: `npm run build`

## Documentation

- [README.md](README.md) - Full documentation
- [API.md](API.md) - API reference
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Implementation details
- [sidecar/README.md](sidecar/README.md) - Sidecar guide

## Support

For issues or questions, please open an issue on GitHub.

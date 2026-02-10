# Quick Start Guide

Get the Beagle Chat plugin running in 5 minutes.

## 🚀 Fast Track (Development)

### 1. Clone and Install
```bash
git clone https://github.com/0xli/openclaw-beagle-channel.git
cd openclaw-beagle-channel
npm install
npm run build
```

### 2. Start Mock Sidecar
```bash
# Terminal 1
node examples/mock-sidecar.js
```

### 3. Test the Plugin
```bash
# Terminal 2
node examples/integration-test.js
```

Expected output:
```
✅ All tests passed! The plugin is working correctly.
```

## 📦 Install from NPM (when published)

```bash
npm install openclaw-channel-beagle
```

## ⚙️ Configure OpenClaw

Create or edit your OpenClaw config file:

```yaml
# ~/.openclaw/config.yaml or /etc/openclaw/config.yaml
channels:
  beagle:
    accounts:
      default:
        sidecarBaseUrl: "http://127.0.0.1:39091"
        authToken: "your-secret-token"
        pollInterval: 5000
```

## 💬 Send Messages

### Text Message
```bash
openclaw message send \
  --channel beagle \
  --to "user@beagle" \
  --text "Hello from OpenClaw!"
```

### Media Message
```bash
openclaw message send \
  --channel beagle \
  --to "user@beagle" \
  --media "https://example.com/image.jpg"
```

## 🔧 Development Setup

### With Mock Sidecar
Perfect for testing without a real Beagle Carrier network:

```bash
# Start mock sidecar
AUTH_TOKEN=test-token node examples/mock-sidecar.js

# Test sending text
curl -X POST http://127.0.0.1:39091/sendText \
  -H "Content-Type: application/json" \
  -d '{"to":"user@beagle","text":"test","authToken":"test-token"}'

# Test receiving messages
curl "http://127.0.0.1:39091/events?authToken=test-token"
```

### Run Integration Tests
```bash
npm run build
node examples/integration-test.js
```

## 🐧 Production Deployment (Ubuntu)

### 1. Install Node.js and Plugin
```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs
sudo npm install -g openclaw-channel-beagle
```

### 2. Install Sidecar Service
```bash
# Copy your sidecar binary
sudo mkdir -p /opt/beagle-sidecar
sudo cp beagle-carrier-sidecar /opt/beagle-sidecar/
sudo chmod +x /opt/beagle-sidecar/beagle-carrier-sidecar

# Install systemd service
sudo cp beagle-carrier-sidecar.service /etc/systemd/system/
sudo nano /etc/systemd/system/beagle-carrier-sidecar.service
# Update AUTH_TOKEN

# Start service
sudo systemctl daemon-reload
sudo systemctl enable beagle-carrier-sidecar
sudo systemctl start beagle-carrier-sidecar
```

### 3. Verify
```bash
# Check sidecar status
sudo systemctl status beagle-carrier-sidecar

# Test connection
curl http://127.0.0.1:39091/health
```

## 📚 Next Steps

- Read [README.md](README.md) for complete documentation
- See [DEPLOYMENT.md](DEPLOYMENT.md) for detailed deployment guide
- Check [examples/usage.js](examples/usage.js) for code examples
- Review [CONTRIBUTING.md](CONTRIBUTING.md) to contribute

## ⚡ Common Issues

### Sidecar Connection Failed
```bash
# Check if sidecar is running
curl http://127.0.0.1:39091/health

# View sidecar logs
sudo journalctl -u beagle-carrier-sidecar -f
```

### Port Already in Use
```bash
# Find what's using port 39091
sudo netstat -tlnp | grep 39091

# Or use a different port
PORT=39092 node examples/mock-sidecar.js
```

### Authentication Failed
Ensure `authToken` matches in both:
- Plugin config (`config.yaml`)
- Sidecar environment (`/etc/systemd/system/beagle-carrier-sidecar.service`)

## 🆘 Get Help

- [GitHub Issues](https://github.com/0xli/openclaw-beagle-channel/issues)
- [Full Documentation](README.md)
- [Deployment Guide](DEPLOYMENT.md)

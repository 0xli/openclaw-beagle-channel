#!/bin/bash
# Setup script for OpenClaw Beagle Channel

set -e

echo "========================================="
echo "OpenClaw Beagle Channel Setup"
echo "========================================="
echo ""

# Check if running on Ubuntu/Debian
if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [[ "$ID" != "ubuntu" && "$ID" != "debian" ]]; then
        echo "Warning: This script is designed for Ubuntu/Debian. Your OS: $ID"
        read -p "Continue anyway? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
fi

# Check for required tools
echo "Checking for required tools..."

# Check Node.js
if ! command -v node &> /dev/null; then
    echo "Error: Node.js is not installed"
    echo "Please install Node.js 22 or higher:"
    echo "  curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -"
    echo "  sudo apt-get install -y nodejs"
    exit 1
fi

NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
if [ "$NODE_VERSION" -lt 22 ]; then
    echo "Error: Node.js version must be 22 or higher (current: $(node -v))"
    exit 1
fi

echo "✓ Node.js $(node -v)"

# Check npm
if ! command -v npm &> /dev/null; then
    echo "Error: npm is not installed"
    exit 1
fi
echo "✓ npm $(npm -v)"

# Check build tools
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ is not installed"
    echo "Please install build essentials:"
    echo "  sudo apt-get install -f build-essential"
    exit 1
fi
echo "✓ g++ ($(g++ --version | head -n1))"

# Check CMake
if ! command -v cmake &> /dev/null; then
    echo "Warning: CMake is not installed"
    echo "CMake is required to build Elastos Carrier SDK"
    echo "Install with: sudo apt-get install cmake"
fi

echo ""
echo "========================================="
echo "Installing Node.js Dependencies"
echo "========================================="
npm install

echo ""
echo "========================================="
echo "Building TypeScript"
echo "========================================="
npx tsc

echo ""
echo "========================================="
echo "Elastos Carrier SDK Setup"
echo "========================================="
echo ""
echo "The Beagle channel requires the Elastos Carrier Native SDK."
echo "Current status: Mock implementation (SDK not linked)"
echo ""
echo "To complete the integration:"
echo "1. Build the Elastos Carrier SDK:"
echo "   git clone https://github.com/0xli/Elastos.NET.Carrier.Native.SDK.git"
echo "   cd Elastos.NET.Carrier.Native.SDK"
echo "   mkdir -p build/linux && cd build/linux"
echo "   cmake ../.."
echo "   make"
echo "   sudo make install"
echo ""
echo "2. Copy SDK files to deps directory:"
echo "   mkdir -p deps/elastos-carrier/{include,lib}"
echo "   cp -r /path/to/sdk/include/* deps/elastos-carrier/include/"
echo "   cp -r /path/to/sdk/lib/* deps/elastos-carrier/lib/"
echo ""
echo "3. Rebuild the native addon:"
echo "   npm run build"
echo ""

# Try to build the native addon (it may fail if SDK is not available)
echo "========================================="
echo "Building Native Addon (this may fail)"
echo "========================================="
if npm run prebuild && npm run build 2>/dev/null; then
    echo "✓ Native addon built successfully"
else
    echo "⚠ Native addon build failed (expected if SDK not installed)"
    echo "  The TypeScript code will work with mock implementation"
fi

echo ""
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Review the README.md for configuration options"
echo "2. Set up the Elastos Carrier SDK (see above)"
echo "3. Configure your OpenClaw instance to use this channel"
echo ""
echo "For testing without the SDK:"
echo "  The current implementation uses a mock carrier that logs"
echo "  messages but doesn't actually communicate over the network."
echo ""

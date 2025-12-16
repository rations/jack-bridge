#!/bin/bash
# build-qjackctl-corrected.sh
# Build custom qjackctl with SYSTEM D-Bus support for jack-bridge
# Fixed version that properly handles Qt5 Linguist Tools and dependencies
set -e

echo "Building custom qjackctl for jack-bridge..."

# Check for required packages (blocking version - we need these to succeed)
echo "Checking build dependencies..."
REQUIRED_PKGS="cmake g++ qtbase5-dev qttools5-dev-tools libjack-jackd2-dev libasound2-dev qttools5-dev-tools"
MISSING_PKGS=""

for pkg in $REQUIRED_PKGS; do
    if ! dpkg -l | grep -q "^ii  $pkg"; then
        echo "ERROR: Missing required package: $pkg"
        MISSING_PKGS="$MISSING_PKGS $pkg"
    fi
done

if [ -n "$MISSING_PKGS" ]; then
    echo "Missing required packages: $MISSING_PKGS"
    echo "Install them with: sudo apt install -y $MISSING_PKGS"
    exit 1
fi

# Navigate to qjackctl source
cd qjackctl-1.0.4 || {
    echo "ERROR: qjackctl-1.0.4 directory not found!"
    exit 1
}

# Create patch marker to avoid re-patching
PATCH_MARKER=".jack-bridge-patched"
if [ ! -f "$PATCH_MARKER" ]; then
    echo "Applying jack-bridge SYSTEM bus patch..."
    
    # Patch line 2425: sessionBus() → systemBus()
    sed -i '2425s/QDBusConnection::sessionBus()/QDBusConnection::systemBus()/' \
        src/qjackctlMainForm.cpp
    
    # Verify patch applied
    if grep -n "QDBusConnection::systemBus()" src/qjackctlMainForm.cpp | grep -q "^2425:"; then
        echo "  ✓ Patch applied successfully (line 2425)"
        touch "$PATCH_MARKER"
    else
        echo "  ✗ Patch failed! Manual intervention required."
        exit 1
    fi
else
    echo "Source already patched (found $PATCH_MARKER)"
fi

# Clean previous build
echo "Cleaning previous build..."
rm -rf build

# Configure with CMake - use a more robust configuration that handles Qt5 Linguist Tools properly
echo "Configuring qjackctl with robust Qt5 configuration..."
cmake -B build \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DCONFIG_DBUS=ON \
    -DCONFIG_JACK_MIDI=ON \
    -DCONFIG_ALSA_SEQ=ON \
    -DCONFIG_JACK_PORT_ALIASES=ON \
    -DCONFIG_JACK_METADATA=ON \
    -DCONFIG_JACK_SESSION=ON \
    -DCONFIG_SYSTEM_TRAY=ON \
    -DCONFIG_PORTAUDIO=OFF  # Disable PortAudio to avoid warnings and simplify build

echo "Build configuration complete. Starting build..."

# Build
cmake --build build --parallel $(nproc)

echo "Build completed successfully!"

# Install binary to contrib/bin
echo "Installing to contrib/bin..."
mkdir -p ../contrib/bin
cp build/src/qjackctl ../contrib/bin/qjackctl
chmod 755 ../contrib/bin/qjackctl

echo ""
echo "✓ Custom qjackctl built successfully!"
echo "Binary: contrib/bin/qjackctl"
echo "Size: $(du -h ../contrib/bin/qjackctl | cut -f1)"
echo ""
echo "The binary has been patched to use SYSTEM D-Bus instead of SESSION D-Bus"
echo "This allows qjackctl to connect to jack-bridge-dbus service running on SYSTEM bus"
echo ""
echo "Run './contrib/install.sh' to install jack-bridge with custom qjackctl"
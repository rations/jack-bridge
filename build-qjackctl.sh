#!/bin/bash
# build-qjackctl.sh
# Build custom qjackctl with SYSTEM D-Bus support for jack-bridge
set -e

echo "Building custom qjackctl for jack-bridge..."

# Check for required packages
REQUIRED_PKGS="cmake g++ qtbase5-dev qttools5-dev-tools libjack-jackd2-dev libasound2-dev"
echo "Checking build dependencies..."
for pkg in $REQUIRED_PKGS; do
    if ! dpkg -l | grep -q "^ii  $pkg "; then
        echo "ERROR: Missing package: $pkg"
        echo "Install with: sudo apt install $REQUIRED_PKGS"
        exit 1
    fi
done

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

# Configure with CMake
echo "Configuring qjackctl..."
cmake -B build \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DCONFIG_DBUS=ON \
    -DCONFIG_JACK_MIDI=ON \
    -DCONFIG_ALSA_SEQ=ON \
    -DCONFIG_JACK_PORT_ALIASES=ON \
    -DCONFIG_JACK_METADATA=ON \
    -DCONFIG_JACK_SESSION=ON \
    -DCONFIG_SYSTEM_TRAY=ON

# Build
echo "Building qjackctl (this may take a few minutes)..."
cmake --build build --parallel $(nproc)

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
echo "Run 'sudo ./contrib/install.sh' to install jack-bridge with custom qjackctl"

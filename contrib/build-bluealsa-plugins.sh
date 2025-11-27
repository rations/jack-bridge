#!/bin/bash
# Build matching BlueALSA ALSA plugins for jack-bridge
# This script builds libasound_module_pcm_bluealsa.so and libasound_module_ctl_bluealsa.so
# from the same commit as your prebuilt bluealsad daemon to ensure compatibility.
#
# Usage: sh contrib/build-bluealsa-plugins.sh
# Output: Plugins will be copied to contrib/bin/ ready for installer

set -e

echo "=== BlueALSA ALSA Plugins Build Script ==="
echo

# Detect daemon version
DAEMON_PATH="/usr/local/bin/bluealsad"
if [ ! -x "$DAEMON_PATH" ]; then
    DAEMON_PATH="contrib/bin/bluealsad"
fi

if [ -x "$DAEMON_PATH" ]; then
    DAEMON_VERSION=$("$DAEMON_PATH" --version 2>/dev/null || echo "unknown")
    echo "Detected daemon version: $DAEMON_VERSION"
    COMMIT="$DAEMON_VERSION"
else
    echo "Warning: bluealsad not found; using default commit b0dd89b"
    COMMIT="b0dd89b"
fi

if [ "$COMMIT" = "unknown" ] || [ -z "$COMMIT" ]; then
    echo "Could not detect daemon version. Please provide commit hash:"
    printf "Enter BlueALSA commit (e.g., b0dd89b): "
    read COMMIT
    if [ -z "$COMMIT" ]; then
        echo "Error: No commit provided"
        exit 1
    fi
fi

echo
echo "Building ALSA plugins from commit: $COMMIT"
echo

# Check dependencies
echo "Step 1: Checking build dependencies..."
MISSING=""
for pkg in git automake libtoolize pkg-config make gcc; do
    if ! command -v $pkg >/dev/null 2>&1; then
        MISSING="$MISSING $pkg"
    fi
done

if [ -n "$MISSING" ]; then
    echo "Error: Missing build tools:$MISSING"
    echo "Install them with:"
    echo "  sudo apt install -y git automake libtool pkg-config build-essential"
    exit 1
fi

echo "  ✓ Build tools available"
echo

# Check library dependencies
echo "Step 2: Checking library dependencies..."
echo "The following packages are required. Checking..."

REQUIRED_LIBS="libbluetooth-dev libdbus-1-dev libasound2-dev libglib2.0-dev libsbc-dev"
OPTIONAL_LIBS="libfdk-aac-dev libldacbt-abr-dev libldacbt-enc-dev liblc3-dev"

MISSING_LIBS=""
for lib in $REQUIRED_LIBS; do
    if ! dpkg -l | grep -q "^ii  $lib"; then
        MISSING_LIBS="$MISSING_LIBS $lib"
    fi
done

if [ -n "$MISSING_LIBS" ]; then
    echo "Required libraries missing:$MISSING_LIBS"
    echo "Installing..."
    sudo apt update
    sudo apt install -y $REQUIRED_LIBS || {
        echo "Error: Failed to install required libraries"
        exit 1
    }
fi

echo "  ✓ Required libraries available"

# Optional codecs
echo
echo "Checking optional codec libraries (AAC, aptX, LDAC, LC3)..."
for lib in $OPTIONAL_LIBS; do
    if dpkg -l | grep -q "^ii  $lib"; then
        echo "  ✓ $lib available"
    else
        echo "  ! $lib not available (optional - higher quality codecs)"
    fi
done
echo

# Clone BlueALSA
BUILD_DIR="/tmp/bluealsa-build-$$"
echo "Step 3: Cloning BlueALSA repository..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if ! git clone https://github.com/Arkq/bluez-alsa.git; then
    echo "Error: Failed to clone BlueALSA repository"
    exit 1
fi

cd bluez-alsa
echo "  ✓ Cloned BlueALSA"
echo

# Checkout specific commit
echo "Step 4: Checking out commit $COMMIT..."
if ! git checkout "$COMMIT"; then
    echo "Error: Failed to checkout commit $COMMIT"
    echo "Verify the commit hash is correct."
    exit 1
fi
echo "  ✓ Checked out $COMMIT"
echo

# Configure
echo "Step 5: Configuring build..."
echo "Running autoreconf..."
autoreconf --install --force || {
    echo "Error: autoreconf failed"
    exit 1
}

echo "Running ./configure with ALSA plugin support..."
echo

# Temporarily disable exit-on-error so we can handle configure failure gracefully
set +e

# Try configure with all available codecs first
# Save output to file and check exit code
./configure \
    --enable-aplay \
    --enable-rfcomm \
    --enable-aac \
    --enable-aptx \
    --enable-ldac \
    --enable-lc3-swb \
    >configure.log 2>&1
CONFIGURE_STATUS=$?

# Re-enable exit-on-error
set -e

# Show configure output
cat configure.log

if [ $CONFIGURE_STATUS -eq 0 ]; then
    # Configure succeeded
    echo
    echo "  ✓ Configuration complete with codec support"
    echo
else
    # Configure failed - likely due to missing optional codecs
    echo
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Configure failed with optional codec support."
    echo "This is NORMAL - optional codec libraries (AAC, aptX, LDAC, LC3) are missing."
    echo "Basic SBC codec will work fine for most Bluetooth devices."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo
    printf "Retry with minimal configuration (SBC only)? [Y/n] "
    read ans
    if [ "$ans" = "n" ] || [ "$ans" = "N" ]; then
        echo "Aborted by user"
        exit 1
    fi
    
    echo
    echo "Configuring with minimal options (SBC codec only)..."
    
    # Disable exit-on-error for this configure attempt too
    set +e
    ./configure --enable-aplay --enable-rfcomm >configure-minimal.log 2>&1
    MINIMAL_STATUS=$?
    set -e
    
    cat configure-minimal.log
    
    if [ $MINIMAL_STATUS -ne 0 ]; then
        echo
        echo "Error: Minimal configure also failed!"
        echo "This usually means missing REQUIRED libraries:"
        echo "  - libbluetooth-dev"
        echo "  - libdbus-1-dev"
        echo "  - libasound2-dev"
        echo "  - libglib2.0-dev"
        echo "  - libsbc-dev"
        echo
        echo "Check configure-minimal.log for details"
        exit 1
    fi
    echo
    echo "  ✓ Configuration complete with SBC codec"
    echo
fi

# Build
echo "Step 6: Building BlueALSA (this may take 2-5 minutes)..."
if ! make -j$(nproc) 2>&1 | tee build.log; then
    echo "Error: Build failed. Check build.log above."
    exit 1
fi
echo "  ✓ Build complete"
echo

# Verify plugins were built
echo "Step 7: Verifying built plugins..."
PCM_PLUGIN="src/asound/.libs/libasound_module_pcm_bluealsa.so"
CTL_PLUGIN="src/asound/.libs/libasound_module_ctl_bluealsa.so"

if [ ! -f "$PCM_PLUGIN" ]; then
    echo "Error: PCM plugin not found at $PCM_PLUGIN"
    echo "Build may have failed. Check build.log"
    exit 1
fi
echo "  ✓ PCM plugin built: $PCM_PLUGIN"
ls -lh "$PCM_PLUGIN"

if [ ! -f "$CTL_PLUGIN" ]; then
    echo "  ! CTL plugin not found (optional)"
else
    echo "  ✓ CTL plugin built: $CTL_PLUGIN"
    ls -lh "$CTL_PLUGIN"
fi
echo

# Copy to repo
REPO_DIR="$HOME/build_a_bridge/jack-bridge"
if [ ! -d "$REPO_DIR" ]; then
    echo "Repository not found at $REPO_DIR"
    printf "Enter path to jack-bridge repository: "
    read REPO_DIR
    if [ ! -d "$REPO_DIR" ]; then
        echo "Error: Directory $REPO_DIR does not exist"
        exit 1
    fi
fi

echo "Step 8: Copying plugins to repository..."
mkdir -p "$REPO_DIR/contrib/bin"

cp -v "$PCM_PLUGIN" "$REPO_DIR/contrib/bin/"
if [ -f "$CTL_PLUGIN" ]; then
    cp -v "$CTL_PLUGIN" "$REPO_DIR/contrib/bin/"
fi

echo "  ✓ Plugins copied to $REPO_DIR/contrib/bin/"
echo

# Verify in repo
echo "Step 9: Verifying plugins in repository..."
cd "$REPO_DIR"
ls -lh contrib/bin/libasound_module_*.so
echo

# Cleanup
echo "Step 10: Cleanup build directory..."
echo "Build directory: $BUILD_DIR"
printf "Remove build directory? [Y/n] "
read ans
if [ "$ans" != "n" ] && [ "$ans" != "N" ]; then
    rm -rf "$BUILD_DIR"
    echo "  ✓ Removed $BUILD_DIR"
else
    echo "  Build directory kept at: $BUILD_DIR"
fi
echo

# Final instructions
echo "=== Build Complete! ==="
echo
echo "Plugins have been added to contrib/bin/:"
ls -1 contrib/bin/libasound_module_*.so | sed 's/^/  • /'
echo
echo "Next steps:"
echo "  1. Commit plugins to repository:"
echo "     cd $REPO_DIR"
echo "     git add contrib/bin/libasound_module_*.so"
echo "     git commit -m \"Add matching ALSA plugins for BlueALSA daemon $COMMIT\""
echo
echo "  2. Re-run installer to install matching plugins:"
echo "     sudo ./contrib/install.sh"
echo
echo "  3. Reboot to activate persistent ports:"
echo "     sudo reboot"
echo
echo "  4. Test Bluetooth audio with GUI"
echo
echo "For testing, run:"
echo "  sh contrib/test-bluetooth.sh MAC_ADDRESS"
echo

exit 0
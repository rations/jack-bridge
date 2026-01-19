#!/bin/bash
# create-release-tarball.sh
# Create a clean distribution tarball for jack-bridge excluding development files
#
# Usage: ./create-release-tarball.sh [version]
# Example: ./create-release-tarball.sh 1.0.0

set -e

# Get version from argument or use current date
VERSION="${1:-$(date +%Y%m%d)}"
TARBALL_NAME="jack-bridge-${VERSION}.tar.gz"
TEMP_DIR="/tmp/jack-bridge-release-${VERSION}"

echo "Creating jack-bridge release tarball v${VERSION}"
echo "Output: ${TARBALL_NAME}"
echo

# Clean up any previous temp directory
rm -rf "$TEMP_DIR"

# Create temp directory
mkdir -p "$TEMP_DIR"
mkdir -p "$TEMP_DIR/jack-bridge-${VERSION}"

# Copy only the files needed for end-user installation
echo "Copying files..."

# Core project files
cp -r README.md "$TEMP_DIR/jack-bridge-${VERSION}/"
cp -r LICENSE "$TEMP_DIR/jack-bridge-${VERSION}/"
cp -r 50-jack.conf "$TEMP_DIR/jack-bridge-${VERSION}/"

# Installation system
cp -r contrib/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# Source code (needed for building)
cp -r src/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# Additional files needed by installer
cp -r usr/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# Build system
cp Makefile "$TEMP_DIR/jack-bridge-${VERSION}/"

echo "Excluded development files:"
echo "  - qjackctl-1.0.4-jack-bridge-mod/ (build directory)"
echo "  - plans/ (developer documentation)"
echo "  - build-qjackctl.sh (build script)"
echo "  - test-bluetooth.sh (test script)"
echo "  - blue-alsaREADME.md (upstream docs)"
echo "  - bluealsa-INSTALL.md (upstream docs)"
echo "  - Alsa-sound-connect-gui.png (screenshot)"
echo "  - build-bluealsa-plugins.sh (build script)"
echo "  - BUILD_PLUGINS_GUIDE.md (developer docs)"
echo "  - .gitignore (git file)"
echo

# Create the tarball
echo "Creating tarball..."
cd /tmp
tar -czf "$TARBALL_NAME" -C "$TEMP_DIR" "jack-bridge-${VERSION}"

# Move to current directory
mv "$TARBALL_NAME" "$OLDPWD/"

# Cleanup
rm -rf "$TEMP_DIR"

echo "✓ Release tarball created: $TARBALL_NAME"
echo
echo "Contents:"
tar -tzf "$TARBALL_NAME" | head -20
echo "..."
echo
echo "Installation instructions for users:"
echo "  tar -xzf $TARBALL_NAME"
echo "  cd jack-bridge-${VERSION}"
echo "  sudo sh contrib/install.sh"
echo "  sudo reboot"
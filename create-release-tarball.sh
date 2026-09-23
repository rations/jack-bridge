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

# Source code (needed for building). Both GUIs build from the ONE root Makefile now -- jack-graph
# has no Makefile of its own -- so its sources travel with src/.
cp -r src/ "$TEMP_DIR/jack-bridge-${VERSION}/"
cp -r jack-graph/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# The bundled fonts. NOT OPTIONAL and not a theme: mxeq and jack-graph draw every glyph themselves
# through FreeType and neither goes through fontconfig, so without these two faces both windows
# fall back to whatever the system has and the layout stops being the one tools/uirender audits.
# install.sh copies this directory to /usr/local/share/jack-bridge/fonts, which is the path
# compiled into both binaries.
cp -r resources/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# Additional files needed by installer
cp -r usr/ "$TEMP_DIR/jack-bridge-${VERSION}/"

# Build system
cp Makefile "$TEMP_DIR/jack-bridge-${VERSION}/"

# NOTHING BUILT ON THIS MACHINE BUT THE BINARIES THE INSTALLER USES.
#
# `cp -r src/` and `cp -r jack-graph/` carry every object file and dependency file the last build
# left, and those are this machine's. On a release with an older glibc, `make` sees objects newer
# than their sources, skips recompiling them and links them -- which produces the very binary the
# rebuild was meant to replace. The binaries in contrib/bin are what install.sh installs; build
# them with `make clean && make` on the oldest release you ship to (glibc is backward compatible,
# not forward) before running this.
find "$TEMP_DIR/jack-bridge-${VERSION}" \( -name '*.o' -o -name '*.d' \) -type f -delete

# The last gtkmm build of jack-graph, from when Devuan 5 needed its own binary. install.sh stopped
# installing it once jack-graph was built on the oldest release, and there is no GTK anywhere in
# either GUI now -- shipping it would put 600 KB of dead gtkmm binary in every tarball.
rm -f "$TEMP_DIR/jack-bridge-${VERSION}/contrib/bin/jack-graph-devuan-five-version"

echo "Excluded development files:"
echo "  - plans/ (developer documentation)"
echo "  - test-bluetooth.sh (test script)"
echo "  - blue-alsaREADME.md (upstream docs)"
echo "  - bluealsa-INSTALL.md (upstream docs)"
echo "  - Alsa-sound-connect-gui.png (screenshot)"
echo "  - build-bluealsa-plugins.sh (build script)"
echo "  - BUILD_PLUGINS_GUIDE.md (developer docs)"
echo "  - .gitignore (git file)"
echo "  - .kilo/ (Kilo IDE config)"
echo "  - old-dbus/ (old D-Bus service files)"
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
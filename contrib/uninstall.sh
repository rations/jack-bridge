#!/bin/sh
# contrib/uninstall.sh
# Revert files installed by contrib/install.sh and unregister init script.
# Usage: sudo sh contrib/uninstall.sh
set -e

ROOT="/"
INIT_SCRIPT="/etc/init.d/jackd-rt"
DEFAULTS="/etc/default/jackd-rt"
ASOUND_CONF="/etc/asound.conf"
USR_LIB="/usr/lib/jack-bridge"
APULSE_FIREFOX="/usr/bin/apulse-firefox"
APULSE_CHROMIUM="/usr/bin/apulse-chromium"
LIMITS_CONF="/etc/security/limits.d/audio.conf"

echo "Uninstalling jack-bridge contrib files (non-destructive)."

# Stop service if running
if [ -x "$INIT_SCRIPT" ]; then
    echo "Stopping service (if running)..."
    if command -v service >/dev/null 2>&1; then
        service jackd-rt stop 2>/dev/null || true
    else
        /etc/init.d/jackd-rt stop 2>/dev/null || true
    fi
fi

# Unregister init script if available
if command -v update-rc.d >/dev/null 2>&1; then
    echo "Removing init script from default runlevels..."
    update-rc.d -f jackd-rt remove || true
fi

# Remove init script and defaults
if [ -f "$INIT_SCRIPT" ]; then
    rm -f "$INIT_SCRIPT"
    echo "Removed $INIT_SCRIPT"
fi

if [ -f "$DEFAULTS" ]; then
    rm -f "$DEFAULTS"
    echo "Removed $DEFAULTS"
fi

# Remove installed helper scripts dir
if [ -d "$USR_LIB" ]; then
    rm -rf "$USR_LIB"
    echo "Removed $USR_LIB"
fi

# Remove apulse wrappers if they match expected pattern
if [ -f "$APULSE_FIREFOX" ]; then
    # Try to verify wrapper content contains 'exec apulse firefox'
    if grep -q "exec apulse firefox" "$APULSE_FIREFOX" 2>/dev/null || true; then
        rm -f "$APULSE_FIREFOX"
        echo "Removed $APULSE_FIREFOX"
    else
        echo "Skipped $APULSE_FIREFOX (file differs from expected wrapper)"
    fi
fi

if [ -f "$APULSE_CHROMIUM" ]; then
    if grep -q "exec apulse chromium" "$APULSE_CHROMIUM" 2>/dev/null || true; then
        rm -f "$APULSE_CHROMIUM"
        echo "Removed $APULSE_CHROMIUM"
    else
        echo "Skipped $APULSE_CHROMIUM (file differs from expected wrapper)"
    fi
fi

# Remove limits.d template if it matches installed template
if [ -f "$LIMITS_CONF" ]; then
    if grep -q "@audio - rtprio 95" "$LIMITS_CONF" 2>/dev/null || true; then
        echo "Limits file $LIMITS_CONF looks like our template. Leaving it in place for safety; remove manually if desired."
    fi
fi

# Note about /etc/asound.conf
if [ -f "$ASOUND_CONF" ]; then
    echo ""
    echo "Notice: $ASOUND_CONF exists. The installer did NOT overwrite an existing file by default."
    echo "If you added the contrib template manually and want it removed, delete $ASOUND_CONF yourself."
fi

echo ""
echo "Uninstall complete. If you need to completely revert system changes, manually review:"
echo " - /etc/asound.conf"
echo " - /etc/security/limits.d/audio.conf"
echo ""
echo "To re-register the init script later, run: sudo update-rc.d jackd-rt defaults"
exit 0
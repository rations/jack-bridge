#!/bin/sh
# contrib/uninstall.sh
# Authoritative removal of jack-bridge artifacts installed by contrib/install.sh.
# This uninstaller is destructive for project-installed files to avoid confusion.
# Usage: sudo sh contrib/uninstall.sh
set -e

require_root() {
  if [ "$(id -u)" -ne 0 ]; then
    echo "This uninstaller must be run as root. Try: sudo ./contrib/uninstall.sh"
    exit 1
  fi
}
require_root

log() { printf '%s\n' "$*"; }

# Paths and artifacts managed by this project
INIT_JACK="/etc/init.d/jackd-rt"
DEF_JACK="/etc/default/jackd-rt"
INIT_BT_HELPER="/etc/init.d/jack-bridge-bluetooth-config"
USR_LIB="/usr/local/lib/jack-bridge"
BIN_BLUEALSAD="/usr/local/bin/bluealsad"
BIN_BLUEALSActl="/usr/local/bin/bluealsactl"
BIN_BLUEALSA_APLAY="/usr/local/bin/bluealsa-aplay"
BIN_BLUEALSA_RFCOMM="/usr/local/bin/bluealsa-rfcomm"
APULSE_FIREFOX="/usr/bin/apulse-firefox"
APULSE_CHROMIUM="/usr/bin/apulse-chromium"
ASOUND_CONF="/etc/asound.conf"
DEVCONF="/etc/jack-bridge/devices.conf"
DEVCONFDIR="/etc/jack-bridge"
XDG_AUTOSTART="/etc/xdg/autostart/jack-bridge-qjackctl.desktop"
POLKIT_RULE="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
DBUS_BLUEALSA="/usr/share/dbus-1/system.d/org.bluealsa.conf"
LIMITS_CONF="/etc/security/limits.d/audio.conf"

# Stop and deregister SysV init scripts we installed/managed
if command -v service >/dev/null 2>&1; then
  service jackd-rt stop 2>/dev/null || true
fi

if command -v update-rc.d >/dev/null 2>&1; then
  update-rc.d -f jackd-rt remove 2>/dev/null || true
  update-rc.d -f jack-bridge-bluetooth-config remove 2>/dev/null || true
  # Best-effort: also remove bluealsad if our contrib init was ever installed
  update-rc.d -f bluealsad remove 2>/dev/null || true
fi

# Remove init scripts and defaults
for f in "$INIT_JACK" "$DEF_JACK" "$INIT_BT_HELPER"; do
  if [ -e "$f" ]; then
    rm -f "$f"
    log "Removed $f"
  fi
done

# Remove helper library directory and its contents
if [ -d "$USR_LIB" ]; then
  rm -rf "$USR_LIB"
  log "Removed $USR_LIB"
fi

# Remove prebuilt BlueALSA binaries if we installed them
for f in "$BIN_BLUEALSAD" "$BIN_BLUEALSActl" "$BIN_BLUEALSA_APLAY" "$BIN_BLUEALSA_RFCOMM"; do
  if [ -f "$f" ]; then
    rm -f "$f"
    log "Removed $f"
  fi
done

# Remove apulse wrappers we installed
if [ -f "$APULSE_FIREFOX" ] && grep -q "exec apulse firefox" "$APULSE_FIREFOX" 2>/dev/null; then
  rm -f "$APULSE_FIREFOX"
  log "Removed $APULSE_FIREFOX"
fi
if [ -f "$APULSE_CHROMIUM" ] && grep -q "exec apulse chromium" "$APULSE_CHROMIUM" 2>/dev/null; then
  rm -f "$APULSE_CHROMIUM"
  log "Removed $APULSE_CHROMIUM"
fi

# Remove ALSA config and jack-bridge device mapping
if [ -f "$ASOUND_CONF" ]; then
  rm -f "$ASOUND_CONF"
  log "Removed $ASOUND_CONF"
fi

if [ -f "$DEVCONF" ]; then
  rm -f "$DEVCONF"
  log "Removed $DEVCONF"
fi
# Remove directory if empty
if [ -d "$DEVCONFDIR" ] && [ -z "$(ls -A "$DEVCONFDIR" 2>/dev/null)" ]; then
  rmdir "$DEVCONFDIR" 2>/dev/null || true
  log "Removed empty $DEVCONFDIR"
fi

# Remove desktop autostart for qjackctl
if [ -f "$XDG_AUTOSTART" ]; then
  rm -f "$XDG_AUTOSTART"
  log "Removed $XDG_AUTOSTART"
fi

# Remove polkit rule and dbus policy we installed
if [ -f "$POLKIT_RULE" ]; then
  rm -f "$POLKIT_RULE"
  log "Removed $POLKIT_RULE"
fi

if [ -f "$DBUS_BLUEALSA" ]; then
  rm -f "$DBUS_BLUEALSA"
  log "Removed $DBUS_BLUEALSA"
fi

# Remove legacy/obsolete artifacts from older versions (cleanup)
for legacy in \
  /etc/init.d/jack-bluealsa-autobridge \
  /usr/local/bin/jack-bluealsa-autobridge \
  /etc/jack-bridge/bluetooth.conf \
  /tmp/bluez-monitor.*.log \
  ; do
  if [ -e "$legacy" ]; then
    rm -f "$legacy"
    log "Removed obsolete $legacy"
  fi
done

# Leave system realtime limits as-is unless it matches our known template
if [ -f "$LIMITS_CONF" ] && grep -q "@audio - rtprio 95" "$LIMITS_CONF" 2>/dev/null; then
  rm -f "$LIMITS_CONF"
  log "Removed $LIMITS_CONF"
fi

echo "Uninstall complete."
exit 0
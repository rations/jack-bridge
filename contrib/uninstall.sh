#!/bin/sh
# contrib/uninstall.sh
# Comprehensive removal of jack-bridge artifacts installed by contrib/install.sh.
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

echo "========================================="
echo "jack-bridge Uninstaller"
echo "========================================="
echo ""
echo "This will remove:"
echo "  - Init scripts and service registrations"
echo "  - Installed binaries (mxeq, BlueALSA tools)"
echo "  - Configuration files"
echo "  - Desktop launcher"
echo "  - Polkit rules and D-Bus policies"
echo ""
echo "This will NOT remove:"
echo "  - Installed packages (jackd, bluez, etc.)"
echo "  - User recordings in ~/Music/"
echo "  - User configs in ~/.config/jack-bridge/"
echo ""
printf "Continue? [y/N] "
read answer
case "$answer" in
  [Yy]*) ;;
  *) echo "Cancelled."; exit 0 ;;
esac
echo ""

# Paths and artifacts managed by this project
INIT_JACK="/etc/init.d/jackd-rt"
INIT_BLUEALSA="/etc/init.d/bluealsad"
INIT_BLUETOOTH="/etc/init.d/bluetoothd"
INIT_PORTS="/etc/init.d/jack-bridge-ports"
DEF_JACK="/etc/default/jackd-rt"
USR_LIB="/usr/local/lib/jack-bridge"
BIN_MXEQ="/usr/local/bin/mxeq"
BIN_BLUEALSAD="/usr/local/bin/bluealsad"
BIN_BLUEALSActl="/usr/local/bin/bluealsactl"
BIN_BLUEALSA_APLAY="/usr/local/bin/bluealsa-aplay"
BIN_BLUEALSA_RFCOMM="/usr/local/bin/bluealsa-rfcomm"
APULSE_FIREFOX="/usr/bin/apulse-firefox"
APULSE_CHROMIUM="/usr/bin/apulse-chromium"
ASOUND_CONF="/etc/asound.conf"
ASOUND_CONF_D="/etc/asound.conf.d"
DEVCONF="/etc/jack-bridge/devices.conf"
DEVCONFDIR="/etc/jack-bridge"
DESKTOP_LAUNCHER="/usr/share/applications/mxeq.desktop"
XDG_AUTOSTART="/etc/xdg/autostart/jack-bridge-qjackctl.desktop"
POLKIT_RULE="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
DBUS_BLUEALSA="/usr/share/dbus-1/system.d/org.bluealsa.conf"
DBUS_BLUEALSA_CONF="/etc/asound.conf.d/20-jack-bridge-bluealsa.conf"
LIMITS_CONF="/etc/security/limits.d/audio.conf"
ICON_DIR="/usr/share/icons/hicolor/scalable/apps"
ICON_FILE="$ICON_DIR/alsasoundconnectlogo.png"
ALSA_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/alsa-lib"
BLUEALSA_PCM_PLUGIN="$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
BLUEALSA_CTL_PLUGIN="$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so"

log "Stopping services..."

# Stop all services we manage
for service in jack-bridge-ports jackd-rt bluealsad bluetoothd; do
  if command -v service >/dev/null 2>&1; then
    service "$service" stop 2>/dev/null || true
  fi
done

log "Deregistering init scripts..."

# Deregister SysV init scripts
if command -v update-rc.d >/dev/null 2>&1; then
  for script in jackd-rt bluealsad bluetoothd jack-bridge-ports; do
    update-rc.d -f "$script" remove 2>/dev/null || true
  done
fi

log "Removing init scripts..."

# Remove init scripts and defaults
for f in "$INIT_JACK" "$INIT_BLUEALSA" "$INIT_BLUETOOTH" "$INIT_PORTS" "$DEF_JACK"; do
  if [ -e "$f" ]; then
    rm -f "$f"
    log "  Removed $f"
  fi
done

log "Removing binaries and libraries..."

# Remove helper library directory and its contents
if [ -d "$USR_LIB" ]; then
  rm -rf "$USR_LIB"
  log "  Removed $USR_LIB"
fi

# Remove GUI and BlueALSA binaries
for f in "$BIN_MXEQ" "$BIN_BLUEALSAD" "$BIN_BLUEALSActl" "$BIN_BLUEALSA_APLAY" "$BIN_BLUEALSA_RFCOMM"; do
  if [ -f "$f" ]; then
    rm -f "$f"
    log "  Removed $f"
  fi
done

log "Removing browser wrappers..."

# Remove apulse wrappers we installed
if [ -f "$APULSE_FIREFOX" ] && grep -q "exec apulse firefox" "$APULSE_FIREFOX" 2>/dev/null; then
  rm -f "$APULSE_FIREFOX"
  log "  Removed $APULSE_FIREFOX"
fi
if [ -f "$APULSE_CHROMIUM" ] && grep -q "exec apulse chromium" "$APULSE_CHROMIUM" 2>/dev/null; then
  rm -f "$APULSE_CHROMIUM"
  log "  Removed $APULSE_CHROMIUM"
fi

log "Removing configuration files..."

# Remove ALSA config and jack-bridge device mapping
if [ -f "$ASOUND_CONF" ]; then
  rm -f "$ASOUND_CONF"
  log "  Removed $ASOUND_CONF"
fi

# Remove ALSA config.d directory
if [ -d "$ASOUND_CONF_D" ]; then
  rm -rf "$ASOUND_CONF_D"
  log "  Removed $ASOUND_CONF_D"
fi

if [ -f "$DEVCONF" ]; then
  rm -f "$DEVCONF"
  log "  Removed $DEVCONF"
fi

# Remove directory if empty
if [ -d "$DEVCONFDIR" ] && [ -z "$(ls -A "$DEVCONFDIR" 2>/dev/null)" ]; then
  rmdir "$DEVCONFDIR" 2>/dev/null || true
  log "  Removed empty $DEVCONFDIR"
fi

# Remove BlueALSA ALSA plugins
if [ -f "$BLUEALSA_PCM_PLUGIN" ]; then
  rm -f "$BLUEALSA_PCM_PLUGIN"
  log "  Removed $BLUEALSA_PCM_PLUGIN"
fi
if [ -f "$BLUEALSA_CTL_PLUGIN" ]; then
  rm -f "$BLUEALSA_CTL_PLUGIN"
  log "  Removed $BLUEALSA_CTL_PLUGIN"
fi

# Restore distro backups if they exist
if [ -f "$BLUEALSA_PCM_PLUGIN.distro-backup" ]; then
  mv "$BLUEALSA_PCM_PLUGIN.distro-backup" "$BLUEALSA_PCM_PLUGIN"
  log "  Restored distro $BLUEALSA_PCM_PLUGIN"
fi
if [ -f "$BLUEALSA_CTL_PLUGIN.distro-backup" ]; then
  mv "$BLUEALSA_CTL_PLUGIN.distro-backup" "$BLUEALSA_CTL_PLUGIN"
  log "  Restored distro $BLUEALSA_CTL_PLUGIN"
fi

log "Removing desktop integration..."

# Remove desktop launcher
if [ -f "$DESKTOP_LAUNCHER" ]; then
  rm -f "$DESKTOP_LAUNCHER"
  log "  Removed $DESKTOP_LAUNCHER"
fi

# Remove desktop autostart for qjackctl
if [ -f "$XDG_AUTOSTART" ]; then
  rm -f "$XDG_AUTOSTART"
  log "  Removed $XDG_AUTOSTART"
fi

# Remove icon
if [ -f "$ICON_FILE" ]; then
  rm -f "$ICON_FILE"
  log "  Removed $ICON_FILE"
  # Update icon cache if possible
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
  fi
fi

log "Removing polkit and D-Bus policies..."

# Remove polkit rule and dbus policies we installed
if [ -f "$POLKIT_RULE" ]; then
  rm -f "$POLKIT_RULE"
  log "  Removed $POLKIT_RULE"
fi

if [ -f "$DBUS_BLUEALSA" ]; then
  rm -f "$DBUS_BLUEALSA"
  log "  Removed $DBUS_BLUEALSA"
fi

if [ -f "$DBUS_BLUEALSA_CONF" ]; then
  rm -f "$DBUS_BLUEALSA_CONF"
  log "  Removed $DBUS_BLUEALSA_CONF"
fi

log "Cleaning up legacy artifacts..."

# Remove legacy/obsolete artifacts from older versions (cleanup)
for legacy in \
  /etc/init.d/jack-bluealsa-autobridge \
  /etc/init.d/jack-bridge-bluetooth-config \
  /usr/local/bin/jack-bluealsa-autobridge \
  /etc/jack-bridge/bluetooth.conf \
  ; do
  if [ -e "$legacy" ]; then
    rm -f "$legacy"
    log "  Removed obsolete $legacy"
  fi
done

# Clean up temporary monitor logs
rm -f /tmp/bluez-monitor.*.log 2>/dev/null || true

# Leave system realtime limits as-is unless it matches our known template
if [ -f "$LIMITS_CONF" ] && grep -q "@audio - rtprio 95" "$LIMITS_CONF" 2>/dev/null; then
  rm -f "$LIMITS_CONF"
  log "  Removed $LIMITS_CONF"
fi

# Reload D-Bus to remove our policies
log "Reloading D-Bus configuration..."
if command -v systemctl >/dev/null 2>&1 && systemctl is-active dbus >/dev/null 2>&1; then
  systemctl reload dbus 2>/dev/null || true
elif [ -f /etc/init.d/dbus ]; then
  /etc/init.d/dbus reload 2>/dev/null || service dbus reload 2>/dev/null || true
fi

# Reload polkit
if command -v systemctl >/dev/null 2>&1 && systemctl is-active polkit >/dev/null 2>&1; then
  systemctl reload polkit 2>/dev/null || true
fi

echo ""
echo "========================================="
echo "Uninstall complete!"
echo "========================================="
echo ""
echo "jack-bridge has been removed from your system."
echo ""
echo "To also remove installed packages, run:"
echo "  sudo apt remove jackd2 qjackctl bluez bluez-tools \\"
echo "  apulse libasound2-plugin-equal \\"
echo"
echo "  sudo apt autoremove"
echo ""
echo "User data preserved:"
echo "  - Recordings in ~/Music/"
echo "  - User configs in ~/.config/jack-bridge/"
echo "  - EQ presets in ~/.local/share/mxeq/"
echo ""

exit 0
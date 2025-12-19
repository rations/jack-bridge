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
echo "  - Installed binaries (mxeq, BlueALSA tools, qjackctl)"
echo "  - Configuration files"
echo "  - Desktop launchers and icons"
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
INIT_BLUETOOTH_CONFIG="/etc/init.d/jack-bridge-bluetooth-config"
INIT_CONNECTION_MANAGER="/etc/init.d/jack-connection-manager"
INIT_DBUS="/etc/init.d/jack-bridge-dbus"
DEF_JACK="/etc/default/jackd-rt"
DEF_BLUEALSA="/etc/default/bluealsad"
USR_LIB="/usr/local/lib/jack-bridge"
BIN_MXEQ="/usr/local/bin/mxeq"
BIN_BLUEALSAD="/usr/local/bin/bluealsad"
BIN_BLUEALSActl="/usr/local/bin/bluealsactl"
BIN_BLUEALSA_APLAY="/usr/local/bin/bluealsa-aplay"
BIN_BLUEALSA_RFCOMM="/usr/local/bin/bluealsa-rfcomm"
BIN_JACK_CONNECTION_MANAGER="/usr/local/bin/jack-connection-manager"
BIN_JACK_BRIDGE_DBUS="/usr/local/bin/jack-bridge-dbus"
BIN_JACK_BRIDGE_QJACKCTL_SETUP="/usr/local/bin/jack-bridge-qjackctl-setup"
BIN_JACK_BRIDGE_VERIFY_QJACKCTL="/usr/local/bin/jack-bridge-verify-qjackctl"
BIN_QJACKCTL="/usr/local/bin/qjackctl"
APULSE_FIREFOX="/usr/bin/apulse-firefox"
APULSE_CHROMIUM="/usr/bin/apulse-chromium"
ASOUND_CONF="/etc/asound.conf"
ASOUND_CONF_D="/etc/asound.conf.d"
DEVCONF="/etc/jack-bridge/devices.conf"
DEVCONFDIR="/etc/jack-bridge"
DESKTOP_LAUNCHER="/usr/share/applications/mxeq.desktop"
DESKTOP_QJACKCTL="/usr/share/applications/qjackctl.desktop"
XDG_AUTOSTART="/etc/xdg/autostart/qjackctl.desktop"
POLKIT_RULE_BLUETOOTH="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
POLKIT_RULE_JACK="/etc/polkit-1/rules.d/50-jack-bridge.rules"
DBUS_BLUEALSA="/usr/share/dbus-1/system.d/org.bluealsa.conf"
DBUS_BLUEALSA_CONF="/etc/asound.conf.d/20-jack-bridge-bluealsa.conf"
DBUS_JACKAUDIO_SERVICE="/usr/share/dbus-1/system-services/org.jackaudio.service.service"
DBUS_JACKAUDIO_CONF="/usr/share/dbus-1/system.d/org.jackaudio.service.conf"
LIMITS_CONF="/etc/security/limits.d/audio.conf"
PULSE_AUTOSPAWN_CONF="/etc/pulse/client.conf.d/01-no-autospawn.conf"
ICON_DIR="/usr/share/icons/hicolor/scalable/apps"
ICON_FILE="$ICON_DIR/alsasoundconnectlogo.png"
ICON_ALSA_SOUND_CONNECT="$ICON_DIR/alsa-sound-connect.png"
ICON_QJACKCTL_SVG="/usr/share/icons/hicolor/scalable/apps/qjackctl.svg"
ALSA_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/alsa-lib"
BLUEALSA_PCM_PLUGIN="$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
BLUEALSA_CTL_PLUGIN="$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so"
ALSA_CONF_DIRS="/usr/share/alsa/alsa.conf.d /etc/alsa/conf.d"

log "Stopping services..."

# Stop all services we manage
for service in jack-bridge-ports jackd-rt bluealsad bluetoothd jack-bridge-bluetooth-config jack-connection-manager jack-bridge-dbus; do
  if command -v service >/dev/null 2>&1; then
    service "$service" stop 2>/dev/null || true
  fi
done

log "Deregistering init scripts..."

# Deregister SysV init scripts
if command -v update-rc.d >/dev/null 2>&1; then
  for script in jackd-rt bluealsad bluetoothd jack-bridge-ports jack-bridge-bluetooth-config jack-connection-manager jack-bridge-dbus; do
    update-rc.d -f "$script" remove 2>/dev/null || true
  done
fi

log "Removing init scripts..."

# Remove init scripts and defaults
for f in "$INIT_JACK" "$INIT_BLUEALSA" "$INIT_BLUETOOTH" "$INIT_PORTS" "$INIT_BLUETOOTH_CONFIG" "$INIT_CONNECTION_MANAGER" "$INIT_DBUS" "$DEF_JACK" "$DEF_BLUEALSA"; do
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
for f in "$BIN_MXEQ" "$BIN_BLUEALSAD" "$BIN_BLUEALSActl" "$BIN_BLUEALSA_APLAY" "$BIN_BLUEALSA_RFCOMM" "$BIN_JACK_CONNECTION_MANAGER" "$BIN_JACK_BRIDGE_DBUS" "$BIN_JACK_BRIDGE_QJACKCTL_SETUP" "$BIN_JACK_BRIDGE_VERIFY_QJACKCTL" "$BIN_QJACKCTL"; do
  if [ -f "$f" ]; then
    rm -f "$f"
    log "  Removed $f"
  fi
done

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

# Remove PulseAudio autospawn config
if [ -f "$PULSE_AUTOSPAWN_CONF" ]; then
  rm -f "$PULSE_AUTOSPAWN_CONF"
  log "  Removed $PULSE_AUTOSPAWN_CONF"
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

# Remove qjackctl desktop file
if [ -f "$DESKTOP_QJACKCTL" ]; then
  rm -f "$DESKTOP_QJACKCTL"
  log "  Removed $DESKTOP_QJACKCTL"
fi

# Remove desktop autostart for qjackctl
if [ -f "$XDG_AUTOSTART" ]; then
  rm -f "$XDG_AUTOSTART"
  log "  Removed $XDG_AUTOSTART"
fi

# Remove icons
if [ -f "$ICON_FILE" ]; then
  rm -f "$ICON_FILE"
  log "  Removed $ICON_FILE"
fi

if [ -f "$ICON_ALSA_SOUND_CONNECT" ]; then
  rm -f "$ICON_ALSA_SOUND_CONNECT"
  log "  Removed $ICON_ALSA_SOUND_CONNECT"
fi

if [ -f "$ICON_QJACKCTL_SVG" ]; then
  rm -f "$ICON_QJACKCTL_SVG"
  log "  Removed $ICON_QJACKCTL_SVG"
fi

# Update icon cache if possible
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi

log "Removing polkit and D-Bus policies..."

# Remove polkit rules
if [ -f "$POLKIT_RULE_BLUETOOTH" ]; then
  rm -f "$POLKIT_RULE_BLUETOOTH"
  log "  Removed $POLKIT_RULE_BLUETOOTH"
fi

if [ -f "$POLKIT_RULE_JACK" ]; then
  rm -f "$POLKIT_RULE_JACK"
  log "  Removed $POLKIT_RULE_JACK"
fi

# Remove D-Bus policies and services
if [ -f "$DBUS_BLUEALSA" ]; then
  rm -f "$DBUS_BLUEALSA"
  log "  Removed $DBUS_BLUEALSA"
fi

if [ -f "$DBUS_BLUEALSA_CONF" ]; then
  rm -f "$DBUS_BLUEALSA_CONF"
  log "  Removed $DBUS_BLUEALSA_CONF"
fi

if [ -f "$DBUS_JACKAUDIO_SERVICE" ]; then
  rm -f "$DBUS_JACKAUDIO_SERVICE"
  log "  Removed $DBUS_JACKAUDIO_SERVICE"
fi

if [ -f "$DBUS_JACKAUDIO_CONF" ]; then
  rm -f "$DBUS_JACKAUDIO_CONF"
  log "  Removed $DBUS_JACKAUDIO_CONF"
fi

log "Cleaning up legacy artifacts..."

# Remove legacy/obsolete artifacts from older versions (cleanup)
for legacy in \
  /etc/init.d/jack-bluealsa-autobridge \
  /usr/local/bin/jack-bluealsa-autobridge \
  /etc/jack-bridge/bluetooth.conf \
  ; do
  if [ -e "$legacy" ]; then
    rm -f "$legacy"
    log "  Removed obsolete $legacy"
  fi
done

# Remove ALSA configuration files from multiple directories
for dir in $ALSA_CONF_DIRS; do
  if [ -f "$dir/50-jack.conf" ]; then
    rm -f "$dir/50-jack.conf"
    log "  Removed $dir/50-jack.conf"
  fi
  if [ -f "$dir/20-jack-bridge-bluealsa.conf" ]; then
    rm -f "$dir/20-jack-bridge-bluealsa.conf"
    log "  Removed $dir/20-jack-bridge-bluealsa.conf"
  fi
  if [ -f "$dir/20-bluealsa.conf" ]; then
    rm -f "$dir/20-bluealsa.conf"
    log "  Removed $dir/20-bluealsa.conf"
  fi
done

# Clean up temporary monitor logs
rm -f /tmp/bluez-monitor.*.log 2>/dev/null || true

# Leave system realtime limits as-is unless it matches our known template
if [ -f "$LIMITS_CONF" ] && grep -q "@audio - rtprio 95" "$LIMITS_CONF" 2>/dev/null; then
  rm -f "$LIMITS_CONF"
  log "  Removed $LIMITS_CONF"
fi

# Remove additional ALSA configuration files
if [ -f "/etc/alsa/conf.d/50-jack.conf" ]; then
  rm -f "/etc/alsa/conf.d/50-jack.conf"
  log "  Removed /etc/alsa/conf.d/50-jack.conf"
fi

if [ -f "/usr/share/alsa/alsa.conf.d/50-jack.conf" ]; then
  rm -f "/usr/share/alsa/alsa.conf.d/50-jack.conf"
  log "  Removed /usr/share/alsa/alsa.conf.d/50-jack.conf"
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
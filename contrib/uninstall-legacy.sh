#!/bin/sh
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "This cleanup script must be run as root."
    echo "Usage: sudo $0 -y"
    exit 1
fi

if [ "$1" != "-y" ]; then
    echo "This will remove legacy jack-bridge binaries, icons and configs."
    echo "To proceed run: sudo $0 -y"
    exit 0
fi

echo "Stopping related services (best-effort)..."
# Best-effort stop; ignore failures
service bluealsad stop 2>/dev/null || true
service jackd stop 2>/dev/null || true

echo "Removing binaries and helpers..."
rm -f /usr/local/bin/mxeq
rm -f /usr/local/bin/bluealsad
rm -f /usr/local/bin/bluealsactl
rm -f /usr/local/bin/bluealsa-aplay
rm -f /usr/local/bin/bluealsa-rfcomm
rm -f /usr/local/bin/apulse-firefox
rm -f /usr/local/bin/apulse-chromium

echo "Removing installed libs and scripts..."
rm -rf /usr/local/lib/jack-bridge || true
rm -f /usr/local/lib/jack-bridge/jack-route-select || true
rm -f /usr/local/lib/jack-bridge/bluetooth-enable.sh || true

echo "Removing desktop entries and icons..."
rm -f /usr/share/applications/mxeq.desktop || true
rm -f /usr/share/applications/apulse-firefox.desktop || true
rm -f /usr/share/applications/apulse-chromium.desktop || true
rm -f /usr/share/icons/hicolor/scalable/apps/alsa-sound-connect.png || true
rm -f /usr/share/icons/hicolor/scalable/apps/mxeq-bluetooth.svg || true

echo "Removing system configs..."
rm -rf /etc/jack-bridge || true
rm -f /etc/asound.conf.d/current_input.conf || true
rm -f /etc/jack-bridge/devices.conf || true
rm -f /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules || true
rm -f /etc/pulse/client.conf.d/01-no-autospawn.conf || true
rm -f /etc/init.d/jackd-rt || true
rm -f /etc/init.d/bluealsad || true
rm -f /etc/init.d/jack-bridge-bluetooth-config || true

echo "Note: this does not edit user ~/.asoundrc. To remove the GUI-managed block from your ~/.asoundrc run:"
echo "  sed -n '1,/^# BEGIN jack-bridge/ p' ~/.asoundrc > ~/.asoundrc.clean && sed -n '/^# END jack-bridge/,\$ p' ~/.asoundrc >> ~/.asoundrc.clean && mv ~/.asoundrc.clean ~/.asoundrc"

echo "Done. You can now build and reinstall:"
echo "  make -j && sudo sh contrib/install.sh"

exit 0
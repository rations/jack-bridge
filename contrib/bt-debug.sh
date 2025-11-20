#!/bin/sh
# Simple Bluetooth troubleshooting script for jack-bridge
# Usage: sudo sh contrib/bt-debug.sh  (some checks require root)
set -e

echo "1) bluetoothd status:"
if pidof bluetoothd >/dev/null 2>&1; then
  echo "bluetoothd running (pids: $(pidof bluetoothd))"
else
  echo "bluetoothd not running"
fi
echo

echo "2) rfkill state:"
if command -v rfkill >/dev/null 2>&1; then
  rfkill list || echo "rfkill list failed"
else
  echo "rfkill not installed. Install: sudo apt update && sudo apt install -y rfkill"
fi
echo
echo "2b) bluetoothctl adapter info (if available):"
if command -v bluetoothctl >/dev/null 2>&1; then
  bluetoothctl show || echo "bluetoothctl show failed or no adapter"
  echo
  echo "Known devices:"
  bluetoothctl devices || echo "bluetoothctl devices failed or none"
else
  echo "bluetoothctl not installed or not in PATH."
fi
echo

echo "3) D-Bus BlueZ adapters (GetManagedObjects -> show Adapter1 paths):"
gdbus call --system --dest org.bluez --object-path / --method org.freedesktop.DBus.ObjectManager.GetManagedObjects | sed -n '1,200p'
echo

echo "4) Check polkit rule file:"
if [ -f /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules ]; then
  echo "polkit rule present: /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
else
  echo "polkit rule missing"
fi
echo

echo "5) Check D-Bus policy for bluealsa:"
if [ -f /usr/share/dbus-1/system.d/org.bluealsa.conf ]; then
  echo "/usr/share/dbus-1/system.d/org.bluealsa.conf present"
else
  echo "org.bluealsa D-Bus policy missing"
fi
echo

echo "6) Current user groups (effective):"
id -nG
echo

echo "7) Try StartDiscovery directly on hci0 (may error — expected for permission issues):"
gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.bluez.Adapter1.StartDiscovery || true
echo

echo "Finished checks."
#!/bin/sh
# Simple Bluetooth troubleshooting script for jack-bridge
# Usage: sudo sh contrib/bt-debug.sh  (some checks require root)
set +e
# Continue on error so the script completes and lets the user perform interactive tests.
# Many checks are best-effort and should not abort the whole run.

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

echo "6) Current user and session info:"
# Determine the logged-in desktop user where possible (prefer logname; fall back to SUDO_USER or $USER)
LOGIN_USER=$(logname 2>/dev/null || true)
if [ -z "$LOGIN_USER" ]; then
    LOGIN_USER=${SUDO_USER:-$USER}
fi
echo "Login user: ${LOGIN_USER}"
if [ -n "$SUDO_USER" ]; then
    echo "SUDO_USER: ${SUDO_USER}"
fi
echo "Effective groups for current (effective) user:"
id -nG
echo "Groups for login user (${LOGIN_USER}):"
id -nG "${LOGIN_USER}" 2>/dev/null || echo "Could not query groups for ${LOGIN_USER}"
if [ -n "$SUDO_USER" ]; then
    echo "Groups for SUDO_USER (${SUDO_USER}):"
    id -nG "${SUDO_USER}" 2>/dev/null || echo "Could not query groups for ${SUDO_USER}"
fi
echo
# Check BlueALSA/bluealsad runtime presence
echo "BlueALSA daemon (bluealsad) process status:"
if pidof bluealsad >/dev/null 2>&1; then
    echo "bluealsad running (pids: $(pidof bluealsad))"
else
    echo "bluealsad not running"
fi
echo
# Check whether org.bluealsa is owned on the system bus
echo "DBus name org.bluealsa ownership (system bus):"
if command -v gdbus >/dev/null 2>&1; then
    gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.NameHasOwner org.bluealsa 2>/dev/null || echo "(query failed or no owner)"
else
    echo "gdbus not installed; cannot query org.bluealsa"
fi
echo

echo "7) Try StartDiscovery directly on hci0 (may error — expected for permission issues):"
if command -v gdbus >/dev/null 2>&1; then
    gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.bluez.Adapter1.StartDiscovery 2>&1 || true
else
    echo "gdbus not installed; cannot attempt StartDiscovery"
fi
echo

# Interactive monitoring helper
# This allows you to exercise the GUI while the script captures D-Bus events.
read -r -p "Start interactive BlueZ monitor now so you can exercise the GUI? [y/N] " ans
if [ "$ans" = "y" ] || [ "$ans" = "Y" ]; then
    if command -v gdbus >/dev/null 2>&1; then
        MON_LOG="/tmp/bluez-monitor.log"
        echo "Starting gdbus monitor -> $MON_LOG (background). Open the GUI and perform Scan/Pair/Connect actions."
        echo "When finished, return to this terminal and press Enter to stop monitoring."
        gdbus monitor --system --dest org.bluez >"$MON_LOG" 2>&1 &
        MON_PID=$!
        # Wait for user to finish testing
        read -r -p ""
        # Stop monitor
        kill "$MON_PID" 2>/dev/null || true
        sleep 1
        echo "Monitor stopped. Saved to $MON_LOG"
        echo "Tail last 200 lines from monitor log:"
        tail -n 200 "$MON_LOG" || true
    else
        echo "gdbus not installed; cannot run interactive monitor."
    fi
fi

echo "Finished checks."
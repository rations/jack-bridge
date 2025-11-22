# Bluetooth build, debug and verification plan

Overview
--------
This document explains steps to build the GUI and verify Bluetooth behavior, how to reproduce and capture errors for Bluetooth scan failures, and a troubleshooting checklist for services, D‑Bus and polkit. The autobridge component has been removed; routing is handled via JACK with jack-route-select and settings in /etc/jack-bridge/devices.conf.

Files referenced
--------------
- [`src/gui_bt.c`](src/gui_bt.c:1)
- [`src/mxeq.c`](src/mxeq.c:1)
- [`src/bt_agent.c`](src/bt_agent.c:1)
- [`contrib/usr/local/lib/jack-bridge/jack-route-select`](contrib/usr/local/lib/jack-bridge/jack-route-select:1)
- [`/etc/jack-bridge/devices.conf`](etc/jack-bridge/devices.conf:1)
- [`contrib/install.sh`](contrib/install.sh:1)

Build commands (compile individually)
-------------------------------------
1) Compile the GUI (mxeq) — links GTK3, GLib/GIO and ALSA:

gcc -Wall -Wextra -o contrib/bin/mxeq src/mxeq.c src/gui_bt.c src/bt_agent.c $(pkg-config --cflags --libs gtk+-3.0 glib-2.0 gio-2.0 alsa)

2) Runtime routing helper:
- jack-route-select is a POSIX shell helper installed to /usr/local/lib/jack-bridge/jack-route-select by the installer. It rewires JACK ports and spawns alsa_out for USB/HDMI/Bluetooth as needed, persisting preferences in /etc/jack-bridge/devices.conf.

Notes
-----
- Compile each binary separately; avoid a single big compile command that mixes GUI and daemon flags.
- If pkg-config reports missing packages, install them (the installer attempts to do this).

How to run the GUI to capture errors
-----------------------------------
- Run with GLib message debugging and capture stderr:

G_MESSAGES_DEBUG=all ./contrib/bin/mxeq 2>&1 | tee /tmp/mxeq.log

- Look for lines printed by the new discovery callback (e.g., "gui_bt: discovery operation failed: ...") or warnings from [`src/gui_bt.c`](src/gui_bt.c:1).

Reproduce StartDiscovery error synchronously (quick test)
--------------------------------------------------------
- Use gdbus to call StartDiscovery and observe the error immediately:

gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.bluez.Adapter1.StartDiscovery

- If the adapter is not `/org/bluez/hci0`, use:

gdbus call --system --dest org.bluez / org.freedesktop.DBus.ObjectManager.GetManagedObjects

and parse the output to find the adapter object path.

Useful runtime files and checks
-------------------------------
- BlueALSA log (if configured): /var/log/bluealsad.log
- BlueZ bluetoothd: /var/log/bluetoothd.log or system syslog
- D-Bus policy: /usr/share/dbus-1/system.d/org.bluealsa.conf
- Polkit rule: /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules
- Device routing config: /etc/jack-bridge/devices.conf (verify BLUETOOTH_DEVICE, BT_PERIOD, BT_NPERIODS, PREFERRED_OUTPUT)
- Routing helper: /usr/local/lib/jack-bridge/jack-route-select
- Verify JACK ports after selection:
  jack_lsp | egrep '^(alsa_pcm:playback_|out_usb:|out_hdmi:|bt_out:)'

Commands to verify services & permissions
-----------------------------------------
1) Is bluetoothd running?
   pidof bluetoothd || ps aux | grep bluetoothd

2) Is adapter blocked by rfkill?
   rfkill list
   rfkill unblock bluetooth

3) Can the system bus be reached and does BlueZ own names?
   gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.ListNames

4) Check BlueALSA D-Bus policy file presence:
   ls -l /usr/share/dbus-1/system.d/org.bluealsa.conf

5) Check polkit rule presence:
   ls -l /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules

6) Ensure your desktop user is in audio and bluetooth groups:
   id -nG | tr ' ' '\n' | egrep '^(audio|bluetooth)$' || id -nG

D-Bus interactive debugging
---------------------------
- Monitor BlueZ signals:
   gdbus monitor --system --dest org.bluez

- Or use dbus-monitor:
   sudo dbus-monitor --system "type='signal',interface='org.freedesktop.DBus.ObjectManager'"

Specific tests to run if Scan yields nothing
------------------------------------------
1) From terminal:
   - Run `G_MESSAGES_DEBUG=all ./contrib/bin/mxeq` and press Scan. Inspect `/tmp/mxeq.log` for the "discovery operation failed" message printed by [`src/gui_bt.c`](src/gui_bt.c:1).

2) If discovery fails with NotAuthorized or AccessDenied:
   - Ensure polkit rule exists and polkitd has been reloaded (installer attempts to reload polkit).

3) If discovery fails with NotReady:
   - Ensure `bluetoothd` is running and adapter is present; check rfkill unblock and dmesg for firmware errors.

4) If StartDiscovery returns but no devices appear:
   - Use `gdbus monitor --system --dest org.bluez` while scanning to watch InterfacesAdded for `org.bluez.Device1`.

Quick fix checklist
--------------------
- [ ] bluetoothd running (check log)
- [ ] rfkill unblocked
- [ ] /usr/share/dbus-1/system.d/org.bluealsa.conf present (or system policy allows bluealsa)
- [ ] /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules exists and polkitd reloaded
- [ ] GUI user in 'audio' (and 'bluetooth' when present) group
- [ ] Run GUI from terminal to capture errors

Expected error classes and remedies
----------------------------------
- NotAuthorized / org.freedesktop.DBus.Error.AccessDenied:
   * polkit rule missing or not applied. Ensure file exists at [`/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules`](contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules:1) and reload polkit.

- NoSuchInterface / org.freedesktop.DBus.Error.UnknownMethod:
   * BlueZ API mismatch (older/newer). Ensure BlueZ >= 5 supporting Adapter1 methods.

- NoAdapterFound:
   * Use `gdbus call` to inspect managed objects and adapt code if adapter path differs.

How I changed the code
----------------------
- I added an async completion callback in [`src/gui_bt.c`](src/gui_bt.c:1) named `discovery_call_done` that logs D-Bus call failures and schedules a `refresh_adapter_state()` invocation on the main loop. This surfaces `StartDiscovery`/`StopDiscovery` errors which were previously ignored.

Follow-up recommendations (MCP context7 best practices)
------------------------------------------------------
- Keep GUI D-Bus calls async and always provide completion callbacks that log or surface errors to the UI.
- Avoid constructing complex GVariant discovery filters in the GUI; do that in init/provisioning scripts or the daemon to avoid GLib version issues.
- When packaging, ship compiled artifacts per-architecture or provide a reproducible build script (separate builds for GUI and daemon).

If you want, I will:
- Produce a small shell script `contrib/bt-debug.sh` that runs the recommended checks and prints a one-line success/failure summary.
- Provide the exact `gcc` commands adjusted for your environment (e.g., paths to pkg-config) and any linker flags if compilation fails.

End of plan
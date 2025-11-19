Jack Bridge - System-wide ALSA + JACK Audio Script With No Systemd, Pulseaudio or Pipewire

This repository contains a script that sets up system-wide audio on Linux debian based systems using JACK and ALSA — without systemd, PulseAudio, or PipeWire. It is designed for users who want a minimal audio stack driven by JACK and ALSA.

Important: it is recommended to remove PulseAudio and/or PipeWire before running this script to avoid conflicts.

What this does

Installs and configures a system-wide JACK + ALSA setup so applications using ALSA can be routed through JACK.

Uses ALSA for device access and volume control (no PulseAudio/PipeWire).

Uses the following packages (these will be installed by the installer):
   jackd
   alsa-utils
   libasound2-plugins
   apulse
   qjackctl
   libasound2-plugin-equal
   swh-plugins
    libgtk-3-0

Volume control (GUI included) 

Volume, record in mono or stereo 41000Hz or 48000Hz and a system-wide equalizer are provided by the bundled GUI "AlsaTune" (AlsaTune is installed to /usr/local/bin/mxeq and a desktop launcher is added as "AlsaTune - sound settings"). After running the installer and rebooting the system, users will not need to use the terminal for volume or EQ — simply launch "AlsaTune - sound settings" from your desktop menu to:

Adjust mixer channels with sliders (no terminal required).
Adjust the 10-band equalizer in real time.
Save and apply EQ presets per-user.

Notes:
This setup routes ALSA applications through the ALSA equalizer plugin and then into JACK so EQ controls in the GUI affect ALSA apps without changing native JACK clients.

Ensure PulseAudio / PipeWire are not running (the installer attempts to disable PulseAudio autospawn). A reboot is recommended after installation to ensure JACK and ALSA device routing are active.

Installation

Clone the repo, make the install script executable, and run it:

Clone the repo:

gitclone https://github.com/rations/jack-bridge.git

Make the installer executable:

sudo chmod +x ./contrib/install.sh

Run the installer:

sudo ./contrib/install.sh

sudo reboot

The installer will install the packages listed above and configure system files under /etc and helper scripts under /usr/local/bin as provided in the contrib/ directory.

Uninstall

To remove what the installer added, run the provided uninstaller:

sudo ./contrib/uninstall.sh

Notes and thanks

This setup intentionally avoids systemd user units, PulseAudio, and PipeWire — it is meant for systems where a lightweight, JACK-first audio stack is desired.

Big thanks to JACK, ALSA and AlsaTune — this script relies on the JACK audio server,
ALSA userland to provide system-wide audio functionality and AlsaTune by mrgreenjeans for the GUI https://sourceforge.net/projects/vuu-do/files/Miscellaneous/apps/AlsaTune/

More in the works

Add support to the install.sh script for other GNU/Linux distros

Bluetooth support

Bluetooth integration (BlueALSA + BlueZ, no systemd/PulseAudio/PipeWire)

Overview
- This project adds Bluetooth audio by integrating BlueZ (bluetoothd) and BlueALSA (bluealsad) into the existing ALSA + JACK stack. No systemd, no PulseAudio, no PipeWire.
- The GUI (AlsaTune) provides Bluetooth controls: Scan, Pair, Trust, Connect, Remove, plus a Bluetooth latency control that maps directly to the bridge config.

Prerequisites
- Packages (installed by the installer):
  - bluez, bluez-tools, dbus, policykit-1
  - alsa-utils, libasound2-plugins, jackd2, qjackctl, etc.
- Users should be members of groups:
  - audio (always)
  - bluetooth (recommended where present; installer attempts to add desktop users)
- BlueALSA runtime:
  - bluealsa user (nologin)
  - /var/lib/bluealsa owned by bluealsa:bluealsa with 0700

Services and start order (SysV init, no systemd)
- The installer registers SysV init scripts. Required-Start/Stop headers handle sequencing:
  1) dbus (distro provided)
  2) bluetoothd (contrib/init.d/bluetoothd)
  3) bluealsad (contrib/init.d/bluealsad)
  4) jackd-rt (contrib/init.d/jackd-rt)
  5) jack-bluealsa-autobridge (contrib/init.d/jack-bluealsa-autobridge)
- bluetoothd is run with “-n” (foreground) so start-stop-daemon reliably manages it.
- bluealsad runs as the bluealsa user on the system bus.

D-Bus and polkit policies
- BlueALSA D-Bus policy (org.bluealsa):
  - Installed to: /usr/share/dbus-1/system.d/org.bluealsa.conf
  - Grants org.bluealsa ownership to user “bluealsa” and allows the “audio” group to use the mixer/PCMs.
- Polkit rule for BlueZ (non-root Pair/Connect/Adapter ops):
  - Installed to: /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules
  - Authorizes users in groups “audio” or “bluetooth” to:
    - Set Adapter Powered/Discoverable/Pairable
    - Pair/Connect/Trust/Disconnect Device1
    - Remove Device from Adapter
- The installer and provisioning script attempt to reload dbus and polkit (best-effort) after installing these policies.

GUI behavior (AlsaTune)
- The Bluetooth panel is collapsed by default. “Scan” starts discovery and “Stop” stops it. Known (cached) devices appear even before scanning and are marked with a star prefix “★”.
- Buttons are gated by live device state (where available):
  - Pair: enabled if not already paired
  - Trust: enabled if paired and not yet trusted
  - Connect: enabled if paired
  - Remove: enabled if an item is selected
- Adapter state (Powered/Discovering) is tracked to enable/disable Scan/Stop accordingly.

Latency control
- The Bluetooth panel includes:
  - “Bluetooth latency (period frames)” slider: 128..1024
  - “nperiods” spin control: 2..4
- These controls write atomically to the bridge config:
  - /etc/jack-bridge/bluetooth.conf
  - Keys written: BRIDGE_PERIOD, BRIDGE_NPERIODS plus canonical A2DP_PERIOD, A2DP_NPERIODS
- The autobridge daemon supports aliases:
  - BRIDGE_PERIOD → A2DP_PERIOD
  - BRIDGE_NPERIODS → A2DP_NPERIODS
- On change, the GUI sends SIGHUP to the autobridge so the new settings apply without reboot.

Autobridge (BlueALSA → JACK)
- Daemon: jack-bluealsa-autobridge
- Reads config: /etc/jack-bridge/bluetooth.conf
- Subscribes to BlueALSA D-Bus and spawns:
  - alsa_in for A2DP sink (phone → us)
  - alsa_out for A2DP source (us → headset)
  - alsa_in for SCO where applicable
- Prevents duplicate spawns per MAC and respects availability heuristics.
- Log: /var/log/jack-bluealsa-autobridge.log
- PID: /var/run/jack-bluealsa-autobridge.pid (GUI sends SIGHUP here on latency changes)

Troubleshooting
- Discovery finds nothing:
  - Ensure adapter is not blocked by rfkill:
    - rfkill list
    - rfkill unblock bluetooth
  - Make sure bluetoothd is running:
    - service bluetoothd status
  - The GUI powers the adapter on demand; if Polkit denies, verify the rule exists and groups are correct.
- Cannot pair/connect/trust:
  - Check the polkit rule file:
    - /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules
  - Ensure your user is in groups:
    - id -nG | grep -E '\baudio\b'
    - id -nG | grep -E '\bbluetooth\b'  (if group exists)
  - Verify BlueZ device is present:
    - bluetoothctl show
    - bluetoothctl devices
- BlueALSA audio not bridging to JACK:
  - Check autobridge log:
    - tail -f /var/log/jack-bluealsa-autobridge.log
  - Confirm BlueALSA PCM is present (after connecting):
    - bluealsactl status  (if provided)
  - Confirm JACK is running and autoconnect script exists:
    - /usr/lib/jack-bridge/jack-autoconnect
- Latency changes do not apply:
  - Verify that GUI updated:
    - /etc/jack-bridge/bluetooth.conf has BRIDGE_PERIOD/BRIDGE_NPERIODS (and A2DP_* equivalents)
  - Confirm autobridge PID and that SIGHUP was delivered:
    - cat /var/run/jack-bluealsa-autobridge.pid
    - kill -HUP $(cat /var/run/jack-bluealsa-autobridge.pid)

Notes
- This stack avoids systemd, PulseAudio, and PipeWire by design.
- BlueALSA supports A2DP/HFP/HSP classic profiles; BLE Audio profiles are not supported.
- The GUI is designed so end users do not need to use the terminal after installation.

Changelog highlights (Bluetooth)
- Added adapter resolution/power-up, discovery filter, and properties subscriptions in [gui_bt.c](src/gui_bt.c:1)
- Added star “★” tag for Known (cached) devices and state markers [Paired]/[Trusted]/[Connected]
- Added Bluetooth latency slider and nperiods spinner in [mxeq.c](src/mxeq.c:1), mapped to [/etc/jack-bridge/bluetooth.conf](etc/jack-bridge/bluetooth.conf:1)
- Added config alias support for BRIDGE_PERIOD/BRIDGE_NPERIODS in [jack-bluealsa-autobridge.c](src/jack-bluealsa-autobridge.c:1)
- Provided polkit rule and D-Bus policy installation; ensured bluetoothd foreground mode and proper bluealsad defaults


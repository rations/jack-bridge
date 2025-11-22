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

The installer will install the packages listed above and configure system files under /etc, helper scripts under /usr/local/lib/jack-bridge, and binaries under /usr/local/bin as provided in the contrib/ directory.

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
- bluetoothd is managed by the distro; this installer does not install or modify a bluetoothd init script.
- Optional: installs contrib/init.d/bluealsad if present and registers it after bluetoothd.
- Installs contrib/init.d/jackd-rt.
- Desired order: dbus (distro) -> bluetoothd (distro) -> bluealsad (optional) -> jackd-rt.
- Also installs a small post-boot helper (/etc/init.d/jack-bridge-bluetooth-config) that sets the adapter Discoverable/Pairable (best-effort).

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
- Values are written atomically to /etc/jack-bridge/devices.conf:
  - Keys: BT_PERIOD, BT_NPERIODS
- They are applied when Bluetooth is selected as the output in the Devices panel. The routing helper [/usr/local/lib/jack-bridge/jack-route-select](contrib/usr/local/lib/jack-bridge/jack-route-select:1) passes -p BT_PERIOD and -n BT_NPERIODS to alsa_out for the bt_out JACK client.

Device switching (Internal / USB / HDMI / Bluetooth)
- Change the active audio target at runtime without restarting JACK.
- The GUI “Devices” panel provides radio buttons for Internal, USB, HDMI, and Bluetooth.
- Under the hood, the helper [/usr/local/lib/jack-bridge/jack-route-select](contrib/usr/local/lib/jack-bridge/jack-route-select:1) rewires JACK ports and spawns alsa_out clients as needed (out_usb, out_hdmi, bt_out).
- The selection and device mappings are persisted in [/etc/jack-bridge/devices.conf](etc/jack-bridge/devices.conf:1).

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
- No audio on Bluetooth target:
  - Ensure the bt_out JACK client exists:
    - jack_lsp | grep '^bt_out:'
  - If missing, select Bluetooth in the Devices panel or run:
    - /usr/local/lib/jack-bridge/jack-route-select bluetooth
  - Confirm BlueALSA is running (bluealsad) and the device is connected in the GUI.
  - Verify [/etc/jack-bridge/devices.conf](etc/jack-bridge/devices.conf:1) has BLUETOOTH_DEVICE (default: bluealsa:PROFILE=a2dp).
- Latency changes do not apply:
  - Verify BT_PERIOD/BT_NPERIODS in [/etc/jack-bridge/devices.conf](etc/jack-bridge/devices.conf:1).
  - Re-select Bluetooth in the Devices panel or run:
    - /usr/local/lib/jack-bridge/jack-route-select bluetooth
  - This respawns bt_out with the new -p/-n settings.

Notes
- This stack avoids systemd, PulseAudio, and PipeWire by design.
- BlueALSA supports A2DP/HFP/HSP classic profiles; BLE Audio profiles are not supported.
- The GUI is designed so end users do not need to use the terminal after installation.

Changelog highlights (Bluetooth)
- Fixed discovery crash and Properties.Set signature issues in [gui_bt.c](src/gui_bt.c:1)
- Added Devices panel and runtime routing via [jack-route-select](contrib/usr/local/lib/jack-bridge/jack-route-select:1); persisted preferences in [/etc/jack-bridge/devices.conf](etc/jack-bridge/devices.conf:1)
- Moved Bluetooth latency to BT_PERIOD/BT_NPERIODS in devices.conf; applied to bt_out via alsa_out
- Removed autobridge and bluetooth.conf; installer/uninstaller now clean legacy artifacts
- Provided polkit rule and D-Bus policy installation; proper bluealsad defaults


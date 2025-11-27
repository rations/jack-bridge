Provisioning and runtime notes for BlueALSA integration (jack-bridge)
====================================================================

Purpose
-------
This document explains the provisioning actions required to run BlueALSA and integrate Bluetooth audio into JACK
on systemd-free Debian-like systems. Routing is handled in JACK via the helper /usr/local/lib/jack-bridge/jack-route-select
and settings in /etc/jack-bridge/devices.conf. The provided helper script `contrib/setup-bluetooth.sh` automates common
provisioning steps; this file documents what to verify manually.

Summary of actions performed by contrib/setup-bluetooth.sh
---------------------------------------------------------
1. Ensure system user
   - Creates a system user `bluealsa` with no login shell if it does not already exist:
     `useradd --system --no-create-home --shell /usr/sbin/nologin --user-group bluealsa`
   - Running as a dedicated user reduces attack surface and keeps state files owned by that user.

2. Create persistent state directory
   - Creates `/var/lib/bluealsa` if missing and sets ownership to `bluealsa:bluealsa` and permissions `0700`.
   - This directory stores BlueALSA persistent state and must be protected.

3. Devices config and routing helper
   - The installer writes `/etc/jack-bridge/devices.conf` with defaults:
     - INTERNAL_DEVICE, USB_DEVICE, HDMI_DEVICE, BLUETOOTH_DEVICE
     - BT_PERIOD, BT_NPERIODS
     - PREFERRED_OUTPUT
   - Runtime routing is performed by `/usr/local/lib/jack-bridge/jack-route-select` which rewires JACK ports and spawns
     alsa_out for USB/HDMI/Bluetooth targets as needed. The GUI Devices panel calls this helper; no separate daemon required.

4. Install D-Bus policy
   - If `usr/share/dbus-1/system.d/org.bluealsa.conf` exists in the repo, it is copied to
     `/usr/share/dbus-1/system.d/org.bluealsa.conf` with mode `0644`. This policy allows the
     `bluealsa` user to own the `org.bluealsa` D-Bus name and permits members of the `audio`
     group to use the service.
   - NOTE: `usr/share/dbus-1/system.d` is a D‑Bus system-bus policy directory (XML files)
     used by the system D-Bus daemon to control which users/groups can own or talk to
     particular D-Bus names. This is NOT related to systemd. The presence of this file
     in the repository simply provides an optional, ready-made D-Bus policy that the
     provisioning script can copy into the system D-Bus policy directory on systems
     where you want the `bluealsa` user to be permitted to own `org.bluealsa`.

5. Add target user to audio group
   - The script adds a target user (passed as an argument or auto-detected) to the `audio` group so
     they can access ALSA devices and BlueALSA mixers without root.

6. Create GUI module stubs
   - Creates placeholder C files:
     - `src/bt_agent.c`
     - `src/gui_bt.c`
   - These are stubs intended to be expanded with the BlueZ Agent (pairing) and GUI controls.

Manual verification checklist
-----------------------------
- Confirm `bluealsa` user exists:
  id bluealsa

- Confirm /var/lib/bluealsa:
  ls -ld /var/lib/bluealsa
  Should be owned by bluealsa:bluealsa and mode 0700.

- Confirm D-Bus policy present:
  ls -l /usr/share/dbus-1/system.d/org.bluealsa.conf

- Confirm devices configuration:
  ls -l /etc/jack-bridge/devices.conf
  cat /etc/jack-bridge/devices.conf

- Confirm target user in audio group:
  id <your-user>   # should show 'audio' in groups

Integration notes
-----------------
- The init scripts provided in contrib/init.d/ are SysV-style and should be installed by package maintainer or copied
  into `/etc/init.d/` and enabled via `update-rc.d` or equivalent for your distribution.
- There is no autobridge daemon. Routing is handled by JACK using `/usr/local/lib/jack-bridge/jack-route-select`
  and preferences in `/etc/jack-bridge/devices.conf`.
- No systemd, PulseAudio, or PipeWire are required. The provisioning script and init scripts are written to work on sysvinit-like systems.

Troubleshooting
---------------
- If BlueALSA does not appear to register on the system bus, confirm `dbus-daemon` is running and that the D-Bus policy
  is present. Check `/var/log/syslog` or `journalctl` (if available).
- If permissions prevent `bluealsad` from owning `org.bluealsa`, ensure the policy file enables the `bluealsa` user and the daemon
  is started with that user.
- For diagnosing routing: verify JACK ports and helper operation
  - jack_lsp | egrep '^(alsa_pcm:playback_|out_usb:|out_hdmi:|bt_out:)'
  - Re-route explicitly: /usr/local/lib/jack-bridge/jack-route-select bluetooth
  - Inspect /etc/jack-bridge/devices.conf for BLUETOOTH_DEVICE and latency (BT_PERIOD/BT_NPERIODS).

Security considerations
-----------------------
- Keep the `bluealsa` account non-login (shell `/usr/sbin/nologin`) and avoid adding unnecessary privileges.
- Restrict `/var/lib/bluealsa` permissions to `0700` to avoid accidental state disclosure.

Building from source — exact commands used to produce contrib/bin/* (bluealsa binaries)
--------------------------------------------------------------------------------------

Note: the upstream BlueALSA project uses the GNU Autotools workflow. The commands below are the recommended sequence to build the BlueALSA daemon and client utilities (these are the same artifacts that may be shipped in [`contrib/bin/bluealsad`](contrib/bin/bluealsad:1) and friends). See also [`bluealsa-INSTALL.md`](bluealsa-INSTALL.md:1) for detailed dependency information.

Prepare the source tree (run from the repository top-level)
- Install development tools and dependencies (example package names for Debian/Ubuntu):
  - build-essential, autoconf, automake, libtool, pkg-config, git
  - libasound2-dev (ALSA), libbluetooth-dev (BlueZ), libglib2.0-dev (GLib/GIO), libdbus-1-dev
  - sbc (or libopen- in some distros), libreadline-dev (for rfcomm), and optional codec libs (fdk-aac, libldac, libopenaptx, opus, lame, lc3)
- Generate configure (when building from git):
  autoreconf --install

Configure & build (out-of-source build recommended)
mkdir build && cd build
../configure --prefix=/usr/local [OPTIONS]
make

Recommended options for sysvinit (non-systemd) systems:
- Do NOT enable systemd unit files: do not pass --enable-systemd
- To include the RFCOMM tool: pass --enable-rfcomm
- To set a non-root runtime user (useful when running without systemd):
  ../configure --prefix=/usr/local --with-bluealsaduser=bluealsa --enable-rfcomm

Exact example (run from repo top):
autoreconf --install
mkdir build && cd build
../configure --prefix=/usr/local --with-bluealsaduser=bluealsa --enable-rfcomm
make

Install (optional)
- Install to the system:
  sudo make install
- Or stage into a directory for packaging:
  sudo make DESTDIR=$(pwd)/BLUEALSA install

Per-binary minimal compile commands (useful for producing a single utility if you have the relevant source file)
- These are minimal gcc commands (assume src/ contains the single utility source and pkg-config is available):

- bluealsactl (requires GLib/GIO and D-Bus):
  gcc -Wall -Wextra -o contrib/bin/bluealsactl src/bluealsactl.c $(pkg-config --cflags --libs glib-2.0 gio-2.0 dbus-1)

- bluealsa-aplay (requires GLib/GIO, D-Bus, and ALSA):
  gcc -Wall -Wextra -o contrib/bin/bluealsa-aplay src/bluealsa-aplay.c $(pkg-config --cflags --libs glib-2.0 gio-2.0 dbus-1 alsa)

- bluealsa-rfcomm (requires GLib/GIO, D-Bus, and readline):
  gcc -Wall -Wextra -o contrib/bin/bluealsa-rfcomm src/bluealsa-rfcomm.c $(pkg-config --cflags --libs glib-2.0 gio-2.0 dbus-1) -lreadline

- bluealsad (daemon)
  - The daemon links multiple internal sources and should be built with the Autotools workflow (no supported single-file gcc command). Use the example configure+make sequence above.

Notes for non-systemd systems (runtime)
- Do NOT enable systemd unit files when building for sysvinit-like targets. The provisioning and init scripts in this repo are designed for systemd-free systems:
  - SysV init script: [`contrib/init.d/bluealsad`](contrib/init.d/bluealsad:1)
  - Provisioning helper: [`contrib/setup-bluetooth.sh`](contrib/setup-bluetooth.sh:1)
  - Installer will install init scripts and register them with `update-rc.d` if available (see [`contrib/install.sh`](contrib/install.sh:336)).
- After installing the binaries, start the daemon using the provided init script or your distribution's init tooling:
  sudo install -m 0755 contrib/init.d/bluealsad /etc/init.d/bluealsad
  sudo update-rc.d bluealsad defaults
  sudo service bluealsad start

References:
- Build/dependency reference: [`bluealsa-INSTALL.md`](bluealsa-INSTALL.md:1)
- Installer behavior (prebuilt copy): [`contrib/install.sh`](contrib/install.sh:336)
- Prebuilt artifacts location: [`contrib/bin/bluealsad`](contrib/bin/bluealsad:1)

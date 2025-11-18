Provisioning and runtime notes for BlueALSA integration (jack-bridge)
====================================================================

Purpose
-------
This document explains the provisioning actions required to run BlueALSA and the jack-bluealsa-autobridge
on systemd-free Debian-like systems. The provided helper script `contrib/setup-bluetooth.sh` automates
the common steps, but this file documents what it does and what to verify manually.

Summary of actions performed by contrib/setup-bluetooth.sh
---------------------------------------------------------
1. Ensure system user
   - Creates a system user `bluealsa` with no login shell if it does not already exist:
     `useradd --system --no-create-home --shell /usr/sbin/nologin --user-group bluealsa`
   - Running as a dedicated user reduces attack surface and keeps state files owned by that user.

2. Create persistent state directory
   - Creates `/var/lib/bluealsa` if missing and sets ownership to `bluealsa:bluealsa` and permissions `0700`.
   - This directory stores BlueALSA persistent state and must be protected.

3. Ensure log file
   - Ensures `/var/log/jack-bluealsa-autobridge.log` exists and is writable.
   - Ownership is set to `bluealsa:bluealsa` where possible.

4. Install D-Bus policy
   - If `usr/share/dbus-1/system.d/org.bluealsa.conf` exists in the repo, it is copied to
     `/usr/share/dbus-1/system.d/org.bluealsa.conf` with mode `0644`. This policy allows the
     `bluealsa` user to own the `org.bluealsa` D-Bus name and permits members of the `audio`
     group to use the service.

5. Add target user to audio group
   - The script adds a target user (passed as an argument or auto-detected) to the `audio` group so
     they can access ALSA devices and BlueALSA mixers without root.

6. Create GUI module stubs
   - Creates placeholder C files:
     - `src/bt_agent.c`
     - `src/gui_bt.c`
     - `src/bt_bridge.c`
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

- Confirm autobridge log file:
  ls -l /var/log/jack-bluealsa-autobridge.log

- Confirm target user in audio group:
  id <your-user>   # should show 'audio' in groups

Integration notes
-----------------
- The init scripts provided in contrib/init.d/ are SysV-style and should be installed by package maintainer or copied
  into `/etc/init.d/` and enabled via `update-rc.d` or equivalent for your distribution.
- The autobridge daemon binary is expected at `/usr/local/bin/jack-bluealsa-autobridge` by the init script.
  Ensure the binary is installed there and executable by the `jack` runtime user configured in `/etc/default/` files.
- No systemd, PulseAudio, or PipeWire are required. The provisioning script and init scripts are written to work on sysvinit-like systems.

Troubleshooting
---------------
- If BlueALSA does not appear to register on the system bus, confirm `dbus-daemon` is running and that the D-Bus policy
  is present. Check `/var/log/syslog` or `journalctl` (if available).
- If permissions prevent `bluealsad` from owning `org.bluealsa`, ensure the policy file enables the `bluealsa` user and the daemon
  is started with that user.
- For diagnosing bridge launches, consult `/var/log/jack-bluealsa-autobridge.log` (autobridge) and the JACK log at the configured path.

Security considerations
-----------------------
- Keep the `bluealsa` account non-login (shell `/usr/sbin/nologin`) and avoid adding unnecessary privileges.
- Restrict `/var/lib/bluealsa` permissions to `0700` to avoid accidental state disclosure.

If you want, I will:
- Harden the provisioning script further (validate binary paths, reload dbus policy, check installed package versions).
- Implement the PCM property parsing to finalize autobridge spawn behavior.
- Implement the GUI BlueZ Agent and pairing UI inside the existing C GUI.

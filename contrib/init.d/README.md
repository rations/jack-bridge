# SysV init scripts (not binaries)

This directory contains SysV-style init scripts that get installed into /etc/init.d by the installer. These are not executables to be placed in contrib/bin, and they are not duplicates of the shipped binaries. Each script controls a different system service:

- bluealsad: starts the BlueALSA daemon (bluealsad) on the system D-Bus
  - Script: contrib/init.d/bluealsad
  - Binary it controls: /usr/local/bin/bluealsad or /usr/bin/bluealsad (installed from contrib/bin/bluealsad)
  - Defaults (optional): /etc/default/bluealsad

- bluetoothd: starts the BlueZ bluetoothd daemon at boot
  - Script: contrib/init.d/bluetoothd
  - Binary it controls: /usr/sbin/bluetoothd (provided by the distro)
  - This should remain here as an init script; it must not be moved to contrib/bin

- jackd-rt: starts JACK (jackd) with realtime tuning
  - Script: contrib/init.d/jackd-rt
  - Binary it controls: /usr/bin/jackd (provided by the distro)
  - Defaults: contrib/default/jackd-rt -> /etc/default/jackd-rt

- jack-bluealsa-autobridge: runs the autobridge daemon after JACK and BlueALSA are ready
  - Script: contrib/init.d/jack-bluealsa-autobridge
  - Binary it controls: /usr/local/bin/jack-bluealsa-autobridge (installed from contrib/bin/jack-bluealsa-autobridge)

Directory responsibilities (summary)

- src/: all C sources (e.g. src/mxeq.c, src/gui_bt.c, src/jack-bluealsa-autobridge.c)
- contrib/bin/: shipped prebuilt executables for end users (mxeq, jack-bluealsa-autobridge, bluealsad, bluealsactl, bluealsa-aplay, bluealsa-rfcomm)
- contrib/init.d/: SysV init scripts (service control), installed into /etc/init.d by contrib/install.sh
- contrib/etc/: configuration templates installed into /etc at install
- contrib/default/: defaults files for the init scripts, installed into /etc/default by contrib/install.sh

Why bluetoothd should not be moved to contrib/bin

- contrib/bin is for prebuilt executables we ship. The bluetoothd file in this directory is an init script (a shell script with LSB headers) that is intended to be placed in /etc/init.d, not executed directly by users as a standalone binary.
- Moving contrib/init.d/bluetoothd into contrib/bin would break install semantics and confuse the packaging layout. The installer expects to copy this script into /etc/init.d and register it with update-rc.d.

What looked “duplicated” before

- The previous duplication you noticed was top-level bin/ vs contrib/bin/, which we’ve resolved by removing the top-level bin and keeping contrib/bin as canonical.
- In the installer, there were duplicated blocks; we cleaned that so that autobridge is registered only once and with a sane guard.

Keep these invariants

- Binaries build from src/ -> shipped (if any) in contrib/bin
- Init scripts remain in contrib/init.d
- Installer [contrib/install.sh] places scripts into /etc/init.d and binaries into /usr/local/bin
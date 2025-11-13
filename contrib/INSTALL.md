INSTALL: jack-bridge contrib
============================

Purpose
-------
This contrib bundle provides a minimal, non-invasive way to route ALSA applications into JACK on Debian-like systems without requiring systemd, PulseAudio, or PipeWire. It uses the ALSA JACK plugin as the system default and a SysV-style init script to run classic jackd with realtime tuning. Browser PulseAudio clients are supported via apulse wrappers.

Included files (templates & scripts)
- contrib/etc/asound.conf            - ALSA -> JACK plugin template (pcm.jack + pcm.!default)
- contrib/init.d/jackd-rt           - SysV init script (auto-detects user/device, reads /etc/default/jackd-rt)
- contrib/default/jackd-rt          - optional overrides (JACKD_USER, JACKD_DEVICE, JACKD_SR, etc.)
- contrib/usr/lib/jack-bridge/detect-alsa-device.sh - helper to pick a full-duplex ALSA device
- contrib/install.sh                - installer script (non-destructive default behavior)
- contrib/TROUBLESHOOTING.md        - troubleshooting & packaging notes
- contrib/etc/security/limits.d/audio.conf - template RT limits for 'audio' group
- apulse wrappers installed by install.sh: /usr/bin/apulse-firefox, /usr/bin/apulse-chromium

Design principles
-----------------
- Do not hardcode usernames, device names, or system paths that vary per-installation.
- Provide safe defaults and allow administrators to override via /etc/default/jackd-rt.
- Be non-destructive: the installer will not overwrite an existing /etc/asound.conf by default.
- Prefer CARD names (hw:CARD=...) when possible to reduce index-based device brittle behavior.
- Document required packages and diagnostic steps clearly for packagers/admins.

Quick install (admin steps)
---------------------------
1) Ensure required system packages are installed:
   - sudo apt-get install jackd2 alsa-utils alsa-plugins apulse
   - Optional: qjackctl (for monitoring only)
2) Run the installer as root:
   - sudo sh contrib/install.sh
   The installer copies templates into /etc, installs init script into /etc/init.d, helper into /usr/lib/jack-bridge, and apulse wrappers into /usr/bin. It registers the init script via update-rc.d when available.
3) Review (and optionally merge) /etc/asound.conf:
   - If you have an existing /etc/asound.conf, merge the pcm.jack and pcm.!default entries rather than blindly overwriting.
4) Configure realtime privileges:
   - Copy the template or merge into /etc/security/limits.d/audio.conf. Example lines:
       @audio - rtprio 95
       @audio - memlock unlimited
       @audio - nice -10
   - Ensure desktop users are in the 'audio' group: sudo usermod -aG audio alice
5) Start the service:
   - sudo service jackd-rt start
   - Check status: sudo service jackd-rt status
6) Validate the ALSA→JACK path:
   - Run: aplay -D default /usr/share/sounds/alsa/Front_Center.wav
   - Inspect JACK ports: jack_lsp (should show system:playback_1 and system:capture_1 and client ports created when apps play)
7) Use browser wrappers:
   - Start browser via apulse wrapper: apulse-firefox or apulse-chromium to route web audio into ALSA -> JACK.

Override examples
-----------------
To pin jackd to a specific ALSA device or run as a specific user, edit /etc/default/jackd-rt (created from contrib/default/jackd-rt) and uncomment the lines:
- JACKD_USER="alice"
- JACKD_DEVICE="hw:0"
- JACKD_SR=48000
- JACKD_PERIOD=256
- JACKD_NPERIODS=3

Uninstall guidance
------------------
The installer is non-destructive by default. To remove installed files manually:
- sudo update-rc.d -f jackd-rt remove
- sudo rm -f /etc/init.d/jackd-rt /etc/default/jackd-rt
- sudo rm -f /etc/asound.conf (only if you replaced it with the template and want to restore previous)
- sudo rm -rf /usr/lib/jack-bridge
- sudo rm -f /usr/bin/apulse-firefox /usr/bin/apulse-chromium

Packaging notes for maintainers
-------------------------------
- Do not overwrite existing /etc/asound.conf on install; instead ship the template and instruct admins to merge.
- Follow distro packaging norms: place helper scripts into /usr/lib/jack-bridge and the init script into /etc/init.d. Provide proper postinst/prerm scripts to register/unregister the init script.
- Provide documentation that desktop users must be in group 'audio' and that limits.d must grant realtime privileges.
- Consider providing systemd unit equivalents in a separate contrib/systemd/ directory for distributions using systemd (optional; this project targets non-systemd setups).

Further improvements (future)
----------------------------
- Provide an uninstall.sh that reverts installed files and unregisters the init script.
- Provide an automated validation script that starts the service, plays a test tone and checks jack_lsp for expected ports.
- Optional CI smoke test that boots a minimal VM/container, installs packages, starts service, and asserts ALSA->JACK behavior.

Contact / best practices
------------------------
This contrib follows contact7 best practices: non-invasive defaults, clear override mechanism, minimal footprint, and thorough troubleshooting documentation.

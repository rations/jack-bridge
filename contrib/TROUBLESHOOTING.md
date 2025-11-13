TROUBLESHOOTING: jack-bridge contrib
====================================

This document helps diagnose common failures when using the contrib package:
- contrib/etc/asound.conf
- contrib/init.d/jackd-rt
- contrib/default/jackd-rt
- contrib/usr/lib/jack-bridge/detect-alsa-device.sh
- contrib/install.sh
- apulse wrappers (apulse-firefox, apulse-chromium)

Before you start
----------------
1. Ensure required packages are installed on Debian-like systems:
   - jackd2, alsa-utils (aplay/arecord), alsa-plugins (ALSA JACK plugin), apulse optional
   - start-stop-daemon (usually in dpkg/system base) and update-rc.d for registering init scripts
2. Confirm JACK runs as a regular desktop user (not root) with realtime privileges:
   - User should be in 'audio' group.
   - /etc/security/limits.d/audio.conf should grant @audio rtprio and memlock.
3. Use the provided contrib/install.sh to deploy files and register init script.

Symptoms and fixes
------------------

1) No JACK ports appear when an ALSA app plays
- Cause: ALSA app is not using ALSA (e.g., it uses PulseAudio directly).
- Check:
  - Run an ALSA test: aplay -D default /usr/share/sounds/alsa/Front_Center.wav
  - Run: aplay -L and confirm "jack" or "default" points to ALSA JACK plugin if /etc/asound.conf is in place.
- Fix:
  - Ensure /etc/asound.conf exists and matches contrib/etc/asound.conf. If an administrator intentionally configured a different /etc/asound.conf, merge into that file instead of overwriting.
  - For applications that use PulseAudio, use the apulse wrappers (apulse-firefox / apulse-chromium) to force ALSA usage.

2) Browser plays but no sound is heard
- Cause: Browser still targeting PulseAudio server not present, or apulse missing.
- Check:
  - Run the browser from terminal via wrapper: apulse-firefox
  - Inspect JACK ports: use jack_lsp, or qjackctl > Graph to see new client ports.
- Fix:
  - Install apulse package (if available) and use provided apulse wrappers.
  - If the browser uses sandboxing that prevents apulse, try launching a non-sandboxed profile or use a browser build that links ALSA.

3) jackd won't start from init script (permission denied / device busy / cannot claim hw)
- Cause: Wrong user, insufficient realtime permissions, another sound server holds device.
- Check:
  - Inspect service status: sudo service jackd-rt status
  - Check /var/log/syslog, /var/log/messages, or journal (if available).
  - Ensure no PulseAudio/pipewire daemon is auto-spawning and grabbing the hardware (ps aux | grep -E "pulseaudio|pipewire")
- Fix:
  - Ensure JACKD_USER override in /etc/default/jackd-rt if auto-detection picks incorrect user.
  - Ensure the chosen user is in 'audio' group and /etc/security/limits.d/audio.conf contains:
      @audio - rtprio 95
      @audio - memlock unlimited
  - Stop PulseAudio/pipewire auto-spawn:
      - For PulseAudio: echo "autospawn = no" > ~/.config/pulse/client.conf (or system config)
      - For PipeWire: disable pipewire-pulse service if present
  - If device is busy by ALSA, run fuser -v /dev/snd/* to find which process holds it.

4) Detect script picks wrong ALSA device (e.g., picks Loopback, incorrect USB device)
- Cause: Card naming or ordering changes; some USB devices show multiple function cards.
- Check:
  - Run aplay -l and arecord -l to inspect cards and devices.
  - Manually test with jackd using a device string: jackd -d alsa -d hw:1 -r 48000 -p 256 -n 3
- Fix:
  - Provide explicit override in /etc/default/jackd-rt: JACKD_DEVICE="hw:1" or JACKD_DEVICE="hw:CARD=DeviceName"
  - If device indices change across reboots, prefer CARD name if possible (hw:CARD=YourCardName)

5) XRuns and poor latency
- Cause: Frames/period too small for device/driver or CPU/IRQ contention.
- Check:
  - Use jack_iodelay or watch XRUN messages in qjackctl console.
  - Try increasing period size (e.g., -p 512) or reduce sample rate.
- Fix:
  - Tune /etc/default/jackd-rt: JACKD_PERIOD and JACKD_NPERIODS to sensible defaults for the hardware.
  - Ensure CPU governor is performance for low-latency scenarios (optional).
  - Ensure realtime priority granted via limits.d for @audio.

6) Installation script warnings about missing binaries
- Cause: Not all packages are installed.
- Check:
  - Which commands are missing: aplay, arecord, jackd, apulse, update-rc.d, start-stop-daemon.
- Fix:
  - Use apt to install: sudo apt-get install jackd2 alsa-utils alsa-plugins apulse
  - If apulse not available, consider packaging instructions in README for building apulse from source.

7) Conflict with existing /etc/asound.conf or distribution defaults
- Cause: Overwriting system config can break existing setups.
- Check:
  - If /etc/asound.conf exists, inspect it before replacing.
- Fix:
  - Prefer to merge or instruct admins to manually integrate pcm.jack entries into their existing config.
  - Installer currently skips overwriting an existing /etc/asound.conf to be non-destructive.

Diagnostics commands
--------------------
- jack_lsp          : list JACK ports/clients
- jack_control start/stop (if using jack1 tools) or service jackd-rt start
- aplay -D default <wav>  : play a wav through the default ALSA device (should route into JACK when /etc/asound.conf in place)
- aplay -l / arecord -l    : list ALSA playback/record devices
- ps aux | grep -E "pulseaudio|pipewire"
- fuser -v /dev/snd/*
- tail -n 200 /var/log/syslog

Packaging notes for maintainers
-------------------------------
- Do not hardcode usernames or devices in supplied files. Provide sensible defaults and allow /etc/default/jackd-rt overrides.
- Keep install.sh idempotent and non-destructive. Preserve existing /etc/asound.conf by default.
- Document that desktop users must be in 'audio' group and limits.d must be configured for realtime priorities.
- Provide clear troubleshooting steps for common desktop browsers that embed PulseAudio; recommend apulse wrappers and explain sandboxing caveats.

Appendix: Quick checklist to validate system
-------------------------------------------
1. sudo sh contrib/install.sh
2. Confirm /etc/asound.conf present and contains pcm.jack mapping
3. sudo service jackd-rt start
4. jack_lsp (should show system:playback_1 and system:capture_1 and clients created when apps play)
5. aplay -D default /usr/share/sounds/alsa/Front_Center.wav - sound should be heard
6. Run browser using apulse wrapper to verify web audio playback

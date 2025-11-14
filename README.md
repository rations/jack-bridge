# Jack Bridge - System-wide ALSA + JACK Audio Script With No Systemd, Pulseaudio or Pipewire

This repository contains a script that sets up system-wide audio on Linux debian based systems using JACK and ALSA — without systemd, PulseAudio, or PipeWire. It is designed for users who want a minimal audio stack driven by JACK and ALSA.

Important: it is recommended to remove PulseAudio and/or PipeWire before running this script to avoid conflicts.

## What this does

- Installs and configures a system-wide JACK + ALSA setup so applications using ALSA can be routed through JACK.
- Uses ALSA for device access and volume control (no PulseAudio/PipeWire).
- Uses the following packages (these will be installed by the installer):
  - jackd
  - alsa-utils
  - libasound2-plugins
  - apulse
  - qjackctl

## Volume control

Volume is controlled with ALSA. Open a terminal and run: alsamixer

Press Enter after typing the command to open the mixer UI to adjust volume

Still in the works GUI for volume control so the terminal isn't required

## Installation

Clone the repo, make the install script executable, and run it:

1. Clone the repo:
   
   gitclone https://github.com/rations/jack-bridge.git

2. Make the installer executable:

   sudo chmod +x ./contrib/install.sh

3. Run the installer:

   sudo ./contrib/install.sh

The installer will install the packages listed above and configure system files under `/etc` and helper scripts under `/usr/local/bin` as provided in the `contrib/` directory.

## Uninstall

To remove what the installer added, run the provided uninstaller:

sudo ./contrib/uninstall.sh

## Notes and thanks

- This setup intentionally avoids systemd user units, PulseAudio, and PipeWire — it is meant for systems where a lightweight, JACK-first audio stack is desired.
- Big thanks to JACK and ALSA — this script relies on the JACK audio server and ALSA userland to provide system-wide audio functionality.

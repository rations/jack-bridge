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



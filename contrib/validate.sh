#!/bin/sh
# contrib/validate.sh
# Simple validation script for the jack-bridge contrib setup.
# It attempts to:
#  1) ensure jackd-rt init script exists and start it
#  2) wait for JACK to be available
#  3) play a short test tone via ALSA default (which should route into JACK when /etc/asound.conf is installed)
#  4) list JACK ports to verify system:playback and client ports appear
#
# Run as root (to start service) or as a user with sudo:
# sudo sh contrib/validate.sh

set -e

JACKD_INIT="/etc/init.d/jackd-rt"
APLAY=$(command -v aplay || true)
JACK_LSP=$(command -v jack_lsp || true)

if [ ! -x "$JACKD_INIT" ]; then
    echo "Init script $JACKD_INIT not found. Ensure you ran contrib/install.sh or installed the init script."
    exit 2
fi

echo "Starting jackd-rt service (if not already running)..."
if command -v service >/dev/null 2>&1; then
    service jackd-rt start || true
else
    $JACKD_INIT start || true
fi

echo "Waiting up to 10 seconds for JACK to appear..."
# Wait for jack_lsp to list something, or for jackd daemon to be present
i=0
while [ $i -lt 10 ]; do
    if [ -n "$JACK_LSP" ] && $JACK_LSP >/dev/null 2>&1; then
        break
    fi
    sleep 1
    i=$((i+1))
done

if [ -z "$JACK_LSP" ]; then
    echo "Warning: jack_lsp not found in PATH. Install jackd2-tools (provides jack_lsp) to inspect JACK ports."
else
    echo "Listing JACK ports:"
    $JACK_LSP || true
fi

# Play a short test tone if aplay exists
if [ -n "$APLAY" ]; then
    # Generate a 1-second 440Hz sine using arecord/aplay via /dev/zero is tricky.
    # Use speaker-test to produce a tone if available, else try aplay with a small raw generated tone.
    if command -v speaker-test >/dev/null 2>&1; then
        echo "Playing a short stereo sine via speaker-test (2 seconds) to the default ALSA device..."
        # -t sine requires alsa-utils speaker-test >= 1.1; fallback to pink noise if not supported
        speaker-test -t sine -f 440 -c 2 -l 1 -D default >/dev/null 2>&1 || speaker-test -t wav -c 2 -l 1 -D default >/dev/null 2>&1 || true
        echo "Playback attempted. Listen for the tone."
    else
        # Try aplay with /usr/share/sounds/alsa/Front_Center.wav if present
        if [ -f /usr/share/sounds/alsa/Front_Center.wav ]; then
            echo "Playing Front_Center.wav via aplay..."
            $APLAY -D default /usr/share/sounds/alsa/Front_Center.wav >/dev/null 2>&1 || true
            echo "Playback attempted."
        else
            echo "No speaker-test or sample wav available to play. Install alsa-utils for full validation."
        fi
    fi
else
    echo "aplay not found; cannot play test tone. Install alsa-utils."
fi

echo ""
echo "Validation steps summary:"
echo " - Confirm jackd-rt started without fatal errors."
echo " - Use 'jack_lsp' to inspect ports; you should see system:playback_1 and system:capture_1 and client ports created when apps play."
echo " - If you hear nothing from the test tone, check /etc/asound.conf exists and contains the pcm.jack configuration (contrib/etc/asound.conf)."
echo " - Check for other audio servers (pulseaudio/pipewire) that may be grabbing the device: ps aux | grep -E 'pulseaudio|pipewire'"

echo "Done."
exit 0
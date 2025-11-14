#!/bin/sh
# contrib/install.sh
# Installer for jack-bridge contrib package on Debian-like systems (SysV-style).
# Installs configs into /etc, helper scripts into /usr/lib/jack-bridge, init script into /etc/init.d,
# and simple apulse wrappers into /usr/bin. Does not hardcode usernames or devices.
#
# Usage: sudo sh contrib/install.sh
set -e

PREFIX_ROOT="/"
ETC_DIR="${PREFIX_ROOT}etc"
USR_LIB_DIR="${PREFIX_ROOT}usr/lib/jack-bridge"
INIT_DIR="${PREFIX_ROOT}etc/init.d"
DEFAULTS_DIR="${PREFIX_ROOT}etc/default"
BIN_DIR="${PREFIX_ROOT}usr/bin"

REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse qjackctl"

echo "Installing jack-bridge contrib files (non-destructive)..."

# Non-interactive package installation for Debian-like systems (will prompt for sudo password)
if command -v apt >/dev/null 2>&1; then
    echo "Detected apt. Installing required packages: $REQUIRED_PACKAGES"
    if ! sudo apt update; then
        echo "Warning: apt update failed; continuing to install may still work."
    fi
    if ! sudo apt install -y $REQUIRED_PACKAGES; then
        echo "Package installation failed or was interrupted. Please install required packages manually:"
        echo "  sudo apt install -y $REQUIRED_PACKAGES"
        echo "Continuing installation of config files (admin must resolve missing packages before use)."
    fi
else
    echo "apt not found. Please ensure these packages are installed: $REQUIRED_PACKAGES"
fi

# Install /etc/asound.conf template (won't overwrite unless --force)
ASOUND_DST="${ETC_DIR}/asound.conf"
if [ -e "$ASOUND_DST" ]; then
    echo "Note: $ASOUND_DST already exists; skipping (preserve existing). If you want to replace, move it and rerun with --force."
else
    mkdir -p "$(dirname "$ASOUND_DST")"
    cp -p contrib/etc/asound.conf "$ASOUND_DST"
    echo "Installed $ASOUND_DST"
fi

# Install init script
mkdir -p "$INIT_DIR"
cp -p contrib/init.d/jackd-rt "${INIT_DIR}/jackd-rt"
chmod 755 "${INIT_DIR}/jackd-rt"
echo "Installed init script to ${INIT_DIR}/jackd-rt"

# Install defaults file
mkdir -p "$DEFAULTS_DIR"
cp -p contrib/default/jackd-rt "${DEFAULTS_DIR}/jackd-rt"
chmod 644 "${DEFAULTS_DIR}/jackd-rt"
echo "Installed defaults to ${DEFAULTS_DIR}/jackd-rt"

# Ensure JACK_NO_AUDIO_RESERVATION is set in /etc/default/jackd-rt to allow service startup
# without a session D-Bus (useful for headless/service start). If the variable already
# exists in the file we leave it unchanged; otherwise append a non-destructive setting.
DEFAULT_FILE="${DEFAULTS_DIR}/jackd-rt"
if [ -f "$DEFAULT_FILE" ]; then
    if grep -q '^JACK_NO_AUDIO_RESERVATION=' "$DEFAULT_FILE"; then
        echo "JACK_NO_AUDIO_RESERVATION already configured in $DEFAULT_FILE; leaving as-is."
    else
        printf "\n# Allow jackd to skip session bus device reservation when running as a system service\nJACK_NO_AUDIO_RESERVATION=1\n" >> "$DEFAULT_FILE"
        echo "Appended JACK_NO_AUDIO_RESERVATION=1 to $DEFAULT_FILE (non-destructive)."
    fi
fi

# Install helper scripts
mkdir -p "$USR_LIB_DIR"
cp -p contrib/usr/lib/jack-bridge/detect-alsa-device.sh "${USR_LIB_DIR}/detect-alsa-device.sh"
chmod 755 "${USR_LIB_DIR}/detect-alsa-device.sh"
echo "Installed helper detect script to ${USR_LIB_DIR}/detect-alsa-device.sh"

# Install autoconnect and watchdog helpers (from contrib; idempotent)
if [ -f "contrib/usr/lib/jack-bridge/jack-autoconnect" ]; then
    cp -p contrib/usr/lib/jack-bridge/jack-autoconnect "${USR_LIB_DIR}/jack-autoconnect"
    chmod 755 "${USR_LIB_DIR}/jack-autoconnect"
    echo "Installed jack-autoconnect to ${USR_LIB_DIR}/jack-autoconnect"
fi

if [ -f "contrib/usr/lib/jack-bridge/jack-watchdog" ]; then
    cp -p contrib/usr/lib/jack-bridge/jack-watchdog "${USR_LIB_DIR}/jack-watchdog"
    chmod 755 "${USR_LIB_DIR}/jack-watchdog"
    echo "Installed jack-watchdog to ${USR_LIB_DIR}/jack-watchdog"
fi

# Install simple apulse wrappers
mkdir -p "$BIN_DIR"
# apulse wrapper for firefox
cat > "${BIN_DIR}/apulse-firefox" <<'EOF'
#!/bin/sh
# wrapper to run firefox under apulse so PA apps use ALSA -> JACK
exec apulse firefox "$@"
EOF
chmod 755 "${BIN_DIR}/apulse-firefox"
echo "Installed apulse-firefox to ${BIN_DIR}/apulse-firefox"

# apulse wrapper for chromium
cat > "${BIN_DIR}/apulse-chromium" <<'EOF'
#!/bin/sh
# wrapper to run chromium under apulse so PA apps use ALSA -> JACK
exec apulse chromium "$@"
EOF
chmod 755 "${BIN_DIR}/apulse-chromium"
echo "Installed apulse-chromium to ${BIN_DIR}/apulse-chromium"

# Create .desktop launcher overrides so desktop environments launch apulse-wrapped browsers
DESKTOP_DIR="/usr/share/applications"
mkdir -p "$DESKTOP_DIR"

# Firefox desktop wrapper (non-destructive: only write if not present or if --force later)
FIREFOX_DESKTOP="${DESKTOP_DIR}/apulse-firefox.desktop"
cat > "$FIREFOX_DESKTOP" <<'EOF'
[Desktop Entry]
Name=Firefox (apulse)
Comment=Run Firefox under apulse so web audio routes to ALSA->JACK
Exec=/usr/bin/apulse-firefox %u
Terminal=false
Type=Application
Categories=Network;WebBrowser;
MimeType=text/html;text/xml;application/xhtml+xml;application/xml;x-scheme-handler/http;x-scheme-handler/https;
EOF
chmod 644 "$FIREFOX_DESKTOP"
echo "Installed desktop launcher $FIREFOX_DESKTOP"

# Chromium desktop wrapper
CHROMIUM_DESKTOP="${DESKTOP_DIR}/apulse-chromium.desktop"
cat > "$CHROMIUM_DESKTOP" <<'EOF'
[Desktop Entry]
Name=Chromium (apulse)
Comment=Run Chromium under apulse so web audio routes to ALSA->JACK
Exec=/usr/bin/apulse-chromium %U
Terminal=false
Type=Application
Categories=Network;WebBrowser;
MimeType=text/html;text/xml;application/xhtml+xml;application/xml;x-scheme-handler/http;x-scheme-handler/https;
EOF
chmod 644 "$CHROMIUM_DESKTOP"
echo "Installed desktop launcher $CHROMIUM_DESKTOP"

# Note: We do not override system 'firefox' or 'chromium' executables. Instead we provide
# wrapped desktop entries named "Firefox (apulse)" and "Chromium (apulse)" so users
# can launch browsers without extra steps. Administrators who want full replacement
# can create symlinks or update system desktop files.

# Install realtime limits template (do not overwrite existing file)
LIMITS_DST="${ETC_DIR}/security/limits.d/audio.conf"
if [ -e "$LIMITS_DST" ]; then
    echo "Note: $LIMITS_DST already exists; skipping (preserve existing). If you want to replace, move it and rerun with --force."
else
    mkdir -p "$(dirname "$LIMITS_DST")"
    cp -p contrib/etc/security/limits.d/audio.conf "$LIMITS_DST"
    echo "Installed realtime limits template to $LIMITS_DST"
fi

# Register init script with update-rc.d if available
if command -v update-rc.d >/dev/null 2>&1; then
    echo "Registering jackd-rt init script with update-rc.d (defaults)..."
    if ! sudo update-rc.d jackd-rt defaults; then
        echo "Warning: update-rc.d returned a non-zero status. You may need to register the init script manually."
    fi
else
    echo "update-rc.d not available; please register ${INIT_DIR}/jackd-rt in your init system manually if desired."
fi

 # Disable PulseAudio autospawn system-wide (create /etc/pulse/client.conf.d/01-no-autospawn.conf)
# Non-destructive: creates conf to prevent user-level pulseaudio autospawn which can block ALSA devices.
mkdir -p /etc/pulse/client.conf.d
cat > /etc/pulse/client.conf.d/01-no-autospawn.conf <<'PAEOF'
# Created by jack-bridge contrib installer
autospawn = no
daemon-binary = /bin/true
PAEOF
chmod 644 /etc/pulse/client.conf.d/01-no-autospawn.conf || true
echo "Created /etc/pulse/client.conf.d/01-no-autospawn.conf to disable PulseAudio autospawn"

# Guidance for PipeWire (no changes performed automatically)
echo "Note: If PipeWire is installed, you may disable its PulseAudio compatibility without purging by disabling user/session autostarts."
echo "      On systemd-based user sessions: systemctl --user mask --now pipewire-pulse.service pipewire.service pipewire.socket"
echo "      On non-systemd sessions, disable any XDG autostart entries for pipewire/pipewire-pulse."

# Install qjackctl autostart entry (GUI convenience; server already runs at boot)
XDG_AUTOSTART_DIR="/etc/xdg/autostart"
mkdir -p "$XDG_AUTOSTART_DIR"
cat > "${XDG_AUTOSTART_DIR}/jack-bridge-qjackctl.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=QjackCtl (Auto)
Comment=Launch QjackCtl minimized; JACK is already started by system service
TryExec=/usr/bin/qjackctl
Exec=/usr/bin/qjackctl --start-minimized
X-GNOME-Autostart-enabled=true
NoDisplay=false
OnlyShowIn=XFCE;LXDE;LXQt;MATE;GNOME;KDE;
EOF
chmod 644 "${XDG_AUTOSTART_DIR}/jack-bridge-qjackctl.desktop" || true
echo "Installed ${XDG_AUTOSTART_DIR}/jack-bridge-qjackctl.desktop"

# Add desktop users (UID>=1000) to 'audio' group automatically so JACK can run without manual user steps
# Non-destructive: users already in the group are left as-is; failures are reported but do not abort install.
echo "Adding desktop users (UID>=1000) to the 'audio' group (automatic)..."
for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    if id -nG "$u" 2>/dev/null | grep -qw audio; then
        echo "User '$u' already in audio group."
    else
        if usermod -aG audio "$u" 2>/dev/null; then
            echo "Added '$u' to audio group."
        else
            echo "Warning: failed to add '$u' to audio group. You may need to run: sudo usermod -aG audio $u"
        fi
    fi
done
echo "Installation complete. Please reboot the system to start JACK and the jackd-rt service at boot."

exit 0
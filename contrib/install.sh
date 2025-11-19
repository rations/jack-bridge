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

REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse qjackctl libasound2-plugin-equal swh-plugins libgtk-3-0 bluez bluez-tools dbus policykit-1"

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

# Install /etc/asound.conf template (force-install; back up existing file)
ASOUND_DST="${ETC_DIR}/asound.conf"
mkdir -p "$(dirname "$ASOUND_DST")"
if [ -e "$ASOUND_DST" ]; then
    # Back up existing file with timestamp before replacing
    BACKUP="${ASOUND_DST}.$(date +%Y%m%d%H%M%S).bak"
    cp -p "$ASOUND_DST" "$BACKUP" || true
    echo "Backed up existing $ASOUND_DST to $BACKUP"
fi
# Force copy the updated template (overwrite)
cp -pf contrib/etc/asound.conf "$ASOUND_DST"
echo "Installed (or replaced) $ASOUND_DST"

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

# Install bundled AlsaTune GUI from repo contrib/ paths
# The mxeq binary and desktop file are expected to be committed into the repo at:
#   contrib/bin/mxeq
#   contrib/mxeq.desktop
# This makes the installer self-contained for end users.
if [ -f "contrib/bin/mxeq" ]; then
    echo "Installing bundled AlsaTune GUI to /usr/local/bin and desktop entries..."
    mkdir -p /usr/local/bin
    install -m 0755 contrib/bin/mxeq /usr/local/bin/mxeq || true
    if [ -f "contrib/mxeq.desktop" ]; then
        mkdir -p /usr/share/applications
        install -m 0644 contrib/mxeq.desktop /usr/share/applications/mxeq.desktop || true
    fi
    echo "Installed AlsaTune (mxeq) launcher."
else
    echo "No bundled AlsaTune found in contrib/; skipping GUI installation."
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

# Install realtime limits template (force-install; back up existing file)
LIMITS_DST="${ETC_DIR}/security/limits.d/audio.conf"
mkdir -p "$(dirname "$LIMITS_DST")"
if [ -e "$LIMITS_DST" ]; then
    BACKUP="${LIMITS_DST}.$(date +%Y%m%d%H%M%S).bak"
    cp -p "$LIMITS_DST" "$BACKUP" || true
    echo "Backed up existing $LIMITS_DST to $BACKUP"
fi
cp -pf contrib/etc/security/limits.d/audio.conf "$LIMITS_DST"
echo "Installed (or replaced) realtime limits template to $LIMITS_DST"

# Register init script with update-rc.d if available (explicit priorities for ordering)
# Desired order: dbus -> bluetoothd -> bluealsad -> jackd-rt -> jack-bluealsa-autobridge
if command -v update-rc.d >/dev/null 2>&1; then
    echo "Registering jackd-rt init script with explicit priorities..."
    # jackd-rt after bluealsad (bluetoothd -> bluealsad -> jackd-rt)
    # start 22 at runlevels 2 3 4 5; stop 78 at 0 1 6
    sudo update-rc.d -f jackd-rt remove >/dev/null 2>&1 || true
    if ! sudo update-rc.d jackd-rt start 22 2 3 4 5 . stop 78 0 1 6 .; then
        echo "Warning: update-rc.d jackd-rt explicit registration failed; falling back to defaults"
        sudo update-rc.d jackd-rt defaults || true
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

# Also add desktop users to 'bluetooth' group when present (required for some BlueZ setups)
if getent group bluetooth >/dev/null; then
    echo "Adding desktop users (UID>=1000) to the 'bluetooth' group (if present)..."
    for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
        if id -nG "$u" 2>/dev/null | grep -qw bluetooth; then
            echo "User '$u' already in bluetooth group."
        else
            if usermod -aG bluetooth "$u" 2>/dev/null; then
                echo "Added '$u' to bluetooth group."
            else
                echo "Warning: failed to add '$u' to bluetooth group. You may need to run: sudo usermod -aG bluetooth $u"
            fi
        fi
    done
else
    echo "Group 'bluetooth' not present; skipping bluetooth group additions."
fi

echo "Ensuring BlueALSA runtime and autobridge integration (non-destructive)..."

# Create dedicated bluealsa system user (nologin) if it does not exist
if ! id -u bluealsa >/dev/null 2>&1; then
    echo "Creating system user 'bluealsa' (nologin) for bluealsad runtime..."
    if command -v adduser >/dev/null 2>&1; then
        sudo adduser --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    else
        sudo useradd --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    fi
else
    echo "User 'bluealsa' already exists."
fi

# Create persistent state directory for bluealsa with strict perms
if [ ! -d /var/lib/bluealsa ]; then
    echo "Creating /var/lib/bluealsa owned by bluealsa (0700)..."
    sudo mkdir -p /var/lib/bluealsa
    sudo chown bluealsa:bluealsa /var/lib/bluealsa || true
    sudo chmod 0700 /var/lib/bluealsa || true
else
    echo "/var/lib/bluealsa already exists; ensuring ownership/perms..."
    sudo chown bluealsa:bluealsa /var/lib/bluealsa 2>/dev/null || true
    sudo chmod 0700 /var/lib/bluealsa 2>/dev/null || true
fi

# Install BlueALSA prebuilt binaries if provided in repo contrib/bin (prebuilt artifacts)
if [ -f "contrib/bin/bluealsad" ]; then
    echo "Installing prebuilt bluealsad to /usr/local/bin..."
    sudo install -m 0755 contrib/bin/bluealsad /usr/local/bin/bluealsad || true
fi
if [ -f "contrib/bin/bluealsactl" ]; then
    echo "Installing prebuilt bluealsactl to /usr/local/bin..."
    sudo install -m 0755 contrib/bin/bluealsactl /usr/local/bin/bluealsactl || true
fi
if [ -f "contrib/bin/bluealsa-aplay" ]; then
    echo "Installing prebuilt bluealsa-aplay to /usr/local/bin..."
    sudo install -m 0755 contrib/bin/bluealsa-aplay /usr/local/bin/bluealsa-aplay || true
fi
if [ -f "contrib/bin/bluealsa-rfcomm" ]; then
    echo "Installing prebuilt bluealsa-rfcomm to /usr/local/bin..."
    sudo install -m 0755 contrib/bin/bluealsa-rfcomm /usr/local/bin/bluealsa-rfcomm || true
fi


# Install jack-bluealsa-autobridge binary if provided in repo contrib/bin (prebuilt artifact)
if [ -f "contrib/bin/jack-bluealsa-autobridge" ]; then
    echo "Installing jack-bluealsa-autobridge to /usr/local/bin..."
    sudo install -m 0755 contrib/bin/jack-bluealsa-autobridge /usr/local/bin/jack-bluealsa-autobridge || true
else
    echo "No jack-bluealsa-autobridge binary found in contrib/bin/. Skip binary install."
fi

# Autobridge init script will be installed after bluealsad registration (see below)

# Install SysV init script for bluetoothd if present
if [ -f "contrib/init.d/bluetoothd" ]; then
    echo "Installing SysV init script for bluetoothd..."
    sudo cp -p contrib/init.d/bluetoothd "${INIT_DIR}/bluetoothd"
    sudo chmod 755 "${INIT_DIR}/bluetoothd"
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering bluetoothd init script with explicit priorities..."
        # bluetoothd before bluealsad and jackd-rt
        sudo update-rc.d -f bluetoothd remove >/dev/null 2>&1 || true
        sudo update-rc.d bluetoothd start 20 2 3 4 5 . stop 80 0 1 6 . || true
    else
        echo "update-rc.d not available; please register ${INIT_DIR}/bluetoothd manually if desired."
    fi
fi

# Optionally install bluealsad init script and defaults if provided in contrib
if [ -f "contrib/init.d/bluealsad" ]; then
    echo "Installing contrib init script for bluealsad (optional)..."
    sudo cp -p contrib/init.d/bluealsad "${INIT_DIR}/bluealsad"
    sudo chmod 755 "${INIT_DIR}/bluealsad"
    # Install defaults file if provided
    if [ -f "contrib/default/bluealsad" ]; then
        sudo install -m 0644 contrib/default/bluealsad "${DEFAULTS_DIR}/bluealsad" || true
        echo "Installed defaults to ${DEFAULTS_DIR}/bluealsad"
    fi
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering bluealsad init script with explicit priorities..."
        # bluealsad after bluetoothd, before jackd-rt
        sudo update-rc.d -f bluealsad remove >/dev/null 2>&1 || true
        sudo update-rc.d bluealsad start 21 2 3 4 5 . stop 79 0 1 6 . || true
    fi
fi

# Now install and register autobridge after bluealsad is enabled/available
if [ -f "contrib/init.d/jack-bluealsa-autobridge" ]; then
    echo "Installing SysV init script for jack-bluealsa-autobridge..."
    sudo cp -p contrib/init.d/jack-bluealsa-autobridge "${INIT_DIR}/jack-bluealsa-autobridge"
    sudo chmod 755 "${INIT_DIR}/jack-bluealsa-autobridge"
    if command -v bluealsad >/dev/null 2>&1; then
        if command -v update-rc.d >/dev/null 2>&1; then
            echo "Registering jack-bluealsa-autobridge init script with explicit priorities..."
            # autobridge after jackd-rt
            sudo update-rc.d -f jack-bluealsa-autobridge remove >/dev/null 2>&1 || true
            sudo update-rc.d jack-bluealsa-autobridge start 23 2 3 4 5 . stop 77 0 1 6 . || true
        else
            echo "update-rc.d not available; please register ${INIT_DIR}/jack-bluealsa-autobridge manually if desired."
        fi
    else
        echo "bluealsad binary not found; skipping autobridge registration to avoid insserv dependency errors. Install BlueALSA first."
    fi
else
    echo "No contrib/init.d/jack-bluealsa-autobridge found; skipping init script install."
fi


# Install BlueALSA D-Bus policy (canonical)
DBUS_POLICY_SRC="usr/share/dbus-1/system.d/org.bluealsa.conf"
DBUS_POLICY_DST="/usr/share/dbus-1/system.d/org.bluealsa.conf"
if [ -f "$DBUS_POLICY_SRC" ]; then
    echo "Installing org.bluealsa D-Bus policy to $DBUS_POLICY_DST (from $DBUS_POLICY_SRC)"
    sudo mkdir -p "$(dirname "$DBUS_POLICY_DST")"
    sudo install -m 0644 "$DBUS_POLICY_SRC" "$DBUS_POLICY_DST" || true
    # best-effort reload of D-Bus to pick up policy changes
    if command -v service >/dev/null 2>&1; then
        sudo service dbus reload >/dev/null 2>&1 || true
    fi
else
    echo "No bundled canonical D-Bus policy found at $DBUS_POLICY_SRC; assuming distro provides one."
fi

# Install polkit rule to authorize BlueZ Adapter/Device operations for users in 'audio' or 'bluetooth'
POLKIT_RULE_SRC="contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
POLKIT_RULE_DST="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
if [ -f "$POLKIT_RULE_SRC" ]; then
    echo "Installing polkit rule to $POLKIT_RULE_DST"
    sudo mkdir -p "$(dirname "$POLKIT_RULE_DST")"
    sudo install -m 0644 "$POLKIT_RULE_SRC" "$POLKIT_RULE_DST" || true
    # best-effort reload of polkit (no systemd dependency)
    if pidof polkitd >/dev/null 2>&1; then
        sudo kill -HUP "$(pidof polkitd | awk '{print $1}')" 2>/dev/null || true
    fi
    if command -v service >/dev/null 2>&1; then
        sudo service polkit restart >/dev/null 2>&1 || true
    fi
else
    echo "Polkit rule not found at $POLKIT_RULE_SRC; skipping (Pair/Connect may prompt/deny without it)."
fi

# Install jack-bridge Bluetooth config (non-destructive)
CONF_SRC="contrib/etc/jack-bridge/bluetooth.conf"
CONF_DST="/etc/jack-bridge/bluetooth.conf"
if [ -f "$CONF_SRC" ]; then
    mkdir -p /etc/jack-bridge
    if [ -e "$CONF_DST" ]; then
        echo "Existing $CONF_DST detected; installing as $CONF_DST.new (review and merge manually if desired)"
        install -m 0644 "$CONF_SRC" "$CONF_DST.new" || true
    else
        install -m 0644 "$CONF_SRC" "$CONF_DST" || true
        echo "Installed $CONF_DST"
    fi
fi

echo "Installation complete. Please reboot the system to start JACK, bluetoothd (distro), bluealsad (if installed), and the jack-bluealsa-autobridge service at boot."

exit 0
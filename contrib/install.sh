#!/bin/sh
# contrib/install.sh
# Installer for jack-bridge contrib package on Debian-like systems (SysV-style).
# Installs configs into /etc, helper scripts into /usr/local/lib/jack-bridge, init script into /etc/init.d,
# and simple apulse wrappers into /usr/bin. Does not hardcode usernames or devices.
#
# Usage: sudo sh contrib/install.sh
set -e

# Require root (system-wide installer)
if [ "$(id -u)" -ne 0 ]; then
    echo "This installer must be run as root. Try: sudo ./contrib/install.sh"
    exit 1
fi

PREFIX_ROOT="/"
ETC_DIR="${PREFIX_ROOT}etc"
USR_LIB_DIR="${PREFIX_ROOT}usr/local/lib/jack-bridge"
INIT_DIR="${PREFIX_ROOT}etc/init.d"
DEFAULTS_DIR="${PREFIX_ROOT}etc/default"
BIN_DIR="${PREFIX_ROOT}usr/bin"

# Note: We do NOT install bluez-alsa-utils because we use our prebuilt BlueALSA daemon in contrib/bin/
# We only need libasound2-plugin-bluez for the ALSA plugin that alsa_out uses
REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse qjackctl libasound2-plugin-equal swh-plugins libgtk-3-0 bluez bluez-tools dbus policykit-1 imagemagick libasound2-plugin-bluez"

echo "Installing jack-bridge contrib files"

# Non-interactive package installation for Debian-like systems (will prompt for sudo password)
if command -v apt >/dev/null 2>&1; then
    echo "Detected apt. Installing required packages: $REQUIRED_PACKAGES"
    if ! apt update; then
        echo "Warning: apt update failed; continuing to install may still work."
    fi
    if ! apt install -y $REQUIRED_PACKAGES; then
        echo "Package installation failed or was interrupted. Required packages must be installed for jack-bridge to function."
        echo "Retry: sudo apt install -y $REQUIRED_PACKAGES"
        exit 1
    fi
else
    echo "apt not found. Please ensure these packages are installed: $REQUIRED_PACKAGES"
fi

# Cleanup obsolete artifacts from previous versions (authoritative removal)
for f in /etc/init.d/jack-bluealsa-autobridge /usr/local/bin/jack-bluealsa-autobridge /etc/jack-bridge/bluetooth.conf; do
    if [ -e "$f" ]; then
        rm -f "$f"
        echo "Removed obsolete $f"
    fi
done

# Remove old BlueALSA config if it was installed by jack-bridge (not by package manager)
for D in /usr/share/alsa/alsa.conf.d /etc/alsa/conf.d; do
    OLD_CONF="$D/20-bluealsa.conf"
    if [ -f "$OLD_CONF" ]; then
        # Check if it's ours (contains "jack-bridge" or "Installed by jack-bridge")
        if grep -q "jack-bridge" "$OLD_CONF" 2>/dev/null; then
            rm -f "$OLD_CONF"
            echo "Removed old jack-bridge BlueALSA config: $OLD_CONF"
        fi
    fi
done

# Remove old contrib/etc/20-bluealsa.conf from repo if present (obsolete)
if [ -f "contrib/etc/20-bluealsa.conf" ]; then
    echo "Note: contrib/etc/20-bluealsa.conf is obsolete (replaced by 20-jack-bridge-bluealsa.conf)"
fi

# Install /etc/asound.conf template
ASOUND_DST="${ETC_DIR}/asound.conf"
mkdir -p "$(dirname "$ASOUND_DST")"
# Force install the updated template (overwrite)
install -m 0644 contrib/etc/asound.conf "$ASOUND_DST"
echo "Installed (replaced) $ASOUND_DST"

# Install init script
mkdir -p "$INIT_DIR"
install -m 0755 contrib/init.d/jackd-rt "${INIT_DIR}/jackd-rt"
echo "Installed init script to ${INIT_DIR}/jackd-rt"

# Install defaults file
mkdir -p "$DEFAULTS_DIR"
install -m 0644 contrib/default/jackd-rt "${DEFAULTS_DIR}/jackd-rt"
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

# Install helper scripts (force overwrite)
mkdir -p "$USR_LIB_DIR"
install -m 0755 contrib/usr/lib/jack-bridge/detect-alsa-device.sh "${USR_LIB_DIR}/detect-alsa-device.sh"
echo "Installed helper detect script to ${USR_LIB_DIR}/detect-alsa-device.sh"

# Install routing helper used by the GUI Devices panel (runtime JACK routing)
if [ -f "contrib/usr/local/lib/jack-bridge/jack-route-select" ]; then
    install -m 0755 contrib/usr/local/lib/jack-bridge/jack-route-select "${USR_LIB_DIR}/jack-route-select"
    echo "Installed routing helper to ${USR_LIB_DIR}/jack-route-select"
else
    echo "Warning: routing helper not found at contrib/usr/local/lib/jack-bridge/jack-route-select; Devices panel may not route."
fi

# Install autoconnect and watchdog helpers (from contrib; force overwrite)
if [ -f "contrib/usr/lib/jack-bridge/jack-autoconnect" ]; then
    install -m 0755 contrib/usr/lib/jack-bridge/jack-autoconnect "${USR_LIB_DIR}/jack-autoconnect"
    echo "Installed jack-autoconnect to ${USR_LIB_DIR}/jack-autoconnect"
fi

if [ -f "contrib/usr/lib/jack-bridge/jack-watchdog" ]; then
    install -m 0755 contrib/usr/lib/jack-bridge/jack-watchdog "${USR_LIB_DIR}/jack-watchdog"
    echo "Installed jack-watchdog to ${USR_LIB_DIR}/jack-watchdog"
fi

# Ensure ALSA override directory exists and install default current_input.conf -> input_card0
ASOUND_D_DIR="${ETC_DIR}/asound.conf.d"
mkdir -p "$ASOUND_D_DIR"
cat > "${ASOUND_D_DIR}/current_input.conf" <<'EOF'
pcm.current_input {
    type plug
    slave.pcm "input_card0"
}
EOF
chmod 644 "${ASOUND_D_DIR}/current_input.conf"
echo "Installed default ${ASOUND_D_DIR}/current_input.conf (pcm.current_input -> input_card0)"

# Install bundled Alsa Sound Connect GUI from repo contrib/ paths
# The mxeq binary and desktop file are expected to be committed into the repo at:
#   contrib/bin/mxeq
#   contrib/mxeq.desktop
# This makes the installer self-contained for end users.
if [ -f "contrib/bin/mxeq" ]; then
    echo "Installing bundled Alsa Sound Connect GUI to /usr/local/bin and desktop entries..."
    mkdir -p /usr/local/bin
    install -m 0755 contrib/bin/mxeq /usr/local/bin/mxeq || true

    # Install desktop entry, but ensure Icon is set to alsa-sound-connect so it matches the installed icon.
    if [ -f "contrib/mxeq.desktop" ]; then
        mkdir -p /usr/share/applications
        TMPDESK="$(mktemp /tmp/mxeq.desktop.XXXXXX)"
        sed 's/^Icon=.*$/Icon=alsa-sound-connect/' contrib/mxeq.desktop > "$TMPDESK" || cp -f contrib/mxeq.desktop "$TMPDESK"
        install -m 0644 "$TMPDESK" /usr/share/applications/mxeq.desktop || true
        rm -f "$TMPDESK" || true
    fi

    # Install PNG icon to hicolor scalable apps and update icon cache
    ICON_SRC_PNG="contrib/usr/share/icons/hicolor/scalable/apps/alsasoundconnectlogo.png"
    ICON_DST_DIR="/usr/share/icons/hicolor/scalable/apps"
    mkdir -p "$ICON_DST_DIR"
    if [ -f "$ICON_SRC_PNG" ]; then
        # Install as alsa-sound-connect.png so the desktop entry Icon=alsa-sound-connect resolves
        install -m 0644 "$ICON_SRC_PNG" "${ICON_DST_DIR}/alsa-sound-connect.png" || true
        echo "Installed icon: ${ICON_DST_DIR}/alsa-sound-connect.png"

        # Generate PNG fallbacks at common sizes for DE compatibility
        for SZ in 16 32 48 128; do
            DST_DIR="/usr/share/icons/hicolor/${SZ}x${SZ}/apps"
            mkdir -p "$DST_DIR"
            if command -v convert >/dev/null 2>&1; then
                # Best-effort resize; if convert fails, copy original
                convert "$ICON_SRC_PNG" -resize "${SZ}x${SZ}" "${DST_DIR}/alsa-sound-connect.png" >/dev/null 2>&1 \
                  || cp -f "$ICON_SRC_PNG" "${DST_DIR}/alsa-sound-connect.png"
            else
                # Fallback: copy original PNG (may be larger than target size)
                cp -f "$ICON_SRC_PNG" "${DST_DIR}/alsa-sound-connect.png" || true
            fi
            echo "Installed icon fallback: ${DST_DIR}/alsa-sound-connect.png"
        done
    else
        echo "Warning: No bundled PNG icon found at $ICON_SRC_PNG; skipping icon install."
    fi

    # Refresh icon cache (best-effort)
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -f /usr/share/icons/hicolor >/dev/null 2>&1 || true
    fi
    # Refresh desktop database so menus pick up new .desktop files (best-effort)
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    fi

    echo "Installed Alsa Sound Connect (mxeq) launcher and icons."

else
    echo "No bundled Alsa Sound Connect found in contrib/; skipping GUI installation."
fi

# Install apulse wrappers (always; do not remove)
mkdir -p "$BIN_DIR"
# /usr/bin/apulse-firefox
cat > "${BIN_DIR}/apulse-firefox" <<'EOF'
#!/bin/sh
# wrapper to run firefox under apulse so PA apps use ALSA -> JACK
exec apulse firefox "$@"
EOF
chmod 755 "${BIN_DIR}/apulse-firefox"
echo "Installed apulse-firefox to ${BIN_DIR}/apulse-firefox"

# /usr/bin/apulse-chromium
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

# Install realtime limits template (force-install; overwrite without backup)
LIMITS_DST="${ETC_DIR}/security/limits.d/audio.conf"
mkdir -p "$(dirname "$LIMITS_DST")"
install -m 0644 contrib/etc/security/limits.d/audio.conf "$LIMITS_DST" || true
echo "Installed (replaced) realtime limits template to $LIMITS_DST"

# Register init script with update-rc.d if available (defaults)
if command -v update-rc.d >/dev/null 2>&1; then
    echo "Registering jackd-rt init script with update-rc.d (defaults)..."
    update-rc.d jackd-rt defaults || true
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

# Explicitly add the installer invoker (SUDO_USER) when present for convenience
if [ -n "$SUDO_USER" ]; then
    if id -nG "$SUDO_USER" 2>/dev/null | grep -qw audio; then
        echo "SUDO_USER '$SUDO_USER' already in audio group."
    else
        if usermod -aG audio "$SUDO_USER" 2>/dev/null; then
            echo "Added SUDO_USER '$SUDO_USER' to audio group."
        else
            echo "Warning: failed to add SUDO_USER '$SUDO_USER' to audio group."
        fi
    fi
fi

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

    # Also add the installer invoker (SUDO_USER) when present
    if [ -n "$SUDO_USER" ]; then
        if id -nG "$SUDO_USER" 2>/dev/null | grep -qw bluetooth; then
            echo "SUDO_USER '$SUDO_USER' already in bluetooth group."
        else
            if usermod -aG bluetooth "$SUDO_USER" 2>/dev/null; then
                echo "Added SUDO_USER '$SUDO_USER' to bluetooth group."
            else
                echo "Warning: failed to add SUDO_USER '$SUDO_USER' to bluetooth group."
            fi
        fi
    fi
else
    echo "Group 'bluetooth' not present; skipping bluetooth group additions."
fi

# Inform admin/user that group changes require re-login
echo "Note: Users added to groups must log out and log back in (or reboot) for group membership to take effect."

echo "Ensuring BlueALSA runtime (non-destructive)..."

# Create dedicated bluealsa system user (nologin) if it does not exist
if ! id -u bluealsa >/dev/null 2>&1; then
    echo "Creating system user 'bluealsa' (nologin) for bluealsad runtime..."
    if command -v adduser >/dev/null 2>&1; then
        adduser --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    else
        useradd --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    fi
else
    echo "User 'bluealsa' already exists."
fi

# Create persistent state directory for bluealsa with strict perms
if [ ! -d /var/lib/bluealsa ]; then
    echo "Creating /var/lib/bluealsa owned by bluealsa (0700)..."
    mkdir -p /var/lib/bluealsa
    chown bluealsa:bluealsa /var/lib/bluealsa || true
    chmod 0700 /var/lib/bluealsa || true
else
    echo "/var/lib/bluealsa already exists; ensuring ownership/perms..."
    chown bluealsa:bluealsa /var/lib/bluealsa 2>/dev/null || true
    chmod 0700 /var/lib/bluealsa 2>/dev/null || true
fi

# Install BlueALSA prebuilt binaries (required - we do not use distro bluez-alsa-utils)
echo "Installing jack-bridge prebuilt BlueALSA binaries to /usr/local/bin..."
if [ -f "contrib/bin/bluealsad" ]; then
    install -m 0755 contrib/bin/bluealsad /usr/local/bin/bluealsad || true
    echo "  ✓ Installed bluealsad"
else
    echo "ERROR: contrib/bin/bluealsad not found! BlueALSA daemon required for Bluetooth audio."
    exit 1
fi
if [ -f "contrib/bin/bluealsactl" ]; then
    install -m 0755 contrib/bin/bluealsactl /usr/local/bin/bluealsactl || true
    echo "  ✓ Installed bluealsactl"
fi
if [ -f "contrib/bin/bluealsa-aplay" ]; then
    install -m 0755 contrib/bin/bluealsa-aplay /usr/local/bin/bluealsa-aplay || true
    echo "  ✓ Installed bluealsa-aplay"
fi
if [ -f "contrib/bin/bluealsa-rfcomm" ]; then
    install -m 0755 contrib/bin/bluealsa-rfcomm /usr/local/bin/bluealsa-rfcomm || true
    echo "  ✓ Installed bluealsa-rfcomm"
fi

# Install matching ALSA plugins (Phase 2: Plugin Compatibility)
# These must match the daemon version to ensure proper D-Bus communication
echo "Installing matching BlueALSA ALSA plugins..."
ALSA_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/alsa-lib"
# Detect architecture for non-x86_64 systems
if [ "$(uname -m)" != "x86_64" ]; then
    # Try to detect correct architecture directory
    for arch_dir in /usr/lib/*/alsa-lib; do
        if [ -d "$arch_dir" ]; then
            ALSA_PLUGIN_DIR="$arch_dir"
            break
        fi
    done
fi

mkdir -p "$ALSA_PLUGIN_DIR"

# Install PCM plugin (required for playback)
if [ -f "contrib/bin/libasound_module_pcm_bluealsa.so" ]; then
    # Backup distro version if it exists
    if [ -f "$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so" ]; then
        mv "$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so" \
           "$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so.distro-backup" 2>/dev/null || true
        echo "  Backed up distro plugin to libasound_module_pcm_bluealsa.so.distro-backup"
    fi
    install -m 0644 contrib/bin/libasound_module_pcm_bluealsa.so "$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
    echo "  ✓ Installed libasound_module_pcm_bluealsa.so (PCM plugin)"
else
    echo "  ! libasound_module_pcm_bluealsa.so not found in contrib/bin/"
    echo "    Using distro plugin (may cause version mismatch issues)"
fi

# Install CTL plugin (optional - provides mixer controls)
if [ -f "contrib/bin/libasound_module_ctl_bluealsa.so" ]; then
    # Backup distro version if it exists
    if [ -f "$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so" ]; then
        mv "$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so" \
           "$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so.distro-backup" 2>/dev/null || true
    fi
    install -m 0644 contrib/bin/libasound_module_ctl_bluealsa.so "$ALSA_PLUGIN_DIR/libasound_module_ctl_bluealsa.so"
    echo "  ✓ Installed libasound_module_ctl_bluealsa.so (CTL plugin)"
else
    echo "  ! libasound_module_ctl_bluealsa.so not found (optional - mixer controls)"
fi

# Note: jack-bluealsa-autobridge has been removed from this project; no autobridge binary is installed.

# Install bluetoothd init script if provided (jack-bridge manages bluetoothd for SysVinit systems)
if [ -f "contrib/init.d/bluetoothd" ]; then
    echo "Installing jack-bridge bluetoothd init script..."
    install -m 0755 contrib/init.d/bluetoothd "${INIT_DIR}/bluetoothd"
    
    # Register with update-rc.d (starts before bluealsad)
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering bluetoothd init script..."
        update-rc.d -f bluetoothd remove >/dev/null 2>&1 || true
        update-rc.d bluetoothd start 20 2 3 4 5 . stop 80 0 1 6 . || true
    fi
else
    echo "No jack-bridge bluetoothd init script found; using distro bluetoothd"
fi

# Install bluealsad init script and defaults if provided in contrib (force overwrite)
if [ -f "contrib/init.d/bluealsad" ]; then
    echo "Installing contrib init script for bluealsad..."
    install -m 0755 contrib/init.d/bluealsad "${INIT_DIR}/bluealsad"
    # Install defaults file if provided
    if [ -f "contrib/default/bluealsad" ]; then
        install -m 0644 contrib/default/bluealsad "${DEFAULTS_DIR}/bluealsad"
        echo "Installed defaults to ${DEFAULTS_DIR}/bluealsad"
    fi

    # If no defaults file was provided, create a conservative /etc/default/bluealsad so
    # bluealsad can be started with common profiles enabled (a2dp + sco/hfp).
    if [ ! -f "${DEFAULTS_DIR}/bluealsad" ]; then
        cat > "${DEFAULTS_DIR}/bluealsad" <<'BLUEALSAD_DEFAULT'
# Created by jack-bridge installer - sensible defaults for BlueALSA daemon
BLUEALSAD_USER=bluealsa
# Provide common profile options so audio profiles are available at start
BLUEALSAD_ARGS="-p a2dp-sink -p a2dp-source -p hfp-hf -p hsp-hs"
BLUEALSAD_LOG=/var/log/bluealsad.log
BLUEALSAD_PIDFILE=/var/run/bluealsad.pid
BLUEALSAD_RUNTIME_DIR=/var/lib/bluealsa
BLUEALSAD_EXTRA=""
BLUEALSAD_VERBOSE=0
BLUEALSAD_AUTOSTART=1
BLUEALSAD_NOTES="Defaults provided by jack-bridge installer"
BLUEALSAD_DEFAULT
        chmod 644 "${DEFAULTS_DIR}/bluealsad" || true
        echo "Wrote default ${DEFAULTS_DIR}/bluealsad with common profile args (a2dp/sco)."
    fi
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering bluealsad init script with explicit priorities..."
        # bluealsad after bluetoothd, before jackd-rt
        update-rc.d -f bluealsad remove >/dev/null 2>&1 || true
        update-rc.d bluealsad start 21 2 3 4 5 . stop 79 0 1 6 . || true
    fi
fi

# Install persistent JACK bridge ports init script (Phase 4: Persistent Ports)
if [ -f "contrib/init.d/jack-bridge-ports" ]; then
    echo "Installing jack-bridge-ports init script (persistent JACK bridge clients)..."
    install -m 0755 contrib/init.d/jack-bridge-ports "${INIT_DIR}/jack-bridge-ports"
    
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering jack-bridge-ports init script..."
        # Start after jackd-rt (which uses priority defaults ~20) - use 22 to ensure JACK is ready
        update-rc.d -f jack-bridge-ports remove >/dev/null 2>&1 || true
        update-rc.d jack-bridge-ports start 22 2 3 4 5 . stop 78 0 1 6 . || true
        echo "  ✓ jack-bridge-ports will spawn usb_out, hdmi_out, bt_out at boot"
    fi
else
    echo "Warning: contrib/init.d/jack-bridge-ports not found; persistent ports disabled"
fi

# (autobridge removed) No jack-bluealsa-autobridge init script is installed or registered.


# Install BlueALSA D-Bus policy (canonical)
DBUS_POLICY_SRC="usr/share/dbus-1/system.d/org.bluealsa.conf"
DBUS_POLICY_DST="/usr/share/dbus-1/system.d/org.bluealsa.conf"
if [ -f "$DBUS_POLICY_SRC" ]; then
    echo "Installing org.bluealsa D-Bus policy to $DBUS_POLICY_DST (from $DBUS_POLICY_SRC)"
    mkdir -p "$(dirname "$DBUS_POLICY_DST")"
    install -m 0644 "$DBUS_POLICY_SRC" "$DBUS_POLICY_DST" || true
    # best-effort reload of D-Bus to pick up policy changes
    if command -v service >/dev/null 2>&1; then
        service dbus reload >/dev/null 2>&1 || true
    fi
else
    echo "No bundled canonical D-Bus policy found at $DBUS_POLICY_SRC; assuming distro provides one."
fi

# Install BlueALSA ALSA configuration (PCM/CTL types)
# We use a custom name (20-jack-bridge-bluealsa.conf) to avoid conflicts with distro packages
# that might provide 20-bluealsa.conf.
if [ -f "contrib/etc/20-jack-bridge-bluealsa.conf" ]; then
    echo "Installing BlueALSA ALSA configuration..."
    # Install to both common include locations to support older/newer ALSA layouts
    ALSA_CONF_INSTALLED=0
    for D in /usr/share/alsa/alsa.conf.d /etc/alsa/conf.d; do
        mkdir -p "$D"
        if install -m 0644 contrib/etc/20-jack-bridge-bluealsa.conf "$D/20-jack-bridge-bluealsa.conf"; then
            echo "  ✓ Installed to $D/20-jack-bridge-bluealsa.conf"
            ALSA_CONF_INSTALLED=1
        else
            echo "  ✗ Failed to install to $D/20-jack-bridge-bluealsa.conf"
        fi
        
        # Remove old conflicting file if we previously installed it
        if [ -f "$D/20-bluealsa.conf" ]; then
            if grep -q "jack-bridge" "$D/20-bluealsa.conf" 2>/dev/null; then
                rm -f "$D/20-bluealsa.conf"
                echo "  Removed old jack-bridge config: $D/20-bluealsa.conf"
            fi
        fi
    done
    
    if [ "$ALSA_CONF_INSTALLED" -eq 0 ]; then
        echo "ERROR: Failed to install 20-jack-bridge-bluealsa.conf to any ALSA directory!"
        echo "       Bluetooth audio will NOT work without this configuration."
        exit 1
    fi
else
    echo "ERROR: contrib/etc/20-jack-bridge-bluealsa.conf not found in repository!"
    echo "       This file is required for Bluetooth audio support."
    exit 1
fi

# Verify ALSA can see the jackbridge_bluealsa device
echo "Verifying ALSA configuration..."
if command -v aplay >/dev/null 2>&1; then
    if aplay -L 2>/dev/null | grep -q "jackbridge_bluealsa"; then
        echo "  ✓ ALSA recognizes 'jackbridge_bluealsa' device"
    else
        echo "  ✗ WARNING: ALSA does not recognize 'jackbridge_bluealsa' device"
        echo "            Bluetooth audio may not work. Check ALSA configuration."
    fi
else
    echo "  ! aplay not found; skipping ALSA verification"
fi

# Install polkit rule to authorize BlueZ Adapter/Device operations for users in 'audio' or 'bluetooth'
POLKIT_RULE_SRC="contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
POLKIT_RULE_DST="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
if [ -f "$POLKIT_RULE_SRC" ]; then
    echo "Installing polkit rule to $POLKIT_RULE_DST"
    mkdir -p "$(dirname "$POLKIT_RULE_DST")"
    install -m 0644 "$POLKIT_RULE_SRC" "$POLKIT_RULE_DST" || true
    # best-effort reload of polkit (no systemd dependency)
    if pidof polkitd >/dev/null 2>&1; then
        kill -HUP "$(pidof polkitd | awk '{print $1}')" 2>/dev/null || true
    fi
    if command -v service >/dev/null 2>&1; then
        service polkit restart >/dev/null 2>&1 || true
    fi
else
    echo "Polkit rule not found at $POLKIT_RULE_SRC; skipping (Pair/Connect may prompt/deny without it)."
fi

# --- jack-bridge Bluetooth adapter convenience helper ---
# Create a small helper script that ensures the primary adapter is Discoverable/Pairable
# and sets DiscoverableTimeout=0 (stay discoverable) when invoked. The installer runs
# this once now and installs an init script to run it at boot after bluetoothd.
BLUETOOTH_HELPER="${USR_LIB_DIR}/bluetooth-enable.sh"
mkdir -p "${USR_LIB_DIR}"
cat > "${BLUETOOTH_HELPER}" <<'BLUETOOTH_HELPER_EOF'
#!/bin/sh
# jack-bridge bluetooth-enable.sh
# Best-effort: set hci0 Adapter to Discoverable=true, Pairable=true, DiscoverableTimeout=0
set -e
if ! command -v gdbus >/dev/null 2>&1; then
    echo "gdbus not available; cannot set BlueZ adapter properties"
    exit 0
fi
# Try to set properties on /org/bluez/hci0; do not fail the installer if these fail.
gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.freedesktop.DBus.Properties.Set "org.bluez.Adapter1" "Discoverable" "<true>" >/dev/null 2>&1 || true
gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.freedesktop.DBus.Properties.Set "org.bluez.Adapter1" "Pairable" "<true>" >/dev/null 2>&1 || true
gdbus call --system --dest org.bluez --object-path /org/bluez/hci0 --method org.freedesktop.DBus.Properties.Set "org.bluez.Adapter1" "DiscoverableTimeout" "<uint32 0>" >/dev/null 2>&1 || true
exit 0
BLUETOOTH_HELPER_EOF
chmod 755 "${BLUETOOTH_HELPER}" || true
echo "Installed bluetooth-enable helper to ${BLUETOOTH_HELPER}"

# Run helper now to enable adapter state at install time (best-effort)
echo "Attempting to enable adapter Discoverable/Pairable now (best-effort)..."
"${BLUETOOTH_HELPER}" || true

# Install init script to ensure adapter is configured after boot/startup of bluetoothd
INIT_HELPER="${INIT_DIR}/jack-bridge-bluetooth-config"
cat > "${INIT_HELPER}" <<'INIT_HELPER_EOF'
#!/bin/sh
### BEGIN INIT INFO
# Provides:          jack-bridge-bluetooth-config
# Required-Start:    $local_fs $remote_fs dbus bluetooth
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Configure BlueZ adapter for jack-bridge (discoverable/pairable)
### END INIT INFO

case "$1" in
  start)
    if [ -x "/usr/local/lib/jack-bridge/bluetooth-enable.sh" ]; then
      /usr/local/lib/jack-bridge/bluetooth-enable.sh >/dev/null 2>&1 || true
    fi
    ;;
  stop|restart|status)
    # no-op
    ;;
  *)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 2
    ;;
esac

exit 0
INIT_HELPER_EOF
chmod 755 "${INIT_HELPER}" || true
echo "Installed init script ${INIT_HELPER}"

# Register the init script so it runs after bluetoothd during boot (SysV)
if command -v update-rc.d >/dev/null 2>&1; then
    update-rc.d -f jack-bridge-bluetooth-config remove >/dev/null 2>&1 || true
    # Start early (after bluetoothd which is registered at 20). Use 25 to run after bluetoothd.
    update-rc.d jack-bridge-bluetooth-config start 25 2 3 4 5 . stop 75 0 1 6 . || true
else
    echo "update-rc.d not available; please register ${INIT_HELPER} to run at boot if desired."
fi

# Install jack-bridge devices config (authoritative; overwrite without backup)
mkdir -p /etc/jack-bridge
cat > /etc/jack-bridge/devices.conf <<'DEVCONF'
# /etc/jack-bridge/devices.conf (installed by jack-bridge)
INTERNAL_DEVICE="hw:0"
USB_DEVICE="hw:1"
HDMI_DEVICE="hw:2,3"
BLUETOOTH_DEVICE="jackbridge_bluealsa:PROFILE=a2dp"
# Default Bluetooth output latency (used by jack-route-select when spawning bt_out)
BT_PERIOD="1024"
BT_NPERIODS="3"
# Initial preferred output
PREFERRED_OUTPUT="internal"
DEVCONF
chmod 0644 /etc/jack-bridge/devices.conf
echo "Installed (replaced) /etc/jack-bridge/devices.conf"

# Seed per-user defaults so the GUI (non-root) has an override file on first run (non-destructive)
SKEL_DIR="/etc/skel/.config/jack-bridge"
mkdir -p "$SKEL_DIR"
cat > "$SKEL_DIR/devices.conf" <<'UCONF'
# ~/.config/jack-bridge/devices.conf (user override)
# Initial preferred output; the GUI/helper updates this without root.
PREFERRED_OUTPUT="internal"
# Optional: BLUETOOTH_DEVICE will be written automatically when you select a BT device.
UCONF
chmod 0644 "$SKEL_DIR/devices.conf" || true
echo "Seeded skeleton per-user config at $SKEL_DIR/devices.conf"

# Create for existing desktop users (UID>=1000) if missing (non-destructive)
for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    home_dir="$(getent passwd "$u" | awk -F: '{print $6}')"
    if [ -n "$home_dir" ] && [ -d "$home_dir" ]; then
        user_conf_dir="$home_dir/.config/jack-bridge"
        user_conf="$user_conf_dir/devices.conf"
        if [ ! -f "$user_conf" ]; then
            mkdir -p "$user_conf_dir"
            cp -f "$SKEL_DIR/devices.conf" "$user_conf"
            chown -R "$u:$u" "$user_conf_dir" 2>/dev/null || true
            chmod 0644 "$user_conf" 2>/dev/null || true
            echo "Seeded $user_conf for user $u"
        fi
    fi
done

echo "Installation complete. Please reboot the system to start JACK, bluetoothd (distro), and bluealsad (if installed)."

exit 0
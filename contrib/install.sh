#!/bin/sh
# contrib/install.sh
# Installer for jack-bridge contrib package on Debian-like systems (SysV-style).
# Installs configs into /etc, helper scripts into /usr/local/lib/jack-bridge, init script into /etc/init.d,
# and simple apulse wrappers into /usr/bin. Does not hardcode usernames or devices.
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
# Note: qjackctl removed from packages (we provide custom build in contrib/bin/)
REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse libasound2-plugin-equal swh-plugins libgtk-3-0 bluez bluez-tools dbus policykit-1 imagemagick libasound2-plugin-bluez libb2-1 libqt6core6 libqt6dbus6 libqt6gui6 libqt6network6 libqt6widgets6 libqt6xml6 libts0 qt6-gtk-platformtheme qt6-qpa-plugins qt6-translations-l10n"

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

# Install 50-jack.conf for ALSA to JACK bridging
echo "Installing 50-jack.conf for ALSA to JACK bridging..."

# Detect ALSA configuration directory - try multiple common locations
ALSA_CONF_DIRS="/usr/share/alsa/alsa.conf.d /etc/alsa/conf.d /usr/local/share/alsa/alsa.conf.d /etc/alsa/alsa.conf.d"
ALSA_CONF_INSTALLED=0

for DIR in $ALSA_CONF_DIRS; do
    if [ -d "$DIR" ] || mkdir -p "$DIR"; then
        if [ -f "50-jack.conf" ]; then
            if install -m 0644 50-jack.conf "$DIR/50-jack.conf"; then
                echo "  ✓ Installed 50-jack.conf to $DIR/50-jack.conf"
                ALSA_CONF_INSTALLED=1
                break
            fi
        else
            echo "  ! 50-jack.conf not found in repository root"
            break
        fi
    fi
done

if [ "$ALSA_CONF_INSTALLED" -eq 0 ]; then
    echo "  ✗ WARNING: Could not install 50-jack.conf"
    echo "            ALSA to JACK bridging may not work properly"
    echo "            Please manually install 50-jack.conf to your ALSA configuration directory"
fi

echo "ALSA->JACK bridge uses distro's 50-jack.conf (system:playback)"
echo "Device switching handled by jack-connection-manager (JACK graph routing)"

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
    echo "WARNING: jack-route-select not found"
fi

# Install connection manager (event-driven C binary, not polling script)
if [ -f "contrib/bin/jack-connection-manager" ]; then
    install -m 0755 contrib/bin/jack-connection-manager /usr/local/bin/jack-connection-manager
    echo "Installed event-driven connection manager to /usr/local/bin/jack-connection-manager"
else
    echo "WARNING: jack-connection-manager not found (run 'make manager' to build it)"
fi

# Install autoconnect helper (from contrib; force overwrite)
if [ -f "contrib/usr/lib/jack-bridge/jack-autoconnect" ]; then
    install -m 0755 contrib/usr/lib/jack-bridge/jack-autoconnect "${USR_LIB_DIR}/jack-autoconnect"
    echo "Installed jack-autoconnect to ${USR_LIB_DIR}/jack-autoconnect"
fi

# Install custom qjackctl binary (SYSTEM bus integration)
if [ -f "contrib/bin/qjackctl" ]; then
    echo "Installing custom qjackctl (jack-bridge SYSTEM bus integration)..."
    install -m 0755 contrib/bin/qjackctl /usr/local/bin/qjackctl
    echo "  ✓ Installed custom qjackctl to /usr/local/bin/qjackctl"
    echo "  ✓ This binary connects to SYSTEM D-Bus (not SESSION bus)"
else
    echo "WARNING: Custom qjackctl binary not found at contrib/bin/qjackctl"
    echo "         Run './build-qjackctl.sh' to build it from qjackctl-1.0.4/ source"
    echo "         Installing distro qjackctl as fallback..."
    apt install -y qjackctl || true
    echo "  ! Distro qjackctl uses SESSION bus (will not integrate with jack-bridge)"
fi

# Install qjackctl icon for desktop file
echo "Installing qjackctl icon..."
mkdir -p /usr/share/icons/hicolor/scalable/apps
mkdir -p /usr/share/icons/hicolor/128x128/apps
mkdir -p /usr/share/icons/hicolor/64x64/apps
mkdir -p /usr/share/icons/hicolor/48x48/apps
mkdir -p /usr/share/icons/hicolor/32x32/apps
mkdir -p /usr/share/icons/hicolor/16x16/apps

# Install SVG icon
if [ -f "contrib/usr/share/icons/hicolor/scalable/apps/qjackctl.svg" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/scalable/apps/qjackctl.svg /usr/share/icons/hicolor/scalable/apps/qjackctl.svg
    echo "  ✓ Installed qjackctl SVG icon"
fi

# Install PNG icons
if [ -f "contrib/usr/share/icons/hicolor/128x128/apps/qjackctl.png" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/128x128/apps/qjackctl.png /usr/share/icons/hicolor/128x128/apps/qjackctl.png
    echo "  ✓ Installed qjackctl 128x128 icon"
fi

if [ -f "contrib/usr/share/icons/hicolor/64x64/apps/qjackctl.png" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/64x64/apps/qjackctl.png /usr/share/icons/hicolor/64x64/apps/qjackctl.png
    echo "  ✓ Installed qjackctl 64x64 icon"
fi

if [ -f "contrib/usr/share/icons/hicolor/48x48/apps/qjackctl.png" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/48x48/apps/qjackctl.png /usr/share/icons/hicolor/48x48/apps/qjackctl.png
    echo "  ✓ Installed qjackctl 48x48 icon"
fi

if [ -f "contrib/usr/share/icons/hicolor/32x32/apps/qjackctl.png" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/32x32/apps/qjackctl.png /usr/share/icons/hicolor/32x32/apps/qjackctl.png
    echo "  ✓ Installed qjackctl 32x32 icon"
fi

if [ -f "contrib/usr/share/icons/hicolor/16x16/apps/qjackctl.png" ]; then
    install -m 0644 contrib/usr/share/icons/hicolor/16x16/apps/qjackctl.png /usr/share/icons/hicolor/16x16/apps/qjackctl.png
    echo "  ✓ Installed qjackctl 16x16 icon"
fi

# Refresh icon cache
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

# Install qjackctl desktop file
echo "Installing qjackctl desktop file..."
mkdir -p /usr/share/applications
cat > /usr/share/applications/qjackctl.desktop <<'EOF'
[Desktop Entry]
Name=QjackCtl
Comment=JACK Audio Connection Kit Control Panel
Exec=/usr/local/bin/qjackctl
Icon=qjackctl
Terminal=false
Type=Application
Categories=AudioVideo;Audio;Midi;
StartupNotify=true
EOF
chmod 644 /usr/share/applications/qjackctl.desktop
echo "  ✓ Installed qjackctl desktop file to /usr/share/applications/qjackctl.desktop"

# Update desktop database so menus pick up new .desktop files
echo "Updating desktop database..."
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    echo "  ✓ Updated desktop database"
fi

# Install qjackctl autostart entry
echo "Installing qjackctl autostart entry..."
mkdir -p /etc/xdg/autostart
cat > /etc/xdg/autostart/qjackctl.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=QjackCtl (Auto)
Comment=Launch QjackCtl minimized; JACK is already started by system service
TryExec=/usr/local/bin/qjackctl
Exec=/usr/local/bin/qjackctl --start-minimized
X-GNOME-Autostart-enabled=true
NoDisplay=false
OnlyShowIn=XFCE;LXDE;LXQt;MATE;GNOME;KDE;
EOF
chmod 644 /etc/xdg/autostart/qjackctl.desktop
echo "  ✓ Installed qjackctl autostart entry to /etc/xdg/autostart/qjackctl.desktop"

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

# Create .desktop launcher overrides so desktop environments launch apulse-wrapped browsers
DESKTOP_DIR="/usr/share/applications"
mkdir -p "$DESKTOP_DIR"

# Install realtime limits template (force-install; overwrite without backup)
LIMITS_DST="${ETC_DIR}/security/limits.d/audio.conf"
mkdir -p "$(dirname "$LIMITS_DST")"
install -m 0644 contrib/etc/security/limits.d/audio.conf "$LIMITS_DST" || true
echo "Installed (replaced) realtime limits template to $LIMITS_DST"

# Register init script with update-rc.d if available
# Use priority 02 for start, 98 for stop so jackd stops LAST (after bridge ports)
if command -v update-rc.d >/dev/null 2>&1; then
    echo "Registering jackd-rt init script with update-rc.d..."
    update-rc.d -f jackd-rt remove >/dev/null 2>&1 || true
    update-rc.d jackd-rt defaults 02 98 || true
    echo "  ✓ jackd-rt: starts at priority 02, stops at priority 98 (after dependent services)"
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

# Install BlueALSA prebuilt binaries (required - jack-bridge does not use distro bluez-alsa-utils)
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

    # Always overwrite /etc/default/bluealsad to ensure --keep-alive is present (critical fix)
    # Previous versions were missing this parameter, causing ports to disappear
    cat > "${DEFAULTS_DIR}/bluealsad" <<'BLUEALSAD_DEFAULT'
# Created by jack-bridge installer - sensible defaults for BlueALSA daemon
BLUEALSAD_USER=bluealsa
# --keep-alive=-1 maintains A2DP transport indefinitely (required for persistent JACK ports)
# Provide common profile options so audio profiles are available at start
BLUEALSAD_ARGS="--keep-alive=-1 -p a2dp-sink -p a2dp-source -p hfp-hf -p hsp-hs"
BLUEALSAD_LOG=/var/log/bluealsad.log
BLUEALSAD_PIDFILE=/var/run/bluealsad.pid
BLUEALSAD_RUNTIME_DIR=/var/lib/bluealsa
BLUEALSAD_EXTRA=""
BLUEALSAD_VERBOSE=0
BLUEALSAD_AUTOSTART=1
BLUEALSAD_NOTES="Defaults provided by jack-bridge installer"
BLUEALSAD_DEFAULT
    chmod 644 "${DEFAULTS_DIR}/bluealsad" || true
    echo "Installed (force-overwrite) ${DEFAULTS_DIR}/bluealsad with --keep-alive=-1"
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering bluealsad init script with explicit priorities..."
        # bluealsad must start BEFORE jack-bridge-ports (use defaults which creates S01)
        update-rc.d -f bluealsad remove >/dev/null 2>&1 || true
        update-rc.d bluealsad defaults 01 99 || true
    fi
fi

# Install persistent JACK bridge ports init script (Phase 4: Persistent Ports)
if [ -f "contrib/init.d/jack-bridge-ports" ]; then
    echo "Installing jack-bridge-ports init script (persistent JACK bridge clients)..."
    install -m 0755 contrib/init.d/jack-bridge-ports "${INIT_DIR}/jack-bridge-ports"
    
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering jack-bridge-ports init script..."
        # Start after jackd-rt (S02) and bluealsad (S01) - use priority 04 for proper ordering
        # Stop at priority 02 so it stops BEFORE jackd-rt (K02 < K98)
        update-rc.d -f jack-bridge-ports remove >/dev/null 2>&1 || true
        update-rc.d jack-bridge-ports defaults 04 02 || true
        echo "  ✓ jack-bridge-ports: starts at priority 04 (after JACK), stops at priority 02 (before JACK)"
        echo "  ✓ Bridge ports will spawn usb_out and hdmi_out at boot"
        echo "  ✓ Bluetooth ports spawn on-demand when user selects Bluetooth output"
    fi
else
    echo "Warning: contrib/init.d/jack-bridge-ports not found; persistent ports disabled"
fi

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
        
        # Remove old conflicting file if it was previously installed
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

# Install jack-connection-manager init script (auto-routes based on saved preference)
if [ -f "contrib/init.d/jack-connection-manager" ]; then
    echo "Installing jack-connection-manager init script..."
    install -m 0755 contrib/init.d/jack-connection-manager "${INIT_DIR}/jack-connection-manager"
    
    if command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering jack-connection-manager init script..."
        update-rc.d -f jack-connection-manager remove >/dev/null 2>&1 || true
        # Must run AFTER jack-bridge-ports (priority 04), so use 05
        update-rc.d jack-connection-manager start 05 2 3 4 5 . stop 01 0 1 6 . || true
        echo "  ✓ jack-connection-manager will auto-route based on saved preference"
    fi
else
    echo "WARNING: contrib/init.d/jack-connection-manager not found"
fi

# Install D-Bus service for qjackctl integration
echo ""
echo "Installing jack-bridge D-Bus service for qjackctl integration..."

# Build D-Bus service if not already built
if [ ! -f "contrib/bin/jack-bridge-dbus" ]; then
    echo "Building jack-bridge-dbus..."
    if command -v make >/dev/null 2>&1; then
        make dbus || {
            echo "WARNING: Failed to build jack-bridge-dbus"
            echo "         qjackctl Start/Stop buttons will not work"
            echo "         Run 'make dbus' manually to enable this feature"
        }
    else
        echo "WARNING: make not found, cannot build jack-bridge-dbus"
        echo "         qjackctl Start/Stop buttons will not work"
    fi
fi

# Install D-Bus service binary
if [ -f "contrib/bin/jack-bridge-dbus" ]; then
    install -m 0755 contrib/bin/jack-bridge-dbus /usr/local/bin/jack-bridge-dbus
    echo "  ✓ Installed jack-bridge-dbus to /usr/local/bin"
else
    echo "  ! jack-bridge-dbus binary not found, skipping D-Bus service install"
    echo "    qjackctl Start/Stop will not work without this service"
fi

# Install D-Bus service activation file
if [ -f "contrib/dbus/org.jackaudio.service.service" ]; then
    mkdir -p /usr/share/dbus-1/system-services
    install -m 0644 contrib/dbus/org.jackaudio.service.service \
        /usr/share/dbus-1/system-services/org.jackaudio.service.service
    echo "  ✓ Installed D-Bus service activation file"
fi

# Install D-Bus policy
if [ -f "contrib/dbus/org.jackaudio.service.conf" ]; then
    mkdir -p /usr/share/dbus-1/system.d
    install -m 0644 contrib/dbus/org.jackaudio.service.conf \
        /usr/share/dbus-1/system.d/org.jackaudio.service.conf
    echo "  ✓ Installed D-Bus policy"
else
    echo "  ! D-Bus policy not found, qjackctl may require manual authorization"
fi

# Install polkit rules for password-less control
if [ -f "contrib/polkit/50-jack-bridge.rules" ]; then
    mkdir -p /etc/polkit-1/rules.d
    install -m 0644 contrib/polkit/50-jack-bridge.rules \
        /etc/polkit-1/rules.d/50-jack-bridge.rules
    echo "  ✓ Installed polkit rules (audio group gets password-less JACK control)"
    
    # Reload polkit
    if pidof polkitd >/dev/null 2>&1; then
        kill -HUP $(pidof polkitd | awk '{print $1}') 2>/dev/null || true
    fi
else
    echo "  ! Polkit rules not found, users may be prompted for password"
fi

# Install jack-bridge-dbus init script
if [ -f "contrib/init.d/jack-bridge-dbus" ]; then
    install -m 0755 contrib/init.d/jack-bridge-dbus "${INIT_DIR}/jack-bridge-dbus"
    echo "  ✓ Installed jack-bridge-dbus init script"
    
    # Register with update-rc.d (must start early, before qjackctl launches)
    if command -v update-rc.d >/dev/null 2>&1; then
        update-rc.d -f jack-bridge-dbus remove >/dev/null 2>&1 || true
        update-rc.d jack-bridge-dbus defaults 01 99 || true
        echo "  ✓ Registered jack-bridge-dbus: starts at priority 01 (very early)"
    fi
fi

# Reload D-Bus to pick up new service
if command -v service >/dev/null 2>&1; then
    service dbus reload >/dev/null 2>&1 || true
    echo "  ✓ Reloaded D-Bus configuration"
fi

echo "D-Bus service installation complete"
echo "qjackctl Start/Stop buttons will now control the system JACK service"
echo ""

# Install jack-bridge devices config (authoritative; overwrite without backup)
mkdir -p /etc/jack-bridge
cat > /etc/jack-bridge/devices.conf <<'DEVCONF'
# /etc/jack-bridge/devices.conf (installed by jack-bridge)
INTERNAL_DEVICE="hw:0"
USB_DEVICE="hw:1"
HDMI_DEVICE="hw:2,3"
BLUETOOTH_DEVICE="jackbridge_bluealsa:PROFILE=a2dp"
# Default Bluetooth output latency (period=256 samples @ 48kHz = 5.3ms)
BT_PERIOD="256"
BT_NPERIODS="3"
# Initial preferred output
PREFERRED_OUTPUT="internal"
DEVCONF
chmod 0644 /etc/jack-bridge/devices.conf
echo "Installed (replaced) /etc/jack-bridge/devices.conf with BT_PERIOD=256"

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

# Seed per-user devices.conf for backward compatibility
for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    home_dir="$(getent passwd "$u" | awk -F: '{print $6}')"
    if [ -n "$home_dir" ] && [ -d "$home_dir" ]; then
        user_conf_dir="$home_dir/.config/jack-bridge"
        user_conf="$user_conf_dir/devices.conf"
        
        # Create config directory if needed
        mkdir -p "$user_conf_dir"
        
        # Seed devices.conf if missing
        if [ ! -f "$user_conf" ]; then
            cp -f "$SKEL_DIR/devices.conf" "$user_conf"
            chown "$u:$u" "$user_conf" 2>/dev/null || true
            chmod 0644 "$user_conf" 2>/dev/null || true
            echo "Seeded $user_conf for user $u"
        fi
        
        # Ensure ownership
        chown -R "$u:$u" "$user_conf_dir" 2>/dev/null || true
    fi
done

# Install qjackctl helper scripts to /usr/local/bin
echo ""
echo "Installing qjackctl D-Bus configuration helpers..."
if [ -f "contrib/usr/local/bin/jack-bridge-qjackctl-setup" ]; then
    install -m 0755 contrib/usr/local/bin/jack-bridge-qjackctl-setup /usr/local/bin/jack-bridge-qjackctl-setup
    echo "  ✓ Installed jack-bridge-qjackctl-setup"
fi

if [ -f "contrib/usr/local/bin/jack-bridge-verify-qjackctl" ]; then
    install -m 0755 contrib/usr/local/bin/jack-bridge-verify-qjackctl /usr/local/bin/jack-bridge-verify-qjackctl
    echo "  ✓ Installed jack-bridge-verify-qjackctl"
fi

# Configure qjackctl to use D-Bus mode for jack-bridge integration
# This is CRITICAL - without this, qjackctl spawns its own jackd instead of using our D-Bus service
echo ""
echo "Configuring qjackctl for D-Bus mode integration..."

# Create skeleton config for new users
QJACKCTL_SKEL_DIR="/etc/skel/.config/rncbc.org"
mkdir -p "$QJACKCTL_SKEL_DIR"
cat > "$QJACKCTL_SKEL_DIR/QjackCtl.conf" <<'QJACKCTL_SKEL'
[General]
DBusEnabled=true
JackDBusEnabled=true
StartJack=false
ServerName=default
QJACKCTL_SKEL
chmod 644 "$QJACKCTL_SKEL_DIR/QjackCtl.conf" || true
echo "  ✓ Created skeleton qjackctl config in /etc/skel/"

# Configure qjackctl for each existing desktop user
for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    home_dir="$(getent passwd "$u" | awk -F: '{print $6}')"
    if [ -n "$home_dir" ] && [ -d "$home_dir" ]; then
        qjackctl_conf_dir="$home_dir/.config/rncbc.org"
        qjackctl_conf="$qjackctl_conf_dir/QjackCtl.conf"
        
        # Create config directory
        mkdir -p "$qjackctl_conf_dir"
        
        if [ -f "$qjackctl_conf" ]; then
            # Update existing configuration
            backup_file="${qjackctl_conf}.backup-$(date +%Y%m%d-%H%M%S)"
            cp "$qjackctl_conf" "$backup_file" 2>/dev/null && \
                echo "  ✓ Backed up existing config for user $u"
            
            # Update or add D-Bus settings
            if grep -q '^DBusEnabled=' "$qjackctl_conf"; then
                sed -i 's/^DBusEnabled=.*/DBusEnabled=true/' "$qjackctl_conf"
            else
                echo "DBusEnabled=true" >> "$qjackctl_conf"
            fi
            
            if grep -q '^JackDBusEnabled=' "$qjackctl_conf"; then
                sed -i 's/^JackDBusEnabled=.*/JackDBusEnabled=true/' "$qjackctl_conf"
            else
                echo "JackDBusEnabled=true" >> "$qjackctl_conf"
            fi
            
            echo "  ✓ Updated qjackctl D-Bus settings for user $u"
        else
            # Create new minimal D-Bus-enabled config
            cat > "$qjackctl_conf" <<'QJACKCTL_CONF'
[General]
DBusEnabled=true
JackDBusEnabled=true
StartJack=false
ServerName=default
QJACKCTL_CONF
            echo "  ✓ Created qjackctl D-Bus config for user $u"
        fi
        
        # Fix ownership
        chown -R "$u:$u" "$qjackctl_conf_dir" 2>/dev/null || true
    fi
done

echo ""
echo "qjackctl D-Bus mode configuration complete!"
echo ""
echo "Verification:"
echo "  Users can run: jack-bridge-verify-qjackctl"
echo "  To reconfigure: jack-bridge-qjackctl-setup"
echo ""

echo "============================================================================"
echo "Installation complete! Changes take effect after reboot."
echo ""
echo "Next steps:"
echo "Reboot: sudo reboot"
echo ""
echo "Important: qjackctl is now configured to use jack-bridge D-Bus mode."
echo "           Start/Stop buttons will control the system jackd-rt service."
echo "============================================================================"
echo ""

exit 0


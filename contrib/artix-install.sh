#!/bin/sh
# contrib/artix-install.sh
# Installer for jack-bridge on Artix Linux (OpenRC)
# Installs configs into /etc, helper scripts into /usr/local/lib/jack-bridge,
# init scripts into /etc/init.d. Uses prebuilt BlueALSA binaries.
# No systemd, PulseAudio, or PipeWire.
# Usage: sudo sh contrib/artix-install.sh

set -e

# Require root (system-wide installer)
if [ "$(id -u)" -ne 0 ]; then
    echo "This installer must be run as root. Try: sudo ./contrib/artix-install.sh"
    exit 1
fi

PREFIX_ROOT="/"
ETC_DIR="${PREFIX_ROOT}etc"
USR_LIB_DIR="${PREFIX_ROOT}usr/local/lib/jack-bridge"
INIT_DIR="${PREFIX_ROOT}etc/init.d"
DEFAULTS_DIR="${PREFIX_ROOT}etc/default"
BIN_DIR="${PREFIX_ROOT}usr/bin"

# Register init scripts with OpenRC (or fallback to SysVinit tools)
register_init_script() {
    local script_name="$1"
    local start_priority="$2"
    local stop_priority="$3"
    local custom_args="$4"

    # Primary: OpenRC's rc-update
    if command -v rc-update >/dev/null 2>&1; then
        echo "Registering $script_name with OpenRC (rc-update)..."
        rc-update add "$script_name" default >/dev/null 2>&1 || true
        echo "  ✓ $script_name: added to default runlevel"

    # Fallback: Debian-style update-rc.d
    elif command -v update-rc.d >/dev/null 2>&1; then
        echo "Registering $script_name with update-rc.d..."
        update-rc.d -f "$script_name" remove >/dev/null 2>&1 || true

        if [ -n "$custom_args" ]; then
            update-rc.d "$script_name" $custom_args >/dev/null 2>&1 || true
            echo "  ✓ $script_name: registered with custom args"
        elif [ -n "$start_priority" ] && [ -n "$stop_priority" ]; then
            update-rc.d "$script_name" defaults "$start_priority" "$stop_priority" >/dev/null 2>&1 || true
            echo "  ✓ $script_name: starts at $start_priority, stops at $stop_priority"
        else
            update-rc.d "$script_name" defaults >/dev/null 2>&1 || true
            echo "  ✓ $script_name: registered with defaults"
        fi

    else
        echo "  ! No init system registration tool found (rc-update or update-rc.d)"
        echo "    Please manually register ${INIT_DIR}/$script_name"
    fi
}

# Artix Linux package names
# Core packages from Artix repositories (system, community, galaxy)
ARTIX_REPO_PACKAGES="jack2 alsa-utils alsa-plugins apulse gtk3 gtkmm3 bluez bluez-utils spandsp dbus polkit imagemagick bluez-libs sbc libb2 tslib lv2"

# AUR packages (install via yay or paru)
# swh-plugins is NOT required: the EQ feature was removed from jack-bridge
AUR_PACKAGES=""

echo "Installing jack-bridge contrib files for Artix Linux"

# Verify Artix Linux
if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" != "artix" ]; then
        echo "Warning: This installer is designed for Artix Linux."
        echo "Detected distribution: $ID ($NAME)"
        echo "Continuing anyway..."
    else
        echo "Detected Artix Linux ($VERSION)"
    fi
else
    echo "Warning: /etc/os-release not found. Assuming Artix-compatible system."
fi

# Install packages from Artix repositories
if command -v pacman >/dev/null 2>&1; then
    echo "Detected pacman. Installing Artix repository packages..."
    echo "Packages: $ARTIX_REPO_PACKAGES"
    echo ""

    # Sync repos and install (non-interactive)
    if ! pacman -Sy --noconfirm $ARTIX_REPO_PACKAGES; then
        echo "Error: Package installation failed or was interrupted."
        echo "Required packages must be installed for jack-bridge to function."
        echo ""
        echo "Retry manually:"
        echo "  sudo pacman -S --noconfirm $ARTIX_REPO_PACKAGES"
        exit 1
    fi

    echo ""
    echo "✓ Repository packages installed successfully"
else
    echo "Error: pacman not found. This does not appear to be an Arch/Artix-based system."
    echo "Please install the following packages manually:"
    echo "  $ARTIX_REPO_PACKAGES"
    exit 1
fi

echo "  Note: jack CLI tools (jack_lsp, alsa_out, etc.) are provided by jack2."
echo "        If missing after install, run: pacman -S jack2"

# Cleanup obsolete artifacts from previous versions (authoritative removal)
echo "Checking for obsolete files..."

for f in /etc/init.d/jack-bluealsa-autobridge /usr/local/bin/jack-bluealsa-autobridge /etc/jack-bridge/bluetooth.conf; do
    if [ -e "$f" ]; then
        rm -f "$f"
        echo "  ✓ Removed obsolete $f"
    fi
done

# Remove old BlueALSA config if it was installed by jack-bridge (not by package manager)
for D in /usr/share/alsa/alsa.conf.d /etc/alsa/conf.d; do
    OLD_CONF="$D/20-bluealsa.conf"
    if [ -f "$OLD_CONF" ]; then
        # Check if it's ours (contains "jack-bridge" or "Installed by jack-bridge")
        if grep -q "jack-bridge" "$OLD_CONF" 2>/dev/null; then
            rm -f "$OLD_CONF"
            echo "  ✓ Removed old jack-bridge BlueALSA config: $OLD_CONF"
        fi
    fi
done

# Remove old contrib/etc/20-bluealsa.conf from repo if present (obsolete)
if [ -f "contrib/etc/20-bluealsa.conf" ]; then
    echo "Note: contrib/etc/20-bluealsa.conf is obsolete (replaced by 20-jack-bridge-bluealsa.conf)"
fi

# Install /etc/asound.conf template
echo "Installing ALSA configuration..."
ASOUND_DST="${ETC_DIR}/asound.conf"
mkdir -p "$(dirname "$ASOUND_DST")"

# Force install the updated template (overwrite)
install -m 0644 contrib/etc/asound.conf "$ASOUND_DST"
echo "  ✓ Installed (replaced) $ASOUND_DST"

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
echo "  ✓ Installed default ${ASOUND_D_DIR}/current_input.conf (pcm.current_input -> input_card0)"

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

# Install jackd-rt OpenRC init script
echo "Installing init scripts..."
mkdir -p "$INIT_DIR"
install -m 0755 contrib/openrc/jackd-rt "${INIT_DIR}/jackd-rt"
echo "  ✓ Installed init script to ${INIT_DIR}/jackd-rt"

# Write OpenRC config to /etc/conf.d/jackd-rt (sourced automatically by openrc-run)
mkdir -p /etc/conf.d
cat > /etc/conf.d/jackd-rt <<'JACKD_CONF'
# jack-bridge jackd-rt OpenRC configuration
# Uncomment and set to override auto-detection:
# JACKD_USER=""
# JACKD_DEVICE=""
JACKD_SR=48000
JACKD_PERIOD=256
JACKD_NPERIODS=3
JACKD_PRIORITY=70
JACKD_MIDI="seq"
JACKD_LOG=/var/log/jackd-rt.log
JACKD_CONF
chmod 644 /etc/conf.d/jackd-rt
echo "  ✓ Installed /etc/conf.d/jackd-rt"

# Register with OpenRC
register_init_script jackd-rt "" "" ""

# Install helper scripts (force overwrite)
echo "Installing helper scripts..."
mkdir -p "$USR_LIB_DIR"

install -m 0755 contrib/usr/lib/jack-bridge/detect-alsa-device.sh "${USR_LIB_DIR}/detect-alsa-device.sh"
echo "  ✓ Installed detect-alsa-device.sh to ${USR_LIB_DIR}/"

# Install routing helper used by the GUI Devices panel
if [ -f "contrib/usr/local/lib/jack-bridge/jack-route-select" ]; then
    install -m 0755 contrib/usr/local/lib/jack-bridge/jack-route-select "${USR_LIB_DIR}/jack-route-select"
    echo "  ✓ Installed jack-route-select to ${USR_LIB_DIR}/"
else
    echo "  ! WARNING: jack-route-select not found"
fi

# Install connection manager (event-driven C binary)
if [ -f "contrib/bin/jack-connection-manager" ]; then
    install -m 0755 contrib/bin/jack-connection-manager /usr/local/bin/jack-connection-manager
    echo "  ✓ Installed jack-connection-manager to /usr/local/bin/"
else
    echo "  ! WARNING: jack-connection-manager not found (run 'make manager' to build)"
fi

# Install autoconnect helper
if [ -f "contrib/usr/lib/jack-bridge/jack-autoconnect" ]; then
    install -m 0755 contrib/usr/lib/jack-bridge/jack-autoconnect "${USR_LIB_DIR}/jack-autoconnect"
    echo "  ✓ Installed jack-autoconnect to ${USR_LIB_DIR}/"
fi

# Install pulse-jack-bridge (Steam Runtime 3.0 Sniper audio bridge)
if [ -f "contrib/bin/pulse-jack-bridge" ]; then
    install -m 0755 contrib/bin/pulse-jack-bridge /usr/local/bin/pulse-jack-bridge
    echo "  ✓ Installed pulse-jack-bridge to /usr/local/bin/pulse-jack-bridge"
else
    echo "  ! WARNING: pulse-jack-bridge not found (run 'make bridge' to build it)"
fi

# Install jack-graph binary (JACK/ALSA port connection manager)
echo "Installing jack-graph..."
if [ -f "contrib/bin/jack-graph" ]; then
    install -m 0755 contrib/bin/jack-graph /usr/local/bin/jack-graph
    echo "  ✓ Installed jack-graph to /usr/local/bin/jack-graph"
else
    echo "  ! WARNING: jack-graph not found in contrib/bin/"
    echo "    Build with: cd jack-graph && make"
fi

# Install jack-graph desktop file
if [ -f "contrib/usr/share/applications/jack-graph.desktop" ]; then
    echo "Installing jack-graph desktop file..."
    mkdir -p /usr/share/applications
    install -m 0644 contrib/usr/share/applications/jack-graph.desktop /usr/share/applications/jack-graph.desktop
    echo "  ✓ Installed jack-graph desktop file"

    # Update desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    fi
else
    echo "  ! WARNING: jack-graph.desktop not found"
fi

# Install bundled Alsa Sound Connect GUI
echo "Installing Alsa Sound Connect GUI..."

if [ -f "contrib/bin/mxeq" ]; then
    echo "  Installing mxeq to /usr/local/bin/..."
    mkdir -p /usr/local/bin
    install -m 0755 contrib/bin/mxeq /usr/local/bin/mxeq || true

    # Install desktop entry with correct icon
    if [ -f "contrib/mxeq.desktop" ]; then
        echo "  Installing mxeq desktop file..."
        mkdir -p /usr/share/applications

        # Ensure Icon is set to alsa-sound-connect
        TMPDESK="$(mktemp /tmp/mxeq.desktop.XXXXXX)"
        sed 's/^Icon=.*$/Icon=alsa-sound-connect/' contrib/mxeq.desktop > "$TMPDESK" || cp -f contrib/mxeq.desktop "$TMPDESK"
        install -m 0644 "$TMPDESK" /usr/share/applications/mxeq.desktop || true
        rm -f "$TMPDESK" || true
    fi

    # Install PNG icon
    ICON_SRC_PNG="contrib/usr/share/icons/hicolor/scalable/apps/alsasoundconnectlogo.png"
    ICON_DST_DIR="/usr/share/icons/hicolor/scalable/apps"

    if [ -f "$ICON_SRC_PNG" ]; then
        echo "  Installing icon..."
        mkdir -p "$ICON_DST_DIR"
        install -m 0644 "$ICON_SRC_PNG" "${ICON_DST_DIR}/alsa-sound-connect.png" || true
        echo "  ✓ Installed icon: ${ICON_DST_DIR}/alsa-sound-connect.png"

        # Generate PNG fallbacks at common sizes
        for SZ in 16 32 48 128; do
            DST_DIR="/usr/share/icons/hicolor/${SZ}x${SZ}/apps"
            mkdir -p "$DST_DIR"
            if command -v convert >/dev/null 2>&1; then
                convert "$ICON_SRC_PNG" -resize "${SZ}x${SZ}" "${DST_DIR}/alsa-sound-connect.png" >/dev/null 2>&1 \
                  || cp -f "$ICON_SRC_PNG" "${DST_DIR}/alsa-sound-connect.png"
            else
                cp -f "$ICON_SRC_PNG" "${DST_DIR}/alsa-sound-connect.png" || true
            fi
            echo "  ✓ Installed icon fallback: ${DST_DIR}/alsa-sound-connect.png"
        done

        # Refresh icon cache
        if command -v gtk-update-icon-cache >/dev/null 2>&1; then
            gtk-update-icon-cache -f /usr/share/icons/hicolor >/dev/null 2>&1 || true
        fi
    else
        echo "  ! WARNING: No bundled PNG icon found at $ICON_SRC_PNG"
    fi

    # Refresh desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    fi

    echo "  ✓ Alsa Sound Connect (mxeq) installed"
else
    echo "  ! No bundled Alsa Sound Connect found in contrib/; skipping GUI installation"
fi

# Install realtime limits template
echo "Configuring realtime scheduling limits..."
LIMITS_DST="${ETC_DIR}/security/limits.d/audio.conf"
mkdir -p "$(dirname "$LIMITS_DST")"
install -m 0644 contrib/etc/security/limits.d/audio.conf "$LIMITS_DST" || true
echo "  ✓ Installed realtime limits to $LIMITS_DST"

# Disable PulseAudio autospawn system-wide
# Prevents user-level pulseaudio from blocking ALSA devices
echo "Configuring PulseAudio..."
mkdir -p /etc/pulse/client.conf.d
cat > /etc/pulse/client.conf.d/01-no-autospawn.conf <<'PAEOF'
# Created by jack-bridge installer
autospawn = no
daemon-binary = /bin/true
PAEOF
chmod 644 /etc/pulse/client.conf.d/01-no-autospawn.conf || true
echo "  ✓ Created /etc/pulse/client.conf.d/01-no-autospawn.conf"

# Add desktop users (UID>=1000) to 'audio' group automatically
echo "Configuring user groups..."

for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    if id -nG "$u" 2>/dev/null | grep -qw audio; then
        echo "  User '$u' already in audio group."
    else
        if usermod -aG audio "$u" 2>/dev/null; then
            echo "  ✓ Added '$u' to audio group."
        else
            echo "  ! Warning: failed to add '$u' to audio group."
            echo "    Run: sudo usermod -aG audio $u"
        fi
    fi
done

# Add installer invoker (SUDO_USER) to audio group
if [ -n "$SUDO_USER" ]; then
    if id -nG "$SUDO_USER" 2>/dev/null | grep -qw audio; then
        echo "  SUDO_USER '$SUDO_USER' already in audio group."
    else
        if usermod -aG audio "$SUDO_USER" 2>/dev/null; then
            echo "  ✓ Added SUDO_USER '$SUDO_USER' to audio group."
        else
            echo "  ! Warning: failed to add SUDO_USER '$SUDO_USER' to audio group."
        fi
    fi
fi

# Add desktop users to 'bluetooth' group when present
if getent group bluetooth >/dev/null; then
    echo "  Adding users to bluetooth group..."
    for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
        if id -nG "$u" 2>/dev/null | grep -qw bluetooth; then
            echo "  User '$u' already in bluetooth group."
        else
            if usermod -aG bluetooth "$u" 2>/dev/null; then
                echo "  ✓ Added '$u' to bluetooth group."
            else
                echo "  ! Warning: failed to add '$u' to bluetooth group."
            fi
        fi
    done

    # Add SUDO_USER to bluetooth group
    if [ -n "$SUDO_USER" ]; then
        if id -nG "$SUDO_USER" 2>/dev/null | grep -qw bluetooth; then
            echo "  SUDO_USER '$SUDO_USER' already in bluetooth group."
        else
            if usermod -aG bluetooth "$SUDO_USER" 2>/dev/null; then
                echo "  ✓ Added SUDO_USER '$SUDO_USER' to bluetooth group."
            else
                echo "  ! Warning: failed to add SUDO_USER '$SUDO_USER' to bluetooth group."
            fi
        fi
    fi
else
    echo "  Group 'bluetooth' not present; skipping bluetooth group additions."
fi

echo "  Note: Users added to groups must log out and log back in (or reboot) for changes to take effect."

# Create dedicated bluealsa system user (nologin) if it does not exist
echo "Configuring BlueALSA runtime..."
if ! id -u bluealsa >/dev/null 2>&1; then
    echo "  Creating system user 'bluealsa' (nologin)..."
    if command -v adduser >/dev/null 2>&1; then
        adduser --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    else
        useradd --system --group --no-create-home --shell /usr/sbin/nologin bluealsa || true
    fi
else
    echo "  User 'bluealsa' already exists."
fi

# Create persistent state directory for bluealsa with strict permissions
if [ ! -d /var/lib/bluealsa ]; then
    echo "  Creating /var/lib/bluealsa (0700)..."
    mkdir -p /var/lib/bluealsa
    chown bluealsa:bluealsa /var/lib/bluealsa || true
    chmod 0700 /var/lib/bluealsa || true
else
    echo "  /var/lib/bluealsa already exists; ensuring ownership/perms..."
    chown bluealsa:bluealsa /var/lib/bluealsa 2>/dev/null || true
    chmod 0700 /var/lib/bluealsa 2>/dev/null || true
fi

# Install BlueALSA prebuilt binaries (required - project does not use distro bluez-alsa-utils)
echo "Installing BlueALSA prebuilt binaries..."

if [ -f "contrib/bin/bluealsad" ]; then
    install -m 0755 contrib/bin/bluealsad /usr/local/bin/bluealsad || true
    echo "  ✓ Installed bluealsad"
else
    echo "  ERROR: contrib/bin/bluealsad not found! BlueALSA daemon required for Bluetooth audio."
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

# Install matching BlueALSA ALSA plugins
echo "Installing BlueALSA ALSA plugins..."

# Detect architecture for correct plugin directory
ALSA_PLUGIN_DIR="/usr/lib/alsa-lib"  # Artix default path
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
        echo "  ✓ Backed up distro plugin"
    fi
    install -m 0644 contrib/bin/libasound_module_pcm_bluealsa.so "$ALSA_PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
    echo "  ✓ Installed libasound_module_pcm_bluealsa.so (PCM plugin)"
else
    echo "  ! WARNING: libasound_module_pcm_bluealsa.so not found in contrib/bin/"
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
    echo "  ! WARNING: libasound_module_ctl_bluealsa.so not found (optional - mixer controls)"
fi

# Install bluealsad init script and defaults
echo "Configuring BlueALSA service..."

if [ -f "contrib/openrc/bluealsad" ]; then
    echo "  Installing bluealsad OpenRC init script..."
    install -m 0755 contrib/openrc/bluealsad "${INIT_DIR}/bluealsad"

    # Write OpenRC config to /etc/conf.d/bluealsad
    mkdir -p /etc/conf.d
    cat > /etc/conf.d/bluealsad <<'BLUEALSAD_CONF'
# jack-bridge bluealsad OpenRC configuration
BLUEALSAD_USER=bluealsa
# --keep-alive=-1 maintains A2DP transport indefinitely (required for persistent JACK ports)
BLUEALSAD_ARGS="--keep-alive=-1 -p a2dp-sink -p a2dp-source -p hfp-hf -p hsp-hs"
BLUEALSAD_LOG=/var/log/bluealsad.log
BLUEALSAD_CONF
    chmod 644 /etc/conf.d/bluealsad
    echo "  ✓ Installed /etc/conf.d/bluealsad"

    # Register with OpenRC
    register_init_script bluealsad "" "" ""

else
    echo "  ! No contrib/openrc/bluealsad found; using distro bluez-alsa if available"
fi

# On Artix, bluez-openrc provides /etc/init.d/bluetooth — do NOT install our custom
# bluetoothd on top. The distro's bluetooth service satisfies the Required-Start:bluetooth
# dependency in bluealsad and jack-bridge-ports. Installing ours causes a double-start conflict.
echo "  Artix: Using distro 'bluetooth' service from bluez-openrc; skipping custom bluetoothd"

# Install JACK bridge ports OpenRC init script
if [ -f "contrib/openrc/jack-bridge-ports" ]; then
    echo "Installing jack-bridge-ports init script (JACK bridge port management)..."
    install -m 0755 contrib/openrc/jack-bridge-ports "${INIT_DIR}/jack-bridge-ports"

    register_init_script jack-bridge-ports "" "" ""

    echo "  ✓ jack-bridge-ports: restores HDMI/USB routing at boot; Bluetooth is on-demand"
else
    echo "  ! WARNING: contrib/openrc/jack-bridge-ports not found; bridge port management disabled"
fi

# Install BlueALSA D-Bus policy (canonical)
DBUS_POLICY_SRC="usr/share/dbus-1/system.d/org.bluealsa.conf"
DBUS_POLICY_DST="/usr/share/dbus-1/system.d/org.bluealsa.conf"

if [ -f "$DBUS_POLICY_SRC" ]; then
    echo "Installing org.bluealsa D-Bus policy to $DBUS_POLICY_DST..."
    mkdir -p "$(dirname "$DBUS_POLICY_DST")"
    install -m 0644 "$DBUS_POLICY_SRC" "$DBUS_POLICY_DST" || true

    # Best-effort reload of D-Bus to pick up policy changes
    if command -v service >/dev/null 2>&1; then
        service dbus reload >/dev/null 2>&1 || true
    fi
else
    echo "  ! No bundled canonical D-Bus policy found at $DBUS_POLICY_SRC"
    echo "    Assuming distro provides one"
fi

# Install BlueALSA ALSA configuration (PCM/CTL types)
# Use custom name (20-jack-bridge-bluealsa.conf) to avoid conflicts with distro packages
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
                echo "  ✓ Removed old jack-bridge config: $D/20-bluealsa.conf"
            fi
        fi
    done

    if [ "$ALSA_CONF_INSTALLED" -eq 0 ]; then
        echo "  ERROR: Failed to install 20-jack-bridge-bluealsa.conf to any ALSA directory!"
        echo "         Bluetooth audio will NOT work without this configuration."
        exit 1
    fi
else
    echo "  ERROR: contrib/etc/20-jack-bridge-bluealsa.conf not found in repository!"
    echo "         This file is required for Bluetooth audio support."
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

# Install polkit rule to authorize BlueZ Adapter/Device operations
# for users in 'audio' or 'bluetooth' groups
POLKIT_RULE_SRC="contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"
POLKIT_RULE_DST="/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules"

if [ -f "$POLKIT_RULE_SRC" ]; then
    echo "Installing polkit rule to $POLKIT_RULE_DST..."
    mkdir -p "$(dirname "$POLKIT_RULE_DST")"
    install -m 0644 "$POLKIT_RULE_SRC" "$POLKIT_RULE_DST" || true

    # Best-effort reload of polkit (no systemd dependency)
    if pidof polkitd >/dev/null 2>&1; then
        kill -HUP "$(pidof polkitd | awk '{print $1}')" 2>/dev/null || true
    fi
    if command -v service >/dev/null 2>&1; then
        service polkit restart >/dev/null 2>&1 || true
    fi
else
    echo "  ! Polkit rule not found at $POLKIT_RULE_SRC"
    echo "    Skipping (Pair/Connect may prompt/deny without it)"
fi

# Install polkit rule for jack-graph to manage jackd-rt service
JACK_POLKIT_RULE_SRC="contrib/polkit/50-jack-bridge.rules"
JACK_POLKIT_RULE_DST="/etc/polkit-1/rules.d/50-jack-bridge.rules"

if [ -f "$JACK_POLKIT_RULE_SRC" ]; then
    echo "Installing jack-graph polkit rule to $JACK_POLKIT_RULE_DST..."
    mkdir -p "$(dirname "$JACK_POLKIT_RULE_DST")"
    install -m 0644 "$JACK_POLKIT_RULE_SRC" "$JACK_POLKIT_RULE_DST" || true

    # Reload polkit
    if pidof polkitd >/dev/null 2>&1; then
        kill -HUP "$(pidof polkitd | awk '{print $1}')" 2>/dev/null || true
    fi
    if command -v service >/dev/null 2>&1; then
        service polkit restart >/dev/null 2>&1 || true
    fi
else
    echo "  ! WARNING: jack-graph polkit rule not found at $JACK_POLKIT_RULE_SRC"
fi

# Install jack-bridge service helper script
if [ -f "contrib/usr/local/lib/jack-bridge/jack-bridge-service-helper" ]; then
    echo "Installing jack-bridge-service-helper script..."
    mkdir -p "${USR_LIB_DIR}"
    install -m 0755 contrib/usr/local/lib/jack-bridge/jack-bridge-service-helper "${USR_LIB_DIR}/jack-bridge-service-helper"
    echo "  ✓ Installed jack-bridge-service-helper to ${USR_LIB_DIR}/"
else
    echo "  ! WARNING: jack-bridge-service-helper not found"
fi

# Create bluetooth-enable.sh helper script
echo "Configuring Bluetooth adapter..."
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
gdbus call --system --object-path /org/bluez/hci0 --method org.freedesktop.DBus.Properties.Set "org.bluez.Adapter1" "DiscoverableTimeout" "<uint32 0>" >/dev/null 2>&1 || true
exit 0
BLUETOOTH_HELPER_EOF

chmod 755 "${BLUETOOTH_HELPER}" || true
echo "  ✓ Installed bluetooth-enable helper to ${BLUETOOTH_HELPER}"

# Run helper now to enable adapter state at install time (best-effort)
echo "  Attempting to enable adapter Discoverable/Pairable now (best-effort)..."
"${BLUETOOTH_HELPER}" || true

# Install jack-bridge-bluetooth-config OpenRC init script
if [ -f "contrib/openrc/jack-bridge-bluetooth-config" ]; then
    install -m 0755 contrib/openrc/jack-bridge-bluetooth-config "${INIT_DIR}/jack-bridge-bluetooth-config"
    echo "  ✓ Installed init script ${INIT_DIR}/jack-bridge-bluetooth-config"
    register_init_script jack-bridge-bluetooth-config "" "" ""
else
    echo "  ! WARNING: contrib/openrc/jack-bridge-bluetooth-config not found"
fi

# Install jack-connection-manager OpenRC init script
if [ -f "contrib/openrc/jack-connection-manager" ]; then
    echo "Installing jack-connection-manager init script..."
    install -m 0755 contrib/openrc/jack-connection-manager "${INIT_DIR}/jack-connection-manager"

    register_init_script jack-connection-manager "" "" ""

    echo "  ✓ jack-connection-manager will auto-route based on saved preference"
else
    echo "  ! WARNING: contrib/openrc/jack-connection-manager not found"
fi

# Install jack-bridge devices config (authoritative; overwrite without backup)
echo "Configuring device settings..."
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
echo "  ✓ Installed (replaced) /etc/jack-bridge/devices.conf with BT_PERIOD=256"

# Seed per-user defaults so the GUI (non-root) has an override file on first run
echo "Seeding per-user configuration..."
SKEL_DIR="/etc/skel/.config/jack-bridge"
mkdir -p "$SKEL_DIR"
cat > "$SKEL_DIR/devices.conf" <<'UCONF'
# ~/.config/jack-bridge/devices.conf (user override)
# Initial preferred output; the GUI/helper updates this without root.
PREFERRED_OUTPUT="internal"
# Optional: BLUETOOTH_DEVICE will be written automatically when you select a BT device.
UCONF

chmod 0644 "$SKEL_DIR/devices.conf" || true
echo "  ✓ Seeded skeleton per-user config at $SKEL_DIR/devices.conf"

# Seed per-user devices.conf for existing users (backward compatibility)
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
            echo "  ✓ Seeded $user_conf for user $u"
        fi

        # Ensure ownership
        chown -R "$u:$u" "$user_conf_dir" 2>/dev/null || true
    fi
done

echo "Verifying service registration..."
if command -v rc-update >/dev/null 2>&1; then
    echo "OpenRC default runlevel services:"
    rc-update show default 2>/dev/null || true
fi
echo ""
echo "Checking jack CLI tools..."
TOOLS_OK=1
for tool in jack_lsp jack_connect jack_disconnect jack_samplerate jack_wait alsa_out alsa_in; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo "  ✓ $tool: $(command -v "$tool")"
    else
        echo "  ✗ $tool: NOT FOUND"
        TOOLS_OK=0
    fi
done
if [ "$TOOLS_OK" -eq 0 ]; then
    echo ""
    echo "  WARNING: Some jack CLI tools are missing. See above."
    echo "  Installed binaries in jack2 package:"
    pacman -Ql jack2 2>/dev/null | grep '/usr/bin/' || echo "    (could not query)"
    echo "  To fix: pacman -S jack2  (or install jack-example-tools from AUR manually)"
fi
echo ""
echo "After first reboot, check service status with:"
echo "  rc-status"
echo "  cat /var/log/jackd-rt.log"
echo ""

echo "============================================================================"
echo "============================================================================"
echo "Installation complete! Changes take effect after reboot."
echo ""
echo "Stopping user-level PipeWire services..."
for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
    pkill -u "$u" -x pipewire       2>/dev/null || true
    pkill -u "$u" -x wireplumber    2>/dev/null || true
    pkill -u "$u" -x pipewire-pulse 2>/dev/null || true
    echo "  ✓ Stopped PipeWire processes for user $u (if any were running)"
done
echo "  Note: PipeWire may restart at next login. To permanently disable it, run as your user:"
echo "        rc-update --user del pipewire default"
echo "        rc-update --user del pipewire-pulse default"
echo "        rc-update --user del wireplumber default"
echo ""
echo "Next steps:"
echo "Reboot: sudo reboot"
echo ""
echo "Use jack-graph to view and connect JACK/ALSA ports visually."
echo "Use the Alsa Sound Connect GUI (mxeq) to switch outputs and manage Bluetooth."
echo "============================================================================"
echo ""

exit 0

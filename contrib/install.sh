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

REQUIRED_PACKAGES="jackd2 alsa-utils alsa-plugins apulse"
OPTIONAL_PACKAGES="qjackctl"

echo "Installing jack-bridge contrib files (non-destructive)..."

# Prompt admin to install packages (allow password entry via sudo)
if command -v apt-get >/dev/null 2>&1; then
    echo "Detected apt-get. The following packages are recommended/required:"
    echo "  Required: $REQUIRED_PACKAGES"
    echo "  Optional: $OPTIONAL_PACKAGES"
    printf "Install required packages now? [Y/n] "
    # read is interactive; default to Y
    read install_resp
    install_resp=${install_resp:-Y}
    case "$(printf '%s' "$install_resp" | tr '[:upper:]' '[:lower:]')" in
        y|yes|'')
            echo "Installing required packages (this will prompt for your sudo password)..."
            if ! sudo apt-get update; then
                echo "Warning: apt-get update failed; continuing to install may still work."
            fi
            if ! sudo apt-get install -y $REQUIRED_PACKAGES; then
                echo "Package installation failed or was interrupted. Please install required packages manually:"
                echo "  sudo apt-get install -y $REQUIRED_PACKAGES"
                echo "Continuing installation of config files (admin must resolve missing packages before use)."
            fi
            ;;
        *)
            echo "Skipping package installation. Ensure $REQUIRED_PACKAGES are installed before using the setup."
            ;;
    esac
else
    echo "apt-get not found. Please ensure these packages are installed: $REQUIRED_PACKAGES"
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

# Install helper scripts
mkdir -p "$USR_LIB_DIR"
cp -p contrib/usr/lib/jack-bridge/detect-alsa-device.sh "${USR_LIB_DIR}/detect-alsa-device.sh"
chmod 755 "${USR_LIB_DIR}/detect-alsa-device.sh"
echo "Installed helper detect script to ${USR_LIB_DIR}/detect-alsa-device.sh"

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

# Detect legacy snd-aloop init script presence and suggest removal (do NOT remove automatically)
LEGACY_INIT="/etc/init.d/snd-aloop-load"
if [ -f "$LEGACY_INIT" ]; then
    echo ""
    echo "Detected legacy snd-aloop init script at $LEGACY_INIT."
    echo "Note: This contrib flow does not rely on snd_aloop by default and shipping a legacy init script can confuse users."
    printf "Do you want to disable and remove the legacy snd-aloop init script? [y/N] "
    read remove_resp
    remove_resp=${remove_resp:-N}
    case "$(printf '%s' "$remove_resp" | tr '[:upper:]' '[:lower:]')" in
        y|yes)
            if command -v update-rc.d >/dev/null 2>&1; then
                sudo update-rc.d -f snd-aloop-load remove || true
            fi
            sudo rm -f "$LEGACY_INIT" || true
            echo "Removed $LEGACY_INIT"
            ;;
        *)
            echo "Left $LEGACY_INIT in place. If you later want to remove it to avoid confusion, run 'sudo rm -f $LEGACY_INIT' and unregister from runlevels."
            ;;
    esac
fi

echo ""
echo "Installation complete. Next steps for administrators:"
echo " - Ensure users who will run JACK are in the 'audio' group."
echo " - Confirm /etc/security/limits.d/audio.conf provides rtprio and memlock settings for @audio group."
echo " - If you want system-wide ALSA->JACK behavior, ensure /etc/asound.conf is present (we installed a template if none existed)."
echo " - Start the service: sudo service jackd-rt start"
echo " - To run browsers under apulse wrappers: use 'apulse-firefox' or 'apulse-chromium'."
echo ""
echo "For troubleshooting, see contrib/TROUBLESHOOTING.md."
echo ""
echo "Note: This installer makes reasonable defaults but is intentionally non-destructive."
exit 0
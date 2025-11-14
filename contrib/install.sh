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

REQUIRED_PACKAGES="jackd2 alsa-utils alsa-plugins apulse qjackctl"

echo "Installing jack-bridge contrib files (non-destructive)..."

# Prompt admin to install packages (allow password entry via sudo)
if command -v apt >/dev/null 2>&1; then
    echo "Detected apt. The following packages are recommended/required:"
    echo "  Required: $REQUIRED_PACKAGES"
    if [ -n "$OPTIONAL_PACKAGES" ]; then
        echo "  Optional: $OPTIONAL_PACKAGES"
    fi
    printf "Install required packages now? [Y/n] "
    # read is interactive; default to Y
    read install_resp
    install_resp=${install_resp:-Y}
    case "$(printf '%s' "$install_resp" | tr '[:upper:]' '[:lower:]')" in
        y|yes|'')
            echo "Installing required packages (this will prompt for your sudo password)..."
            if ! sudo apt update; then
                echo "Warning: apt update failed; continuing to install may still work."
            fi
            if ! sudo apt install -y $REQUIRED_PACKAGES; then
                echo "Package installation failed or was interrupted. Please install required packages manually:"
                echo "  sudo apt install -y $REQUIRED_PACKAGES"
                echo "Continuing installation of config files (admin must resolve missing packages before use)."
            fi
            ;;
        *)
            echo "Skipping package installation. Ensure $REQUIRED_PACKAGES are installed before using the setup."
            ;;
    esac
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

# Option: start and verify service now (JACK server should be up without user action)
printf "Start and verify jackd-rt service now? [Y/n] "
read start_now
start_now=${start_now:-Y}
case "$(printf '%s' "$start_now" | tr '[:upper:]' '[:lower:]')" in
    y|yes|'')
        echo "Starting jackd-rt service..."
        if command -v invoke-rc.d >/dev/null 2>&1; then
            if ! invoke-rc.d jackd-rt start; then
                echo "Warning: invoke-rc.d failed; trying 'service jackd-rt start'..."
                service jackd-rt start || true
            fi
        else
            service jackd-rt start || true
        fi

        # Wait for JACK readiness
        if command -v jack_wait >/dev/null 2>&1; then
            if ! jack_wait -w -t 8 >/dev/null 2>&1; then
                echo "Warning: JACK did not become ready in time; last 30 lines of /var/log/jackd-rt.log:"
                tail -n 30 /var/log/jackd-rt.log 2>/dev/null || true
            else
                echo "JACK is up."
            fi
        else
            sleep 1
            if ! { [ -s /var/run/jackd-rt.pid ] && kill -0 "$(cat /var/run/jackd-rt.pid 2>/dev/null)" >/dev/null 2>&1; }; then
                echo "Warning: jackd may not be running; last 30 lines of /var/log/jackd-rt.log:"
                tail -n 30 /var/log/jackd-rt.log 2>/dev/null || true
            else
                pid=$(cat /var/run/jackd-rt.pid 2>/dev/null || true)
                echo "JACK appears to be running (pid $pid)."
            fi
        fi
        ;;
    *)
        echo "Skipping immediate service start. JACK will start automatically at next boot."
        ;;
esac

# Optional: disable PulseAudio autospawn (non-destructive; does not purge packages)
printf "Disable PulseAudio autospawn system-wide (create /etc/pulse/client.conf.d/01-no-autospawn.conf)? [y/N] "
read pa_disable
pa_disable=${pa_disable:-N}
case "$(printf '%s' "$pa_disable" | tr '[:upper:]' '[:lower:]')" in
    y|yes)
        mkdir -p /etc/pulse/client.conf.d
        cat > /etc/pulse/client.conf.d/01-no-autospawn.conf <<'PAEOF'
# Created by jack-bridge contrib installer
autospawn = no
daemon-binary = /bin/true
PAEOF
        chmod 644 /etc/pulse/client.conf.d/01-no-autospawn.conf || true
        echo "Created /etc/pulse/client.conf.d/01-no-autospawn.conf"
        ;;
    *)
        echo "Keeping PulseAudio autospawn behavior unchanged."
        ;;
esac

# Guidance for PipeWire (no changes performed automatically)
echo "Note: If PipeWire is installed, you may disable its PulseAudio compatibility without purging by disabling user/session autostarts."
echo "      On systemd-based user sessions: systemctl --user mask --now pipewire-pulse.service pipewire.service pipewire.socket"
echo "      On non-systemd sessions, disable any XDG autostart entries for pipewire/pipewire-pulse."

# Optional: install qjackctl autostart entry (GUI convenience; server already runs at boot)
printf "Install qjackctl autostart for desktop sessions (launch minimized on login)? [y/N] "
read qjc_auto
qjc_auto=${qjc_auto:-N}
case "$(printf '%s' "$qjc_auto" | tr '[:upper:]' '[:lower:]')" in
    y|yes)
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
        ;;
    *)
        echo "Skipping qjackctl autostart entry."
        ;;
esac

# Optional: add desktop users (UID>=1000) to 'audio' group
printf "Add desktop users (UID>=1000) to the 'audio' group now? [y/N] "
read add_audio
add_audio=${add_audio:-N}
case "$(printf '%s' "$add_audio" | tr '[:upper:]' '[:lower:]')" in
    y|yes)
        for u in $(awk -F: '$3>=1000 && $3<65534 {print $1}' /etc/passwd); do
            if id -nG "$u" 2>/dev/null | grep -qw audio; then
                echo "User '$u' already in audio group."
            else
                if usermod -aG audio "$u" 2>/dev/null; then
                    echo "Added '$u' to audio group."
                else
                    echo "Warning: failed to add '$u' to audio group."
                fi
            fi
        done
        echo "Note: Users may need to log out and back in for new group membership to take effect in their sessions."
        ;;
    *)
        echo "Leaving 'audio' group membership unchanged."
        ;;
esac

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
echo "Installation complete."
echo " - JACK server is configured to start automatically at boot via 'jackd-rt' SysV service."
echo " - If you chose to start it now, it should already be running."
echo " - ALSA apps will route to JACK via /etc/asound.conf by default."
echo " - For PulseAudio-only apps, use the apulse wrappers: 'apulse-firefox' or 'apulse-chromium'."
echo " - Ensure desktop users are in the 'audio' group and that /etc/security/limits.d/audio.conf grants rtprio/memlock."
echo ""
echo "Quick validation (optional):"
echo "   aplay -D default /usr/share/sounds/alsa/Front_Center.wav"
echo "If there is no sound, inspect: tail -n 50 /var/log/jackd-rt.log"
echo ""
echo "For troubleshooting, see contrib/TROUBLESHOOTING.md."
echo "This installer applies reasonable defaults and is intentionally non-destructive."
exit 0
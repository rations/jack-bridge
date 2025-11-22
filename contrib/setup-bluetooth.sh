#!/bin/sh
# contrib/setup-bluetooth.sh
# Provision runtime for BlueALSA and create GUI module stubs.
# Usage: sudo sh contrib/setup-bluetooth.sh [TARGET_USER]
#
# Actions:
# - create system user 'bluealsa' (nologin) if missing
# - create /var/lib/bluealsa owned by bluealsa:bluealsa mode 0700
# - install D-Bus policy file if present in repo (usr/share/dbus-1/system.d/org.bluealsa.conf)
# - install polkit rule for BlueZ if present (contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules)
# - add TARGET_USER (or first audio-group user) to 'audio' (and 'bluetooth' if present) groups
# - create C GUI stub files: src/bt_agent.c, src/gui_bt.c, src/bt_bridge.c if they don't exist
#
# Notes:
# - Autobridge has been removed. Routing is handled via JACK using
#   /usr/local/lib/jack-bridge/jack-route-select and settings in
#   /etc/jack-bridge/devices.conf (installed by the main installer).
# - This script does not manage devices.conf; use contrib/install.sh for that.
# This is intentionally portable POSIX sh for Debian-like systems without systemd.

set -eu

TARGET_USER="${1:-}"

# Helper: print and run
run() {
    printf '%s\n' "$*"
    sh -c "$*"
}

# Create bluealsa system user if missing
if ! id bluealsa >/dev/null 2>&1; then
    printf 'Creating system user bluealsa (nologin)\n'
    if command -v useradd >/dev/null 2>&1; then
        useradd --system --no-create-home --shell /usr/sbin/nologin --user-group bluealsa || true
    else
        printf 'useradd not found; please create user bluealsa manually\n'
    fi
else
    printf 'User bluealsa exists\n'
fi

# Ensure /var/lib/bluealsa exists with correct ownership and mode
BLUEALSA_DIR="/var/lib/bluealsa"
if [ ! -d "$BLUEALSA_DIR" ]; then
    mkdir -p "$BLUEALSA_DIR" || true
fi
chown bluealsa:bluealsa "$BLUEALSA_DIR" >/dev/null 2>&1 || true
chmod 0700 "$BLUEALSA_DIR" >/dev/null 2>&1 || true
printf "Ensured %s owned bluealsa:bluealsa mode 0700\n" "$BLUEALSA_DIR"

# Autobridge removed: no log provisioning required.
printf "Note: autobridge removed; routing via jack-route-select and /etc/jack-bridge/devices.conf\n"

# Install D-Bus policy if provided in repo (best-effort)
if [ -f "usr/share/dbus-1/system.d/org.bluealsa.conf" ]; then
    printf "Installing D-Bus policy to /usr/share/dbus-1/system.d/\n"
    install -m 0644 usr/share/dbus-1/system.d/org.bluealsa.conf /usr/share/dbus-1/system.d/org.bluealsa.conf || true
else
    printf "No D-Bus policy found in repo path usr/share/dbus-1/system.d/org.bluealsa.conf - skipping copy\n"
fi

# Install polkit rule to authorize BlueZ Adapter/Device operations for audio/bluetooth groups (best-effort)
if [ -f "contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules" ]; then
    printf "Installing polkit rule to /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules\n"
    install -m 0644 contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules || true
else
    printf "No polkit rule found at contrib/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules - skipping copy\n"
fi

# Best-effort reload of dbus and polkit to pick up new policies (no systemd dependency)
if command -v service >/dev/null 2>&1; then
    service dbus reload >/dev/null 2>&1 || true
    service polkit restart >/dev/null 2>&1 || true
fi
# HUP polkitd if running (fallback)
if pidof polkitd >/dev/null 2>&1; then
    kill -HUP "$(pidof polkitd | awk '{print $1}')" >/dev/null 2>&1 || true
fi

# Add target user to audio group
if [ -z "$TARGET_USER" ]; then
    # try to find first user in audio group with uid >=1000
    TARGET_USER="$(getent group audio | awk -F: '{print $4}' | cut -d, -f1)"
    if [ -z "$TARGET_USER" ]; then
        # fallback: first user with uid >=1000
        TARGET_USER="$(awk -F: '($3>=1000)&&($1!="nobody"){print $1; exit}' /etc/passwd || true)"
    fi
fi

if [ -n "$TARGET_USER" ]; then
    if id "$TARGET_USER" >/dev/null 2>&1; then
        printf "Adding %s to audio group\n" "$TARGET_USER"
        if command -v usermod >/dev/null 2>&1; then
            usermod -a -G audio "$TARGET_USER" >/dev/null 2>&1 || true
        else
            printf "usermod not available; please add %s to audio group manually\n" "$TARGET_USER"
        fi
        if getent group bluetooth >/dev/null 2>&1; then
            printf "Adding %s to bluetooth group (if not already)\n" "$TARGET_USER"
            if command -v usermod >/dev/null 2>&1; then
                usermod -a -G bluetooth "$TARGET_USER" >/dev/null 2>&1 || true
            else
                printf "usermod not available; please add %s to bluetooth group manually\n" "$TARGET_USER"
            fi
        else
            printf "Group 'bluetooth' not present; skipping bluetooth group add for %s\n" "$TARGET_USER"
        fi
    else
        printf "Target user %s does not exist; skipping group adds\n" "$TARGET_USER"
    fi
else
    printf "No target user determined; skipping group adds\n"
fi

# Create GUI C module stubs if missing
mkdir -p src

create_stub() {
    path="$1"
    if [ -f "$path" ]; then
        printf "Stub %s already exists - skipping\n" "$path"
        return
    fi
    cat > "$path" <<'EOF'
/*
 * PATH: PLACEHOLDER
 * Minimal stub for GUI Bluetooth integration. Implement D-Bus Agent and GUI hooks here.
 * This file is created by contrib/setup-bluetooth.sh and should be extended in C.
 */

#include <stdio.h>

/* Replace with real headers (glib/gio/gtk) when implementing */
int bt_stub_main(void) {
    printf("This is a placeholder for " "PLACEHOLDER" "\n");
    return 0;
}
EOF
    # Replace placeholder tag with actual path name inside file
    sed -i "s|PLACEHOLDER|$path|g" "$path" || true
    chmod 0644 "$path"
    printf "Created stub %s\n" "$path"
}

create_stub "src/bt_agent.c"
create_stub "src/gui_bt.c"
create_stub "src/bt_bridge.c"

printf "Provisioning and stubs creation complete.\n"
printf "Next: implement D-Bus Agent and GUI integration in the created stubs, and run this script at package install time if desired.\n"
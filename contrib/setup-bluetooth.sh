#!/bin/sh
# contrib/setup-bluetooth.sh
# Provision runtime for BlueALSA and create GUI module stubs.
# Usage: sudo sh contrib/setup-bluetooth.sh [TARGET_USER]
#
# Actions:
# - create system user 'bluealsa' (nologin) if missing
# - create /var/lib/bluealsa owned by bluealsa:bluealsa mode 0700
# - ensure /var/log/jack-bluealsa-autobridge.log exists and owned by root (or bluealsa)
# - install D-Bus policy file if present in repo (usr/share/dbus-1/system.d/org.bluealsa.conf)
# - add TARGET_USER (or first audio-group user) to 'audio' group
# - create C GUI stub files: src/bt_agent.c, src/gui_bt.c, src/bt_bridge.c if they don't exist
#
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

# Ensure log file exists
LOG_FILE="/var/log/jack-bluealsa-autobridge.log"
if [ ! -f "$LOG_FILE" ]; then
    touch "$LOG_FILE" >/dev/null 2>&1 || true
fi
# chown to bluealsa if available, else keep root
if id bluealsa >/dev/null 2>&1; then
    chown bluealsa:bluealsa "$LOG_FILE" >/dev/null 2>&1 || true
fi
chmod 0644 "$LOG_FILE" >/dev/null 2>&1 || true
printf "Ensured log %s\n" "$LOG_FILE"

# Install D-Bus policy if provided in repo (best-effort)
if [ -f "usr/share/dbus-1/system.d/org.bluealsa.conf" ]; then
    printf "Installing D-Bus policy to /usr/share/dbus-1/system.d/\n"
    install -m 0644 usr/share/dbus-1/system.d/org.bluealsa.conf /usr/share/dbus-1/system.d/org.bluealsa.conf || true
else
    printf "No D-Bus policy found in repo path usr/share/dbus-1/system.d/org.bluealsa.conf - skipping copy\n"
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
    else
        printf "Target user %s does not exist; skipping audio group add\n" "$TARGET_USER"
    fi
else
    printf "No target user determined; skipping audio group add\n"
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
#!/bin/sh
# bluetooth-enable.sh - ensure BlueZ adapter(s) are powered/discoverable/pairable on sysvinit systems
# Installs into /usr/local/lib/jack-bridge by contrib/install.sh
set -eu

LOG="/var/log/jack-bridge-bluetooth-config.log"
umask 022
mkdir -p "$(dirname "$LOG")" 2>/dev/null || true
touch "$LOG" 2>/dev/null || true

log() { echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG"; }

require_gdbus() {
  if ! command -v gdbus >/dev/null 2>&1; then
    log "gdbus not found; install 'dbus' package"
    exit 1
  fi
}

wait_for_bluez() {
  i=0
  while [ $i -lt 10 ]; do
    if gdbus call --system --dest org.freedesktop.DBus \
        --object-path /org/freedesktop/DBus \
        --method org.freedesktop.DBus.NameHasOwner org.bluez 2>/dev/null | grep -q true; then
      return 0
    fi
   sleep 1
   i=$((i+1))
  done
  return 1
}

list_adapters() {
  # Extract /org/bluez/hci* object paths from GetManagedObjects
  gdbus call --system --dest org.bluez --object-path / \
    --method org.freedesktop.DBus.ObjectManager.GetManagedObjects \
    | tr ',' '\n' | grep -o "/org/bluez/hci[0-9]\+" | sort -u
}

set_prop_bool() {
  ap="$1"; key="$2"; val="$3"
  gdbus call --system --dest org.bluez --object-path "$ap" \
    --method org.freedesktop.DBus.Properties.Set \
    "org.bluez.Adapter1" "$key" "<$val>" >/dev/null 2>&1 || return 1
}

set_prop_u32() {
  ap="$1"; key="$2"; val="$3"
  gdbus call --system --dest org.bluez --object-path "$ap" \
    --method org.freedesktop.DBus.Properties.Set \
    "org.bluez.Adapter1" "$key" "<uint32 $val>" >/dev/null 2>&1 || return 1
}

get_prop() {
  ap="$1"; key="$2"
  gdbus call --system --dest org.bluez --object-path "$ap" \
    --method org.freedesktop.DBus.Properties.Get \
    "org.bluez.Adapter1" "$key" 2>/dev/null || true
}

main() {
  require_gdbus
  if ! wait_for_bluez; then
    log "org.bluez not available on system bus (skipping)"
    exit 0
  fi

  ADAPTERS="$(list_adapters || true)"
  if [ -z "$ADAPTERS" ]; then
    log "No adapters found under /org/bluez"
    exit 0
  fi

  for A in $ADAPTERS; do
    log "Configuring $A"

    # Ensure Powered=true
   if ! get_prop "$A" "Powered" | grep -q '<true>'; then
     if set_prop_bool "$A" "Powered" "true"; then
       log "Powered=true"
       sleep 1
     else
       log "Failed to set Powered=true (continuing)"
     fi
   fi

   # Pairable and Discoverable are controlled by GUI toggle for security
   # Set PairableTimeout to 0 so when user enables pairing via GUI, it doesn't auto-disable
   set_prop_u32 "$A" "PairableTimeout" 0 && log "PairableTimeout=0" || log "Failed to set PairableTimeout"
   
   # Note: Both Pairable and Discoverable default to false (secure).
   # Users enable them via GUI toggle when they want to pair devices.
   # This gives users full control over Bluetooth visibility and pairing.

   # Show final state summary
   STATE="$(gdbus call --system --dest org.bluez --object-path "$A" \
     --method org.freedesktop.DBus.Properties.GetAll "org.bluez.Adapter1" 2>/dev/null || true)"
   log "Adapter1 state: $STATE"
  done
}

main "$@"
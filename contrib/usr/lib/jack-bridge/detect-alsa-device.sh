#!/bin/sh
# contrib/usr/lib/jack-bridge/detect-alsa-device.sh
# Strict POSIX sh, no here-docs into functions, no eval. Works under /bin/sh (dash).
# Prints a device like "hw:CARD=Name" or "hw:0" on stdout and exits 0.

set -eu
# Wait for devices to settle (helpful for slow hardware where USB might appear before Internal)
sleep 5

aplay_cmd=$(command -v aplay 2>/dev/null || true)
arecord_cmd=$(command -v arecord 2>/dev/null || true)

# Fallback if tools missing
if [ -z "${aplay_cmd}" ] || [ -z "${arecord_cmd}" ]; then
    echo "hw:0"
    exit 0
fi

# Capture listings
aplay_out=$("${aplay_cmd}" -l 2>/dev/null || true)
arecord_out=$("${arecord_cmd}" -l 2>/dev/null || true)

# Extract lines "card N: NAME [DESC]" -> "N|NAME|DESC" for playback/capture
# Note: sed regex handles the brackets to extract description
aplay_cards=$(printf '%s\n' "$aplay_out"   | sed -n 's/^card \([0-9][0-9]*\): \([^[]*\)\[\([^]]*\)\].*/\1|\2|\3/p')
arecord_cards=$(printf '%s\n' "$arecord_out" | sed -n 's/^card \([0-9][0-9]*\): \([^[]*\)\[\([^]]*\)\].*/\1|\2|\3/p')

FOUND_IDX=""
FOUND_NAME=""

# Iterate playback cards, prefer non-loopback present in capture list
printf '%s\n' "$aplay_cards" | while IFS= read -r line; do
    [ -n "$line" ] || continue
    idx=$(echo "$line" | cut -d'|' -f1)
    name=$(echo "$line" | cut -d'|' -f2)
    desc=$(echo "$line" | cut -d'|' -f3)
    
    # trim spaces around name
    name=$(printf '%s' "$name" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    
    case "$name" in
        *Loopback*|*Loop\ Back*) continue ;;
    esac
    
    # check same index in capture (simple check)
    if printf '%s\n' "$arecord_cards" | grep -q "^${idx}|"; then
        # If this is a USB device (check name OR desc), skip it in this priority pass
        if echo "$name $desc" | grep -qi "USB"; then
            continue
        fi
        printf '%s|%s\n' "$idx" "$name"
        exit 0
    fi
    
    # check same name (case-insensitive, fixed string)
    if printf '%s\n' "$arecord_cards" | grep -Fiq "|${name}|"; then
        # If this is a USB device, skip it in this priority pass
        if echo "$name $desc" | grep -qi "USB"; then
            continue
        fi
        printf '%s|%s\n' "$idx" "$name"
        exit 0
    fi
done > /tmp/jb_detect_choice.$$ 2>/dev/null || true

if [ -s /tmp/jb_detect_choice.$$ ]; then
    choice=$(cat /tmp/jb_detect_choice.$$ 2>/dev/null || true)
    rm -f /tmp/jb_detect_choice.$$ 2>/dev/null || true
    FOUND_IDX=${choice%%|*}
    FOUND_NAME=$(echo "$choice" | cut -d'|' -f2)
else
    rm -f /tmp/jb_detect_choice.$$ 2>/dev/null || true
    # Fallback: try to find non-USB non-loopback first
    # Check both name and desc for "USB"
    first=$(printf '%s\n' "$aplay_cards" | grep -vi "USB" | sed -n '/Loopback/!{/Loop Back/!p;}' | sed -n '1p')
    if [ -z "$first" ]; then
        # If no non-USB, take any non-loopback (e.g. USB)
        first=$(printf '%s\n' "$aplay_cards" | sed -n '/Loopback/!{/Loop Back/!p;}' | sed -n '1p')
    fi
    if [ -n "$first" ]; then
        FOUND_IDX=$(echo "$first" | cut -d'|' -f1)
        FOUND_NAME=$(echo "$first" | cut -d'|' -f2)
        FOUND_NAME=$(printf '%s' "$FOUND_NAME" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    fi
fi

# Final fallback
if [ -z "${FOUND_IDX}" ]; then
    echo "hw:0"
    exit 0
fi

# Sanitize name for CARD= usage
SANITIZED=$(printf '%s' "$FOUND_NAME" | sed 's/[^A-Za-z0-9_-]/_/g' | cut -c1-32)

# Prefer CARD=name if name appears in playback list (case-insensitive)
if [ -n "$SANITIZED" ] && printf '%s\n' "$aplay_out" | grep -Fiq "$SANITIZED"; then
    echo "hw:CARD=$SANITIZED"
    exit 0
fi

# Else numeric index
echo "hw:$FOUND_IDX"
exit 0
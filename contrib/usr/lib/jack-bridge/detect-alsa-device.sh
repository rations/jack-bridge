#!/bin/sh
# contrib/usr/lib/jack-bridge/detect-alsa-device.sh
# Strict POSIX sh, no here-docs into functions, no eval. Works under /bin/sh (dash).
# Prints a device like "hw:CARD=Name" or "hw:0" on stdout and exits 0.
#
# CRITICAL: This script chooses the MAIN JACK device (what jackd opens with -d alsa -D).
# USB interfaces must NEVER be selected here - they should only be used via alsa_out bridges.
# If USB hijacks the main device, it blocks alsa_out and causes "Device or resource busy" errors.

set -eu
# Wait for udev to settle and apply persistent naming rules (increased for reliability)
sleep 5

# Logging function for diagnostics
LOGFILE="/tmp/jack-bridge-detect.log"
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') [detect-alsa-device] $*" >> "$LOGFILE" 2>/dev/null || true
}

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
log "Raw aplay output: $aplay_out"
log "Raw arecord output: $arecord_out"

# Extract lines "card N: NAME [DESC]" -> "N|NAME|DESC" for playback/capture
# Note: sed regex handles the brackets to extract description
aplay_cards=$(printf '%s\n' "$aplay_out"   | sed -n 's/^card \([0-9][0-9]*\): \([^[]*\)\[\([^]]*\)\].*/\1|\2|\3/p')
arecord_cards=$(printf '%s\n' "$arecord_out" | sed -n 's/^card \([0-9][0-9]*\): \([^[]*\)\[\([^]]*\)\].*/\1|\2|\3/p')
log "Parsed aplay_cards: $aplay_cards"
log "Parsed arecord_cards: $arecord_cards"

FOUND_IDX=""
FOUND_NAME=""

# Score function: higher score = better choice for JACK main device
score_card() {
    name="$1"
    desc="$2"
    score=5

    # Exclude USB devices entirely (score 0)
    if echo "$name $desc" | grep -qi "usb"; then
        score=0
    fi

    # Highest priority: persistently named "Internal" card
    if [ "$name" = "Internal" ]; then
        score=20
    fi

    # Boost internal/onboard devices by name patterns
    if echo "$name" | grep -qiE "(intel|pch|hdmi|hda|alc|analog)"; then
        score=10
    fi

    echo "$score"
}

# Simple solution: return the first non-USB card that has capture support
log "Finding first non-USB card with capture..."

# Use IFS to split the cards variable
old_IFS="$IFS"
IFS='
'
for line in $aplay_cards; do
    IFS="$old_IFS"
    [ -n "$line" ] || continue
    idx=$(echo "$line" | cut -d'|' -f1)
    name=$(echo "$line" | cut -d'|' -f2)
    desc=$(echo "$line" | cut -d'|' -f3)

    # trim spaces around name
    name=$(printf '%s' "$name" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

    log "Checking card $idx: name='$name', desc='$desc'"

    case "$name" in
        *Loopback*|*Loop\ Back*)
            log "Skipping loopback card $idx"
            continue ;;
    esac

    # Skip USB devices
    if echo "$name $desc" | grep -qi "usb"; then
        log "Skipping USB card $idx"
        continue
    fi

    # Must be present in capture list (same index or same name)
    in_capture=0
    if echo "$arecord_cards" | grep -q "^${idx}|"; then
        in_capture=1
        log "Card $idx found in capture by index"
    elif echo "$arecord_cards" | grep -Fiq "|${name}|"; then
        in_capture=1
        log "Card $idx found in capture by name"
    fi

    if [ "$in_capture" -eq 1 ]; then
        FOUND_IDX=$idx
        FOUND_NAME=$name
        log "Selected first non-USB card: idx=$FOUND_IDX, name='$FOUND_NAME'"
        break
    else
        log "Card $idx not in capture, skipping"
    fi
    IFS='
'
done
IFS="$old_IFS"

if [ -z "$FOUND_IDX" ]; then
    log "No non-USB cards found, using fallback..."
    # Robust fallback: prioritize Internal, then any non-USB, then force card 0 if not USB
    first=$(printf '%s\n' "$aplay_cards" | grep -viE "USB|Loopback|Loop Back" | grep -iE "Internal|PCH|HDA|Intel|Analog|HDMI" | sed -n '1p')

    if [ -z "$first" ]; then
        first=$(printf '%s\n' "$aplay_cards" | grep -viE "USB|Loopback|Loop Back" | sed -n '1p')
    fi

    if [ -z "$first" ]; then
        # Force card 0 if it exists and is not USB (last resort)
        first=$(printf '%s\n' "$aplay_cards" | grep "^0|" | sed -n '1p')
        if [ -n "$first" ]; then
            name=$(echo "$first" | cut -d'|' -f2)
            desc=$(echo "$first" | cut -d'|' -f3)
            if echo "$name $desc" | grep -qi "usb"; then
                first=""  # Don't use USB card 0
                log "Rejected USB card 0"
            fi
        fi
    fi

    if [ -n "$first" ]; then
        FOUND_IDX=$(echo "$first" | cut -d'|' -f1)
        FOUND_NAME=$(echo "$first" | cut -d'|' -f2)
        FOUND_NAME=$(printf '%s' "$FOUND_NAME" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        log "Fallback selected: idx=$FOUND_IDX, name='$FOUND_NAME'"
    else
        log "No fallback card found!"
    fi
fi

# Final fallback
if [ -z "${FOUND_IDX}" ]; then
    log "Using final fallback: hw:0"
    echo "hw:0"
    exit 0
fi

log "Final selection: idx=$FOUND_IDX, name='$FOUND_NAME'"

# Sanitize name for CARD= usage
SANITIZED=$(printf '%s' "$FOUND_NAME" | sed 's/[^A-Za-z0-9_-]/_/g' | cut -c1-32)

# Prefer CARD=name if name appears in playback list (case-insensitive)
if [ -n "$SANITIZED" ] && printf '%s\n' "$aplay_out" | grep -Fiq "$SANITIZED"; then
    result="hw:CARD=$SANITIZED"
    log "Output: $result"
    echo "$result"
    exit 0
fi

# Else numeric index
result="hw:$FOUND_IDX"
log "Output: $result"
echo "$result"
exit 0
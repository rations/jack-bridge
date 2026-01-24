#!/bin/sh
# contrib/usr/lib/jack-bridge/detect-alsa-device.sh
# Strict POSIX sh, no here-docs into functions, no eval. Works under /bin/sh (dash).
# Prints a device like "hw:CARD=Name" or "hw:0" on stdout and exits 0.
#
# CRITICAL: This script chooses the MAIN JACK device (what jackd opens with -d alsa -D).
# USB interfaces must NEVER be selected here - they should only be used via alsa_out bridges.
# If USB hijacks the main device, it blocks alsa_out and causes "Device or resource busy" errors.

set -eu

# Logging function
log() {
    printf "%s [detect-alsa-device] %s\n" "$(date --iso-8601=seconds)" "$*" >> /tmp/jack-bridge-detect.log
}

log "Starting ALSA device detection"
# Wait for udev to settle - increased to 5s to handle USB device timing variations
sleep 5
log "Slept 5 seconds for udev to settle"

aplay_cmd=$(command -v aplay 2>/dev/null || true)
arecord_cmd=$(command -v arecord 2>/dev/null || true)

# Fallback if tools missing
if [ -z "${aplay_cmd}" ] || [ -z "${arecord_cmd}" ]; then
    echo "hw:0"
    exit 0
fi

# Capture listings
log "Running aplay -l and arecord -l"
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

# Check if this card is USB by looking at raw aplay output
is_usb_card() {
    local card_idx="$1"
    # Look for "USB" anywhere in the card's section in raw aplay output
    # Use awk to extract just this card's section
    printf '%s\n' "$aplay_out" | awk "
        /^card ${card_idx}:/ { in_card=1; print; next }
        in_card && /^card [0-9]+:/ && !/^card ${card_idx}:/ { exit }
        in_card { print }
    " | grep -qi "usb"
}

# Check if we have any non-USB cards
non_usb_count=$(printf '%s\n' "$aplay_cards" | while IFS= read -r line; do
    [ -n "$line" ] || continue
    idx=$(echo "$line" | cut -d'|' -f1)
    if ! is_usb_card "$idx"; then
        echo "non-usb"
        exit 0
    fi
done | wc -l)
log "Found $non_usb_count non-USB cards"
if [ "$non_usb_count" -eq 0 ]; then
    log "WARNING: No non-USB cards found! This will cause USB override issues."
fi

FOUND_IDX=""
FOUND_NAME=""

log "Finding first non-USB card with capture..."
# Iterate playback cards, prefer non-loopback present in capture list
printf '%s\n' "$aplay_cards" | while IFS= read -r line; do
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

    # check same index in capture (simple check)
    if printf '%s\n' "$arecord_cards" | grep -q "^${idx}|"; then
        log "Card $idx found in capture by index"
        # If this is a USB device, skip it in this priority pass
        if is_usb_card "$idx"; then
            log "Card $idx is USB - skipping"
            continue
        else
            log "Card $idx is not USB - selecting"
        fi
        log "Selected card $idx: name='$name'"
        printf '%s|%s\n' "$idx" "$name"
        exit 0
    fi

    # check same name (case-insensitive, fixed string)
    if printf '%s\n' "$arecord_cards" | grep -Fiq "|${name}|"; then
        log "Card $idx found in capture by name"
        # If this is a USB device, skip it in this priority pass
        if is_usb_card "$idx"; then
            log "Skipping USB card $idx"
            continue
        fi
        log "Selected card $idx: name='$name'"
        printf '%s|%s\n' "$idx" "$name"
        exit 0
    fi
done > /tmp/jb_detect_choice.$$ 2>/dev/null || true

if [ -s /tmp/jb_detect_choice.$$ ]; then
    choice=$(cat /tmp/jb_detect_choice.$$ 2>/dev/null || true)
    rm -f /tmp/jb_detect_choice.$$ 2>/dev/null || true
    FOUND_IDX=${choice%%|*}
    FOUND_NAME=$(echo "$choice" | cut -d'|' -f2)
    log "Selected from primary search: idx=$FOUND_IDX, name='$FOUND_NAME'"
else
    rm -f /tmp/jb_detect_choice.$$ 2>/dev/null || true
    log "Primary search failed, trying fallback patterns"
    # Fallback: MUST find non-USB device for JACK main device
    # Try multiple patterns for internal/builtin audio
    first=$(printf '%s\n' "$aplay_cards" | while IFS= read -r line; do
        [ -n "$line" ] || continue
        idx=$(echo "$line" | cut -d'|' -f1)
        name=$(echo "$line" | cut -d'|' -f2)
        desc=$(echo "$line" | cut -d'|' -f3)
        name=$(printf '%s' "$name" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

        case "$name" in
            *Loopback*|*Loop\ Back*) continue ;;
        esac

        # Skip USB cards
        if is_usb_card "$idx"; then
            continue
        fi

        # Prefer PCH/HDA/Intel/Analog/HDMI patterns
        if echo "$name $desc" | grep -iE "PCH|HDA|Intel|Analog|HDMI" >/dev/null; then
            echo "$line"
            exit 0
        fi
    done)

    if [ -z "$first" ]; then
        log "No PCH/HDA/Intel/Analog/HDMI cards found, trying any non-USB"
        # Try any non-USB, non-loopback
        first=$(printf '%s\n' "$aplay_cards" | while IFS= read -r line; do
            [ -n "$line" ] || continue
            idx=$(echo "$line" | cut -d'|' -f1)
            name=$(echo "$line" | cut -d'|' -f2)
            name=$(printf '%s' "$name" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

            case "$name" in
                *Loopback*|*Loop\ Back*) continue ;;
            esac

            # Skip USB cards
            if is_usb_card "$idx"; then
                continue
            fi

            echo "$line"
            exit 0
        done)
    fi

    if [ -z "$first" ]; then
        log "No non-USB cards found, falling back to card 0"
        # LAST RESORT: Take card 0 if it exists (should be internal)
        first=$(printf '%s\n' "$aplay_cards" | grep "^0|" | sed -n '1p')
    fi

    # NEVER fallback to USB for JACK's main device - it causes "Device busy" errors
    # USB devices should ONLY be used via alsa_out bridges spawned by jack-bridge-ports

    if [ -n "$first" ]; then
        FOUND_IDX=$(echo "$first" | cut -d'|' -f1)
        FOUND_NAME=$(echo "$first" | cut -d'|' -f2)
        FOUND_NAME=$(printf '%s' "$FOUND_NAME" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        log "Selected from fallback: idx=$FOUND_IDX, name='$FOUND_NAME'"
    else
        log "ERROR: No suitable cards found!"
    fi
fi

# Final fallback
if [ -z "${FOUND_IDX}" ]; then
    log "Final fallback: using hw:0"
    echo "hw:0"
    exit 0
fi

log "Final selection: idx=$FOUND_IDX, name='$FOUND_NAME'"

# Sanitize name for CARD= usage
SANITIZED=$(printf '%s' "$FOUND_NAME" | sed 's/[^A-Za-z0-9_-]/_/g' | cut -c1-32)

# Prefer CARD=name if name appears in playback list (case-insensitive)
if [ -n "$SANITIZED" ] && printf '%s\n' "$aplay_out" | grep -Fiq "$SANITIZED"; then
    log "Output: hw:CARD=$SANITIZED"
    echo "hw:CARD=$SANITIZED"
    exit 0
fi

# Else numeric index
log "Output: hw:$FOUND_IDX"
echo "hw:$FOUND_IDX"
exit 0
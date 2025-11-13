#!/bin/sh
# detect-alsa-device.sh
# POSIX shell script that attempts to find a sane full-duplex ALSA device.
# Prints a device string suitable for jackd (e.g. hw:0 or hw:CARD=USB).
# Exit status 0 on success, non-zero on failure.

# Strategy:
# 1) Look for a card that appears in both aplay -l and arecord -l (playback+capture).
# 2) Prefer non-loopback cards (skip 'Loopback' in name).
# 3) If none found, fall back to hw:0
# 4) Print concise device identifier: hw:CARD=<name> if available, else hw:<index>

aplay_cmd=$(command -v aplay 2>/dev/null || true)
arecord_cmd=$(command -v arecord 2>/dev/null || true)

# If tools missing, fallback to hw:0
if [ -z "$aplay_cmd" ] || [ -z "$arecord_cmd" ]; then
    echo "hw:0"
    exit 0
fi

# Parse card listings
aplay_list=$($aplay_cmd -l 2>/dev/null)
arecord_list=$($arecord_cmd -l 2>/dev/null)

# helper to get card indices/names into lines "index:name"
parse_cards() {
    echo "$1" | awk -F'[:[]' '/^card [0-9]+:/ { idx=$2; sub(/^ /,"",idx); name=$3; gsub(/^[ \t]+|[ \t]+$/,"",name); print idx ":" name }'
}

aplay_cards=$(parse_cards "$aplay_list")
arecord_cards=$(parse_cards "$arecord_list")

best_card=""
for aline in $(printf "%s\n" "$aplay_cards"); do
    idx=$(printf "%s\n" "$aline" | cut -d: -f1)
    name=$(printf "%s\n" "$aline" | cut -d: -f2-)
    # skip Loopback devices
    case "$name" in
        *Loopback*|*Loop Back*) continue ;;
    esac
    match_found=0
    for rline in $(printf "%s\n" "$arecord_cards"); do
        ridx=$(printf "%s\n" "$rline" | cut -d: -f1)
        rname=$(printf "%s\n" "$rline" | cut -d: -f2-)
        if [ "$idx" = "$ridx" ] || [ "$name" = "$rname" ]; then
            match_found=1
            break
        fi
    done
    if [ "$match_found" -eq 1 ]; then
        best_card="$idx:$name"
        break
    fi
done

# If none matched by index/name, try any non-loopback from aplay
if [ -z "$best_card" ]; then
    for aline in $(printf "%s\n" "$aplay_cards"); do
        idx=$(printf "%s\n" "$aline" | cut -d: -f1)
        name=$(printf "%s\n" "$aline" | cut -d: -f2-)
        case "$name" in
            *Loopback*|*Loop Back*) continue ;;
        esac
        best_card="$idx:$name"
        break
    done
fi

# If still empty, fallback to hw:0
if [ -z "$best_card" ]; then
    echo "hw:0"
    exit 0
fi

card_idx=$(printf "%s\n" "$best_card" | cut -d: -f1)
card_name=$(printf "%s\n" "$best_card" | cut -d: -f2-)

# sanitize card_name to safe token
sanitized=$(printf "%s\n" "$card_name" | sed 's/[^A-Za-z0-9_/-]/_/g' | cut -c1-32)
if [ -n "$sanitized" ]; then
    # Verify that the sanitized name exists in aplay output (best-effort)
    if echo "$aplay_list" | grep -qi "$sanitized"; then
        echo "hw:CARD=$sanitized"
        exit 0
    fi
fi

# Fallback to numeric index
echo "hw:$card_idx"
exit 0
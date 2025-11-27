#!/bin/sh
# Quick Bluetooth audio verification script for jack-bridge
# Usage: sh contrib/test-bluetooth.sh [MAC_ADDRESS]
# If MAC_ADDRESS not provided, will attempt to use first discovered device

set +e  # Continue on errors to show complete diagnostic

echo "=== jack-bridge Bluetooth Audio Test ==="
echo

# Colors for output (if terminal supports)
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    GREEN=''; RED=''; YELLOW=''; NC=''
fi

pass() { printf "${GREEN}✓${NC} %s\n" "$1"; }
fail() { printf "${RED}✗${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}!${NC} %s\n" "$1"; }

MAC="${1:-}"

echo "Step 1: Verify Services"
echo "========================"
if pidof bluetoothd >/dev/null 2>&1; then
    pass "bluetoothd running"
else
    fail "bluetoothd NOT running"
fi

if pidof bluealsad >/dev/null 2>&1; then
    pass "bluealsad running"
else
    fail "bluealsad NOT running"
fi

if pidof jackd >/dev/null 2>&1; then
    pass "jackd running"
else
    fail "jackd NOT running - cannot test JACK routing"
fi
echo

echo "Step 2: Verify ALSA Configuration"
echo "==================================="
if [ -f /usr/share/alsa/alsa.conf.d/20-jack-bridge-bluealsa.conf ] || \
   [ -f /etc/alsa/conf.d/20-jack-bridge-bluealsa.conf ]; then
    pass "20-jack-bridge-bluealsa.conf installed"
else
    fail "20-jack-bridge-bluealsa.conf NOT installed - run installer"
fi

if command -v aplay >/dev/null 2>&1; then
    if aplay -L 2>/dev/null | grep -q "jackbridge_bluealsa"; then
        pass "ALSA recognizes 'jackbridge_bluealsa' device"
    else
        fail "ALSA does NOT recognize 'jackbridge_bluealsa' device"
        echo "    Run: sudo cp contrib/etc/20-jack-bridge-bluealsa.conf /usr/share/alsa/alsa.conf.d/"
    fi
fi
echo

echo "Step 3: Verify ALSA Plugins"
echo "============================"
PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/alsa-lib"
if [ ! -d "$PLUGIN_DIR" ]; then
    for d in /usr/lib/*/alsa-lib; do
        if [ -d "$d" ]; then
            PLUGIN_DIR="$d"
            break
        fi
    done
fi

if [ -f "$PLUGIN_DIR/libasound_module_pcm_bluealsa.so" ]; then
    pass "PCM plugin: $PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
    ls -lh "$PLUGIN_DIR/libasound_module_pcm_bluealsa.so"
else
    fail "PCM plugin NOT found at $PLUGIN_DIR"
fi

if [ -f "$PLUGIN_DIR/libasound_module_ctl_bluealsa.so" ]; then
    pass "CTL plugin: $PLUGIN_DIR/libasound_module_ctl_bluealsa.so"
else
    warn "CTL plugin not found (optional)"
fi
echo

echo "Step 4: Verify Persistent JACK Ports"
echo "======================================"
if command -v jack_lsp >/dev/null 2>&1; then
    if jack_lsp 2>/dev/null | grep -q '^usb_out:'; then
        pass "usb_out ports exist"
    else
        warn "usb_out ports NOT found"
    fi
    
    if jack_lsp 2>/dev/null | grep -q '^hdmi_out:'; then
        pass "hdmi_out ports exist"
    else
        warn "hdmi_out ports NOT found"
    fi
    
    if jack_lsp 2>/dev/null | grep -q '^bt_out:'; then
        pass "bt_out ports exist"
    else
        fail "bt_out ports NOT found - run: sudo service jack-bridge-ports restart"
    fi
    
    echo
    echo "All JACK ports:"
    jack_lsp 2>/dev/null | grep "_out:" | sed 's/^/  /'
else
    warn "jack_lsp not available"
fi
echo

echo "Step 5: Bluetooth Device Status"
echo "================================="
if [ -z "$MAC" ]; then
    echo "No MAC address provided. Checking for known devices..."
    if command -v bluetoothctl >/dev/null 2>&1; then
        DEVICES=$(bluetoothctl devices 2>/dev/null | head -5)
        if [ -n "$DEVICES" ]; then
            echo "Known devices:"
            echo "$DEVICES" | sed 's/^/  /'
            FIRST_MAC=$(echo "$DEVICES" | head -1 | awk '{print $2}')
            if [ -n "$FIRST_MAC" ]; then
                echo
                echo "To test with first device, run:"
                echo "  sh contrib/test-bluetooth.sh $FIRST_MAC"
            fi
        else
            warn "No known Bluetooth devices. Pair a device first."
        fi
    fi
else
    echo "Testing with MAC: $MAC"
    if command -v bluetoothctl >/dev/null 2>&1; then
        INFO=$(bluetoothctl info "$MAC" 2>/dev/null)
        if echo "$INFO" | grep -q "Paired: yes"; then
            pass "Device $MAC is paired"
        else
            warn "Device $MAC is NOT paired"
        fi
        
        if echo "$INFO" | grep -q "Trusted: yes"; then
            pass "Device $MAC is trusted"
        else
            warn "Device $MAC is NOT trusted"
        fi
        
        if echo "$INFO" | grep -q "Connected: yes"; then
            pass "Device $MAC is connected"
        else
            warn "Device $MAC is NOT connected"
            echo "    Run: bluetoothctl connect $MAC"
        fi
    fi
fi
echo

echo "Step 6: Test ALSA Direct Playback"
echo "==================================="
if [ -n "$MAC" ]; then
    if [ -f /usr/share/sounds/alsa/Front_Center.wav ]; then
        echo "Testing: aplay -D jackbridge_bluealsa:DEV=$MAC,PROFILE=a2dp ..."
        if aplay -D "jackbridge_bluealsa:DEV=$MAC,PROFILE=a2dp,SRV=org.bluealsa" \
                /usr/share/sounds/alsa/Front_Center.wav 2>&1; then
            pass "ALSA direct playback succeeded"
        else
            fail "ALSA direct playback failed"
            echo "    Check: /var/log/bluealsad.log"
        fi
    else
        warn "Test wav file not found; skipping ALSA direct test"
    fi
else
    warn "No MAC provided; skipping ALSA direct test"
fi
echo

echo "Step 7: Test JACK Routing"
echo "=========================="
if [ -n "$MAC" ] && command -v jack_lsp >/dev/null 2>&1; then
    echo "Running: jack-route-select bluetooth $MAC"
    if /usr/local/lib/jack-bridge/jack-route-select bluetooth "$MAC" 2>&1; then
        pass "jack-route-select succeeded"
        sleep 1
        echo
        echo "JACK connections to bt_out:"
        jack_lsp -c 2>/dev/null | grep -A 5 "bt_out:" | sed 's/^/  /' || echo "  (none)"
    else
        fail "jack-route-select failed"
        echo "    Check: /tmp/jack-route-select.log"
    fi
else
    warn "No MAC or jack_lsp not available; skipping JACK routing test"
fi
echo

echo "Step 8: Check Logs for Errors"
echo "==============================="
if [ -f /var/log/bluealsad.log ]; then
    echo "Last 10 lines of bluealsad.log:"
    tail -10 /var/log/bluealsad.log | sed 's/^/  /'
else
    warn "bluealsad log not found"
fi
echo

if [ -f /tmp/jack-route-select.log ]; then
    echo "Last 10 lines of jack-route-select.log:"
    tail -10 /tmp/jack-route-select.log | sed 's/^/  /'
fi
echo

echo "=== Test Complete ==="
echo
echo "For GUI testing:"
echo "  1. Launch: mxeq"
echo "  2. Expand BLUETOOTH panel"
echo "  3. Click Scan → Select device → Pair → Trust → Connect"
echo "  4. Click 'Set as Output'"
echo "  5. Verify Devices panel shows 'Bluetooth' selected"
echo "  6. Play audio and verify sound through Bluetooth device"
echo

exit 0
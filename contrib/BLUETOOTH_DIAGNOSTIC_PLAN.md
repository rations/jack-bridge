# Bluetooth Audio Diagnostic and Resolution Plan

## Problem Summary

Bluetooth devices (headphones, speakers) pair and connect but immediately disconnect. No audio output. The debug log shows:
```
Connected → Bonded → ServicesResolved → Paired → [immediately] ServicesResolved:false → Connected:false → Device Removed
```

This pattern indicates **A2DP profile negotiation failure** - bluealsad cannot establish audio transport.

## Root Cause Analysis

Based on the diagnostics:

### ✅ What's Working
- bluetoothd is running and functional
- Bluetooth adapter is discoverable/pairable
- Devices can pair (authentication succeeds)
- Prebuilt bluealsad has all profiles: a2dp-sink, a2dp-source, hfp-hf, hsp-hs
- Game controllers work (they use HID profile, not audio)

### ❌ What's Failing
- A2DP audio transport establishment
- Device immediately unpairs after connection
- No audio stream created

### 🔍 Likely Root Causes (in order of probability)

1. **bluealsad not registering A2DP MediaEndpoint with BlueZ**
   - If bluealsad fails to register org.bluez.MediaEndpoint1 interfaces on D-Bus, BlueZ cannot negotiate A2DP profiles
   - This would cause immediate disconnect after device attempts A2DP connection

2. **Codec mismatch** 
   - Your bluealsad only supports SBC codec
   - If device requires AAC or aptX and bluealsad rejects, connection fails
   - However, SBC is mandatory so devices should fall back to it

3. **D-Bus policy/permission issues**
   - bluealsad might lack permission to register MediaEndpoint1
   - Check `/usr/share/dbus-1/system.d/org.bluealsa.conf`

4. **Daemon startup race condition**
   - bluealsad might start before bluetoothd is fully ready
   - Check init script dependencies and timing

## Diagnostic Commands

Execute these in order to identify the issue:

### Step 1: Verify D-Bus Service Registration
```bash
# Check if org.bluealsa is owned on system bus
gdbus call --system --dest org.freedesktop.DBus \
  --object-path /org/freedesktop/DBus \
  --method org.freedesktop.DBus.GetNameOwner org.bluealsa

# Expected: ('org.bluealsa',) or (':1.XX',)
# If error: bluealsad not running or cannot claim name
```

### Step 2: Check MediaEndpoint Registration
```bash
# List all MediaEndpoint1 interfaces registered with BlueZ
gdbus introspect --system --dest org.bluez --object-path / --recurse 2>/dev/null | \
  grep -B5 "interface org.bluez.MediaEndpoint1"

# Expected: Multiple /org/bluealsa/... paths with MediaEndpoint1
# If none: bluealsad failed to register endpoints (ROOT CAUSE)
```

### Step 3: Check bluealsad Logs
```bash
# View startup logs
sudo tail -100 /var/log/bluealsad.log

# Look for:
# - "Registered endpoint" (good)
# - "Failed to register" (bad)
# - D-Bus permission errors
# - Profile initialization errors
```

### Step 4: Check D-Bus Policy
```bash
# Verify D-Bus policy allows bluealsa user to own org.bluealsa
cat /usr/share/dbus-1/system.d/org.bluealsa.conf

# Must contain:
#   <policy user="bluealsa">
#     <allow own_prefix="org.bluealsa"/>
#   </policy>
```

### Step 5: Manual bluealsad Test
```bash
# Stop init service
sudo service bluealsad stop

# Run manually in foreground with debug
sudo -u bluealsa /usr/local/bin/bluealsad -p a2dp-sink -p a2dp-source --syslog

# Watch for registration messages
# Ctrl+C to stop
```

### Step 6: Check BlueZ Adapter State During Connection
```bash
# In one terminal: monitor D-Bus
gdbus monitor --system --dest org.bluez &

# In another: attempt device connection via bluetoothctl
# Watch for SetConfiguration calls to MediaEndpoint1
```

## Expected Diagnostic Results

Based on the symptoms, you will likely find:

**Most Probable: MediaEndpoint Registration Failure**
- Step 2 shows no MediaEndpoint1 interfaces under /org/bluealsa/
- Step 3 logs show "Failed to register endpoint" errors
- Step 5 manual run shows D-Bus permission denied or registration failures

**If This is the Issue:** bluealsad binary may have been built incorrectly or is incompatible with your BlueZ version.

## Resolution Strategy

### If MediaEndpoint Registration Fails: Rebuild BlueALSA

Your prebuilt binaries appear to have an issue. Here's how to rebuild correctly:

#### Prerequisites
```bash
sudo apt install -y \
  build-essential autoconf automake libtool pkg-config git \
  libasound2-dev libbluetooth-dev libglib2.0-dev libdbus-1-dev \
  libsbc-dev libspandsp-dev libreadline-dev
```

#### Build Steps
```bash
# 1. Get clean BlueALSA source (version 4.3.1 is stable for non-systemd)
cd ~
git clone https://github.com/arkq/bluez-alsa.git
cd bluez-alsa
git checkout v4.3.1  # or use latest stable tag

# 2. Generate configure
autoreconf --install --force

# 3. Configure for sysvinit (NO systemd)
mkdir build && cd build
../configure \
  --prefix=/usr/local \
  --with-bluealsaduser=bluealsa \
  --enable-rfcomm \
  --disable-systemd \
  --enable-a2dpconf \
  --enable-cli

# 4. Build
make -j$(nproc)

# 5. Check build
./src/bluealsad --help
# Should show profiles and codecs

# 6. Test run (don't install yet)
sudo ./src/bluealsad -p a2dp-sink -p a2dp-source --syslog
# Ctrl+C after seeing "Registered endpoint" messages

# 7. Install to system
sudo make install

# 8. Restart daemon
sudo service bluealsad restart

# 9. Verify registration
gdbus introspect --system --dest org.bluez --object-path / --recurse | \
  grep -A2 "MediaEndpoint1"
```

### If D-Bus Policy Issues: Fix Permissions

Update `/usr/share/dbus-1/system.d/org.bluealsa.conf`:
```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="bluealsa">
    <allow own_prefix="org.bluealsa"/>
    <allow send_destination="org.bluealsa"/>
  </policy>
  
  <policy user="root">
    <allow own_prefix="org.bluealsa"/>
    <allow send_destination="org.bluealsa"/>
  </policy>

  <policy group="audio">
    <allow send_destination="org.bluealsa"/>
  </policy>
</busconfig>
```

Reload D-Bus:
```bash
sudo service dbus reload
sudo service bluealsad restart
```

### If Init Script Timing Issues: Fix Dependencies

Update `contrib/init.d/bluealsad` Required-Start:
```bash
### BEGIN INIT INFO
# Required-Start:    $local_fs $remote_fs dbus bluetooth
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO
```

Add delay in start():
```bash
start() {
    # Wait for bluetoothd
    sleep 3
    
    # Wait for adapter
    for i in $(seq 1 10); do
        if bluetoothctl show >/dev/null 2>&1; then
            break
        fi
        sleep 1
    done
    
    # Then start bluealsad
    ...
}
```

## Testing Plan

After implementing fixes:

### 1. Verify Daemon Startup
```bash
sudo service bluealsad restart
sudo tail -50 /var/log/bluealsad.log

# Should see:
# - No errors
# - "Registered endpoint" for each profile
```

### 2. Verify D-Bus Registration
```bash
gdbus introspect --system --dest org.bluez --object-path / --recurse | \
  grep MediaEndpoint1 | wc -l

# Should show 2+ endpoints (source + sink)
```

### 3. Test Device Connection
```bash
# Connect Bluetooth speaker/headphones
bluetoothctl
> power on
> scan on
> pair XX:XX:XX:XX:XX:XX
> trust XX:XX:XX:XX:XX:XX  
> connect XX:XX:XX:XX:XX:XX

# Device should stay connected
```

### 4. Test ALSA Playback
```bash
# Test direct ALSA playback
aplay -D jackbridge_bluealsa:DEV=XX:XX:XX:XX:XX:XX,PROFILE=a2dp \
  /usr/share/sounds/alsa/Front_Center.wav

# Should play audio through Bluetooth device
```

### 5. Test JACK Integration
```bash
# Ensure JACK is running
jack_lsp

# Route to Bluetooth via GUI or:
/usr/local/lib/jack-bridge/jack-route-select bluetooth XX:XX:XX:XX:XX:XX

# Verify bt_out client appears
jack_lsp | grep bt_out

# Test audio
jack_connect mxeq:out_1 bt_out:playback_1
jack_connect mxeq:out_2 bt_out:playback_2
```

## Success Criteria

✅ Device connects and stays connected  
✅ `aplay -D jackbridge_bluealsa:...` produces audio  
✅ GUI device switching to Bluetooth works  
✅ `jack_lsp` shows bt_out client  
✅ Audio plays through Bluetooth device from any ALSA/JACK app

## Alternative: Use Distro Package

If rebuild fails, consider using distro's bluez-alsa-utils:

```bash
# Remove custom binaries
sudo rm -f /usr/local/bin/bluealsad /usr/local/bin/bluealsactl

# The distro daemon is /usr/bin/bluealsa
# Your init script already checks for it

# Create /etc/default/bluealsad with distro paths:
DAEMON=/usr/bin/bluealsa
BLUEALSAD_USER=bluealsa  
BLUEALSAD_ARGS="-p a2dp-sink -p a2dp-source"

# Restart
sudo service bluealsad restart
```

**However**: Distro package may include systemd dependencies. Only use if rebuild fails.

## Next Steps

1. **Execute diagnostics** (Steps 1-6 above) to confirm root cause
2. **Share diagnostic output** so we can pinpoint the exact issue
3. **Implement appropriate fix** (rebuild, D-Bus policy, or init script)
4. **Test systematically** following the testing plan
5. **Update installer** to prevent this for future users

## Questions to Answer

Before proceeding with rebuild:

1. What do Steps 1-2 show? (D-Bus registration status)
2. What errors appear in `/var/log/bluealsad.log`?
3. When running bluealsad manually (Step 5), do you see "Registered endpoint" messages?
4. Do you have build tools installed? (`gcc --version`)

Please run the diagnostic commands and share the output so we can determine the precise fix needed.
# Bluetooth Persistent Ports - Root Cause and Fix

## Root Cause Identified

**Problem:** BlueALSA drops the A2DP transport immediately when no audio is streaming, even though the BlueZ device stays connected.

**Evidence:**
```
alsa_out -d bluealsa → "PCM not found" 
bluealsad.log → "Freeing transport: A2DP Source"
bluetoothctl → "Connected: yes" (but no A2DP transport)
```

**Why USB/HDMI work but Bluetooth doesn't:**
- USB/HDMI: Hardware always present → `alsa_out` creates ports immediately
- Bluetooth: A2DP transport drops when idle → no PCM available → no ports

## The Fix

Add `--keep-alive=-1` to bluealsad daemon to maintain A2DP transport indefinitely.

From BlueALSA documentation:
> `--keep-alive=SEC` - Keep transport active for SEC seconds after playback stops
> `--keep-alive=-1` - Keep transport active indefinitely (always-on)

This is designed for exactly this use case: persistent connections for headless setups.

## Changes Required

### 1. Update `contrib/init.d/bluealsad`

Add `--keep-alive=-1` to the daemon startup:

```bash
# OLD:
start-stop-daemon --start --background --make-pidfile --pidfile "$PIDFILE" \
  --exec /usr/bin/bluealsa -- -p a2dp-source -p a2dp-sink

# NEW:
start-stop-daemon --start --background --make-pidfile --pidfile "$PIDFILE" \
  --exec /usr/bin/bluealsa -- --keep-alive=-1 -p a2dp-source -p a2dp-sink
```

### 2. Current init script is already correct

The `contrib/init.d/jack-bridge-ports` script already uses `bluealsa-aplay --pcm=jack` which will:
- Monitor BlueALSA for A2DP transports
- Create `bluealsa:playback_1/2` ports when transport is active
- With `--keep-alive=-1`, transport stays active → ports appear immediately

### 3. No other changes needed

- Init script path fix already applied (uses `/usr/local/bin/bluealsa-aplay`)
- `pcm.bluealsa` definition already added to `/etc/asound.conf`
- GUI code is correct
- jack-route-select is correct

## Expected Behavior After Fix

1. **Boot sequence:**
   - bluealsad starts with `--keep-alive=-1`
   - Device auto-connects (if paired/trusted)
   - A2DP transport stays active (doesn't drop)
   - bluealsa-aplay sees transport → creates JACK ports
   - `bluealsa:playback_1` and `bluealsa:playback_2` appear immediately

2. **Port visibility:**
   ```bash
   jack_lsp | grep bluealsa
   bluealsa:playback_1
   bluealsa:playback_2
   ```

3. **Routing works:**
   ```bash
   jack-route-select bluetooth MAC
   # Routes all sources to bluealsa:playback_1/2
   ```

## Why This Wasn't Working Before

Without `--keep-alive`, BlueALSA follows this cycle:
1. Device connects at BlueZ level (shows "Connected: yes")
2. A2DP transport created
3. No audio flowing → transport freed after timeout
4. `alsa_out -d bluealsa` → PCM not found
5. `bluealsa-aplay` → no transport → no ports created

With `--keep-alive=-1`:
1. Device connects at BlueZ level
2. A2DP transport created
3. Transport stays active indefinitely
4. PCM always available → ports always visible

## Implementation

User needs to:
1. Switch to Code mode
2. Update `contrib/init.d/bluealsad` to add `--keep-alive=-1`
3. Run: `sudo ./contrib/install.sh`
4. Run: `sudo service bluealsad restart`
5. Run: `sudo service jack-bridge-ports restart`
6. Verify: `jack_lsp | grep bluealsa` shows ports immediately
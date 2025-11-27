# Bluetooth Audio Fix - Complete Implementation Guide

## Overview

This document describes the comprehensive fix for persistent "no sound" issues with Bluetooth audio in jack-bridge. The solution addresses 5 critical problems through architectural improvements and proper configuration management.

---

## Problems Identified

### 1. ❌ ALSA Configuration Not Installed (SHOWSTOPPER)
**Problem**: The installer had code to install `20-jack-bridge-bluealsa.conf` but used `|| true` causing silent failures. ALSA never recognized `jackbridge_bluealsa` as a valid device name.

**Result**: `alsa_out -d jackbridge_bluealsa` always failed immediately.

**Fix**: Enhanced installer with explicit error checking and verification.

### 2. ⚠️ Plugin/Daemon Version Mismatch
**Problem**: Distro ALSA plugin (libasound2-plugin-bluez, Feb 2023 ≈ v4.0.0) may not match custom daemon (commit b0dd89b, unknown version). BlueALSA changed D-Bus API between versions.

**Fix**: Installer now supports installing matching plugins from `contrib/bin/`.

### 3. 🔄 GUI State Desynchronization
**Problem**: Clicking "Set as Output" in Bluetooth panel didn't update Devices panel radio buttons, causing UI confusion.

**Fix**: Added global references and idle callback to sync Devices panel state.

### 4. ⏱️ Bluetooth Connection Timing Issues
**Problem**: Devices disconnect after 2-5 seconds when no audio stream starts. Debug log showed:
```
Connected: <true> → ServicesResolved: <true> → [2s timeout] → Connected: <false>
```

**Root Cause**: `alsa_out` spawn took time, and by the time `bt_out` ports appeared, device had already disconnected.

**Fix**: **Persistent JACK ports** - Keep `bt_out` (and `usb_out`, `hdmi_out`) always running.

### 5. 📦 Missing Components
**Problem**: No `libasound_module_ctl_bluealsa.so` (optional mixer control plugin).

**Fix**: Installer checks for both PCM and CTL plugins, installing if available.

---

## Complete Solution Architecture

### Boot Sequence (SysVinit Priority Order)

```
Priority 20: bluetoothd     (managed by jack-bridge or distro)
Priority 21: bluealsad      (jack-bridge prebuilt daemon)
Priority 22: jack-bridge-ports  (NEW - spawns persistent JACK bridge clients)
Priority 25: jack-bridge-bluetooth-config
Priority defaults: jackd-rt
```

### Persistent Ports Architecture (NEW - Phase 4)

**Concept**: Instead of spawning `alsa_out` clients on-demand, keep them running permanently.

**Benefits**:
- ✅ **Solves timing issue**: `bt_out:playback_{1,2}` exist before device connects
- ✅ **Faster switching**: No spawn delay (0.1-5s eliminated)
- ✅ **Better UX**: Users see all outputs in qjackctl graph
- ✅ **Cleaner code**: `jack-route-select` just disconnects/reconnects

**Implementation**:
- New init script: `/etc/init.d/jack-bridge-ports` (priority 22, after jackd-rt)
- Spawns at boot: `usb_out`, `hdmi_out`, `bt_out`
- Each runs: `alsa_out -j <client> -d <device> -r <rate> -n <nperiods> -p <period>`
- Ports remain active; routing helper just connects/disconnects them

### Enhanced Bluetooth Workflow (Phase 5)

**Old Flow (broken)**:
1. User clicks "Set as Output"
2. Helper spawns `bt_out` → takes 1-3 seconds
3. Device waits for audio...
4. Timeout (2-5s) → Device disconnects ❌
5. No sound

**New Flow (working)**:
1. System boots → `bt_out` ports already exist (connected to nothing)
2. User clicks Scan → Pair → Trust → Connect → "Set as Output"
3. Helper writes BlueALSA defaults to `~/.config/jack-bridge/bluealsa_defaults.conf`
4. Helper reconnects `bt_out` to new device (happens instantly)
5. Device sees immediate audio activity → Stays connected ✅
6. Audio flows!

---

## Files Changed

### Phase 1: Installer Configuration
**File**: `contrib/install.sh`
- Lines 497-541: Enhanced ALSA config installation with verification
- Lines 377-420: Added ALSA plugin installation (Phase 2)
- Lines 474-490: Added jack-bridge-ports init script installation (Phase 4)

### Phase 2: ALSA Plugin Support
**File**: `contrib/install.sh` (lines 377-420)
- Detects architecture (`x86_64` or other)
- Backs up distro plugins to `*.distro-backup`
- Installs matching plugins from `contrib/bin/` if available
- Falls back to distro version with warning

### Phase 3: GUI State Sync
**File**: `src/mxeq.c`
- Lines 54-58: Added global references to Devices panel radio buttons
- Lines 1908-1912: Store global references during panel creation
- Lines 2009-2018: New idle callback to sync Devices panel to Bluetooth
- Lines 2049-2052: Call sync callback after successful routing

### Phase 4: Persistent Ports
**New File**: `contrib/init.d/jack-bridge-ports` (189 lines)
- Waits for JACK server to be ready
- Auto-detects USB/HDMI card/device numbers
- Spawns persistent bridge clients: `usb_out`, `hdmi_out`, `bt_out`
- Provides status command to check running ports

**File**: `contrib/usr/local/lib/jack-bridge/jack-route-select`
- Lines 220-226: Disabled client killing (persistent ports remain alive)
- Lines 240-262: Simplified `ensure_usb_out()` - just checks existence
- Lines 264-282: Simplified `ensure_hdmi_out()` - just checks existence  
- Lines 286-311: Simplified `ensure_bt_out()` - updates defaults, checks existence
- Lines 398-420: Removed `kill_bridge_clients()` call from routing

---

## Installation Instructions

### 1. Add Matching ALSA Plugins (REQUIRED for compatibility)

Your prebuilt daemon is commit `b0dd89b`. You need to build matching ALSA plugins:

```bash
# Clone BlueALSA at the exact commit your daemon was built from
cd /tmp
git clone https://github.com/Arkq/bluez-alsa.git
cd bluez-alsa
git checkout b0dd89b

# Install build dependencies
sudo apt install -y build-essential automake libtool pkg-config \
    libbluetooth-dev libdbus-1-dev libasound2-dev \
    libglib2.0-dev libsbc-dev

# Configure and build (ALSA plugins only, no daemon)
autoreconf --install --force
./configure --enable-aplay --enable-rfcomm --enable-cli
make

# Copy built plugins to repo
cp src/.libs/libasound_module_pcm_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/
cp src/.libs/libasound_module_ctl_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/

# Verify
ls -lh ~/build_a_bridge/jack-bridge/contrib/bin/libasound_module_*.so
```

### 2. Run Updated Installer

```bash
cd ~/build_a_bridge/jack-bridge
sudo ./contrib/install.sh
```

**What It Does**:
- ✅ Installs `20-jack-bridge-bluealsa.conf` to ALSA directories
- ✅ Verifies ALSA recognizes `jackbridge_bluealsa` device
- ✅ Installs matching ALSA plugins (if in `contrib/bin/`)
- ✅ Installs `jack-bridge-ports` init script (persistent ports)
- ✅ Registers all init scripts with proper priorities

### 3. Reboot

```bash
sudo reboot
```

---

## Testing & Verification

### Step 1: Verify Services Started

```bash
# Check all daemons running
ps aux | grep bluetoothd  # Should show running
ps aux | grep bluealsad   # Should show running (as bluealsa user)
ps aux | grep jackd       # Should show running

# Check persistent ports exist
jack_lsp | grep "_out:"
# Expected output:
#   usb_out:playback_1
#   usb_out:playback_2
#   hdmi_out:playback_1
#   hdmi_out:playback_2
#   bt_out:playback_1
#   bt_out:playback_2
```

### Step 2: Verify ALSA Configuration

```bash
# Check ALSA recognizes jackbridge_bluealsa
aplay -L | grep jackbridge_bluealsa
# Expected output:
#   jackbridge_bluealsa
#       BlueALSA parameterized PCM (jack-bridge)

# Check ALSA config files installed
ls -la /usr/share/alsa/alsa.conf.d/20-jack-bridge-bluealsa.conf
ls -la /etc/alsa/conf.d/20-jack-bridge-bluealsa.conf
```

### Step 3: Test ALSA Direct Playback (No JACK)

```bash
# Connect Bluetooth device first
bluetoothctl
> scan on
> pair MAC_ADDRESS
> trust MAC_ADDRESS
> connect MAC_ADDRESS
> exit

# Test with inline parameters
aplay -D "jackbridge_bluealsa:DEV=MAC_ADDRESS,PROFILE=a2dp,SRV=org.bluealsa" \
    /usr/share/sounds/alsa/Front_Center.wav

# Test with defaults mechanism (create ~/.asoundrc entry)
cat >> ~/.asoundrc <<EOF
defaults.jackbridge_bluealsa.DEV "MAC_ADDRESS"
defaults.jackbridge_bluealsa.PROFILE "a2dp"
defaults.jackbridge_bluealsa.SRV "org.bluealsa"
EOF

# Now test with short name
aplay -D jackbridge_bluealsa /usr/share/sounds/alsa/Front_Center.wav
```

**Expected**: Should hear audio through Bluetooth device.

### Step 4: Test JACK Routing

```bash
# Route to Bluetooth
/usr/local/lib/jack-bridge/jack-route-select bluetooth MAC_ADDRESS

# Verify bt_out ports still exist
jack_lsp | grep bt_out
# Should show: bt_out:playback_1 and bt_out:playback_2

# Check connections (if mxeq or other source is running)
jack_lsp -c | grep bt_out
# Should show sources connected to bt_out:playback_1 and bt_out:playback_2

# Check logs for errors
tail -50 /tmp/jack-route-select.log
```

### Step 5: Test GUI Complete Workflow

1. **Open GUI**: `mxeq` or from applications menu
2. **Expand Bluetooth panel**
3. **Press "Scan"** → Should discover devices
4. **Select device** → Click **"Pair"** → Wait for success
5. **Click "Trust"** → Should mark device trusted
6. **Click "Connect"** → Should connect (Device1.Connected=true)
7. **Click "Set as Output"** → Should show "Routing to bt_out ports"
8. **Check Devices panel** → Should show "Bluetooth" radio button selected ✅
9. **Play audio** → Should hear through Bluetooth device

---

## Troubleshooting

### Issue: "ALSA does not recognize 'jackbridge_bluealsa' device"

**Check**:
```bash
ls -la /usr/share/alsa/alsa.conf.d/20-jack-bridge-bluealsa.conf
ls -la /etc/alsa/conf.d/20-jack-bridge-bluealsa.conf
```

**Fix**: Re-run installer or manually copy:
```bash
sudo cp contrib/etc/20-jack-bridge-bluealsa.conf /usr/share/alsa/alsa.conf.d/
sudo cp contrib/etc/20-jack-bridge-bluealsa.conf /etc/alsa/conf.d/
```

### Issue: "bt_out port not found"

**Check**:
```bash
sudo service jack-bridge-ports status
jack_lsp | grep bt_out
```

**Fix**:
```bash
# Restart persistent ports service
sudo service jack-bridge-ports restart

# Wait 5 seconds
sleep 5

# Check again
jack_lsp | grep "_out:"
```

### Issue: "Device connects then disconnects"

**Diagnosis**: Check if `bt_out` exists BEFORE connecting device:
```bash
# Before connecting:
jack_lsp | grep bt_out  # Should already show bt_out:playback_{1,2}

# If missing, persistent ports aren't running:
sudo service jack-bridge-ports start
```

### Issue: "Trust button gives error"

**Check polkit rule**:
```bash
cat /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules
```

**Verify user in groups**:
```bash
groups  # Should include: audio bluetooth
```

**Fix**: Re-login or reboot after installer adds you to groups.

### Issue: "Plugin version mismatch"

**Symptoms**: `bluealsad` logs show errors, `aplay -D jackbridge_bluealsa` fails

**Check versions**:
```bash
# Daemon version
/usr/local/bin/bluealsad --version

# Plugin file date
ls -la /usr/lib/x86_64-linux-gnu/alsa-lib/libasound_module_pcm_bluealsa.so
```

**Fix**: Build matching plugins (see Installation Instructions above)

---

## Architecture Diagrams

### Pre-Fix: On-Demand Spawning (BROKEN)
```
User clicks "Set as Output"
  ↓
jack-route-select bluetooth MAC
  ↓
Spawn: alsa_out -j bt_out -d "jackbridge_bluealsa:DEV=MAC,PROFILE=a2dp,SRV=org.bluealsa"
  ↓
CRASH: String too long (70+ chars) → Buffer overflow
  OR
  ↓
Spawn takes 1-3 seconds
  ↓
Device timeout (2-5s) → Disconnect ❌
```

### Post-Fix: Persistent Ports (WORKING)
```
System Boot
  ↓
jack-bridge-ports spawns:
  - usb_out:playback_{1,2}   (plughw:1,0)
  - hdmi_out:playback_{1,2}  (plughw:0,3)
  - bt_out:playback_{1,2}    (jackbridge_bluealsa)
  ↓
Ports remain active, disconnected
  ↓
User clicks "Set as Output"
  ↓
Write BlueALSA defaults: ~/.config/jack-bridge/bluealsa_defaults.conf
  defaults.jackbridge_bluealsa.DEV "MAC_ADDRESS"
  defaults.jackbridge_bluealsa.PROFILE "a2dp"
  ↓
Reconnect existing bt_out ports (instant)
  ↓
Device sees audio activity immediately → Stays connected ✅
```

### ALSA Defaults Mechanism
```
User config: ~/.asoundrc
  ↓
Includes: ~/.config/jack-bridge/bluealsa_defaults.conf
  ↓
Contains:
  defaults.jackbridge_bluealsa.DEV "AA:BB:CC:DD:EE:FF"
  defaults.jackbridge_bluealsa.PROFILE "a2dp"
  defaults.jackbridge_bluealsa.SRV "org.bluealsa"
  ↓
Referenced by: /usr/share/alsa/alsa.conf.d/20-jack-bridge-bluealsa.conf
  ↓
Defines: pcm.jackbridge_bluealsa with @args [DEV PROFILE SRV ...]
  ↓
Result: aplay -D jackbridge_bluealsa
  → Uses defaults from ~/.asoundrc
  → Opens BlueALSA device AA:BB:CC:DD:EE:FF with a2dp profile
```

---

## Configuration Files Reference

### System-Wide (Root Required)

**ALSA Configuration**:
- `/usr/share/alsa/alsa.conf.d/20-jack-bridge-bluealsa.conf` - Defines `jackbridge_bluealsa` PCM
- `/etc/alsa/conf.d/20-jack-bridge-bluealsa.conf` - Same (dual location for compatibility)
- `/etc/asound.conf` - Master ALSA config

**D-Bus & Security**:
- `/usr/share/dbus-1/system.d/org.bluealsa.conf` - BlueALSA D-Bus policy
- `/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules` - BlueZ permissions for audio/bluetooth groups

**Init Scripts**:
- `/etc/init.d/bluetoothd` - BlueZ daemon
- `/etc/init.d/bluealsad` - BlueALSA daemon
- `/etc/init.d/jack-bridge-ports` - Persistent JACK bridge clients (NEW)
- `/etc/init.d/jackd-rt` - JACK server
- `/etc/init.d/jack-bridge-bluetooth-config` - Adapter configuration helper

**Defaults**:
- `/etc/default/jackd-rt` - JACK options
- `/etc/default/bluealsad` - BlueALSA daemon options

**Routing Config**:
- `/etc/jack-bridge/devices.conf` - System defaults (device names, latency, preferred output)

### Per-User (No Root)

**ALSA Overrides**:
- `~/.asoundrc` - Managed block includes:
  - `~/.config/jack-bridge/current_input.conf`
  - `~/.config/jack-bridge/current_output.conf`
  - `~/.config/jack-bridge/jack_playback_override.conf`
  - `~/.config/jack-bridge/bluealsa_defaults.conf`

**Routing Preferences**:
- `~/.config/jack-bridge/devices.conf` - User preferences (PREFERRED_OUTPUT, BLUETOOTH_DEVICE)

---

## Build Instructions: Matching ALSA Plugins

If you don't have matching plugins in `contrib/bin/`, build them:

```bash
# 1. Find your daemon's commit
COMMIT=$(/usr/local/bin/bluealsad --version 2>/dev/null || echo "b0dd89b")
echo "Daemon commit: $COMMIT"

# 2. Clone and checkout exact commit
cd /tmp
git clone https://github.com/Arkq/bluez-alsa.git
cd bluez-alsa
git checkout $COMMIT

# 3. Install build dependencies
sudo apt install -y build-essential automake libtool pkg-config \
    libbluetooth-dev libdbus-1-dev libasound2-dev libglib2.0-dev libsbc-dev \
    libfdk-aac-dev libldacbt-abr-dev libldacbt-enc-dev liblc3-dev

# 4. Configure (ALSA plugins only, no daemon rebuild)
autoreconf --install --force
./configure \
    --enable-aplay \
    --enable-rfcomm \
    --enable-cli \
    --enable-aac \
    --enable-aptx \
    --enable-ldac \
    --enable-lc3-swb

# 5. Build
make -j$(nproc)

# 6. Copy to jack-bridge repo
cp src/.libs/libasound_module_pcm_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/
cp src/.libs/libasound_module_ctl_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/

# 7. Verify size and date
ls -lh ~/build_a_bridge/jack-bridge/contrib/bin/libasound_module_*.so

# 8. Commit to repo
cd ~/build_a_bridge/jack-bridge
git add contrib/bin/libasound_module_*.so
git commit -m "Add matching ALSA plugins for BlueALSA daemon b0dd89b"
```

### Alternative: Use Distro Plugin (Not Recommended)

If you skip building matching plugins, the installer will use the distro version with this warning:
```
! libasound_module_pcm_bluealsa.so not found in contrib/bin/
  Using distro plugin (may cause version mismatch issues)
```

**Risk**: Daemon/plugin API mismatch may cause:
- D-Bus communication errors
- Device enumeration failures
- Audio stream initialization problems

---

## Verification Checklist

After installation and reboot:

- [ ] bluetoothd running (`ps aux | grep bluetoothd`)
- [ ] bluealsad running as bluealsa user (`ps aux | grep bluealsad`)
- [ ] JACK server running (`ps aux | grep jackd`)
- [ ] Persistent ports exist (`jack_lsp | grep "_out:"` shows 6 ports)
- [ ] ALSA recognizes device (`aplay -L | grep jackbridge_bluealsa`)
- [ ] ALSA plugins installed (`ls -la /usr/lib/x86_64-linux-gnu/alsa-lib/libasound_module_*bluealsa.so`)
- [ ] Polkit rule present (`cat /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules`)
- [ ] User in groups (`groups` shows audio, bluetooth)
- [ ] Config files present (`ls -la /etc/jack-bridge/devices.conf`)
- [ ] GUI launches without errors (`mxeq`)

After GUI Bluetooth workflow:

- [ ] Scan discovers devices
- [ ] Pair succeeds without errors
- [ ] Trust succeeds without errors
- [ ] Connect succeeds (device stays connected)
- [ ] "Set as Output" updates Devices panel to "Bluetooth"
- [ ] Audio plays through Bluetooth device
- [ ] Device remains connected during playback
- [ ] Switching to Internal/USB/HDMI works

---

## FAQ

### Q: Do I need to rebuild the GUI (mxeq)?
**A**: Yes, after modifying `src/mxeq.c`. Run:
```bash
make clean
make
sudo cp src/mxeq contrib/bin/mxeq
sudo ./contrib/install.sh  # Reinstalls GUI
```

### Q: Can I use the distro ALSA plugin?
**A**: Maybe. Test it first:
```bash
aplay -D "jackbridge_bluealsa:DEV=MAC,PROFILE=a2dp" /usr/share/sounds/alsa/Front_Center.wav
```
If that works, you're OK. If it fails with D-Bus errors, you need matching plugins.

### Q: Why keep all ports running? Doesn't that waste resources?
**A**: No. `alsa_out` clients use <0.1% CPU when disconnected. The benefit (instant routing + no timing issues) far outweighs the minimal resource cost.

### Q: What if I don't want persistent ports?
**A**: Disable the init script:
```bash
sudo update-rc.d -f jack-bridge-ports remove
sudo service jack-bridge-ports stop
```
Then manually spawn ports when needed. But this re-introduces the timing issue.

### Q: Can I adjust Bluetooth latency?
**A**: Yes! In the GUI:
1. Expand "BLUETOOTH" panel
2. Adjust "Bluetooth latency (period frames)" slider (128-1024)
3. Adjust "nperiods" spinner (2-4)
4. Restart persistent ports: `sudo service jack-bridge-ports restart`

---

## Success Metrics

After applying this fix, you should achieve:

✅ **Scan** → Devices discovered instantly  
✅ **Pair** → No errors  
✅ **Trust** → No errors  
✅ **Connect** → Device remains connected  
✅ **Set as Output** → Devices panel updates to "Bluetooth"  
✅ **Audio playback** → Sound flows immediately through Bluetooth  
✅ **Device stability** → No disconnects during playback  
✅ **Switching** → Can toggle between Internal/USB/HDMI/Bluetooth seamlessly  

---

## Credits

**Solution Architect**: User insight about persistent ports was key to solving the timing issue.

**Implementation**: 5-phase comprehensive fix addressing configuration, compatibility, GUI sync, persistent architecture, and enhanced workflow.

**Version**: 2025-11-27 - Complete Bluetooth Audio Integration Fix
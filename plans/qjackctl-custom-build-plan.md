# Custom qjackctl Build Plan for jack-bridge

**Status:** Ready for Implementation  
**Priority:** Critical  
**Strategy:** Compile custom qjackctl binary with SYSTEM bus (like BlueALSA approach)

---

## Problem Summary

**Root Cause:** qjackctl connects to SESSION D-Bus, but jack-bridge-dbus service runs on SYSTEM D-Bus.

**Evidence:**
```cpp
// qjackctl-1.0.4/src/qjackctlMainForm.cpp:2425
QDBusConnection dbusc = QDBusConnection::sessionBus();  // ← PROBLEM!
```

**Our Service:**
```c
// src/jack_bridge_dbus.c:515  
g_bus_own_name(G_BUS_TYPE_SYSTEM, ...)  // ← SYSTEM bus
```

**Result:** They never connect. qjackctl spawns its own `jackdbus` process.

---

## Solution: Custom qjackctl Binary

Just like BlueALSA, we will:
1. Modify qjackctl source to use SYSTEM bus instead of SESSION bus
2. Build custom qjackctl binary
3. Install to `contrib/bin/qjackctl`
4. Update installer to use our custom binary

---

## Required Source Changes

### Change 1: JACK D-Bus Control Connection (CRITICAL)

**File:** `qjackctl-1.0.4/src/qjackctlMainForm.cpp`

**Line 2425 - Original Code:**
```cpp
QDBusConnection dbusc = QDBusConnection::sessionBus();  // ← SESSION
```

**Line 2425 - Modified Code:**
```cpp
QDBusConnection dbusc = QDBusConnection::systemBus();  // ← SYSTEM
```

**That's it!** Only ONE line needs changing.

**Note:** Line 846 already uses `systemBus()` for qjackctl's own D-Bus service, so it's correct.

---

## Build Script Implementation

**File:** `build-qjackctl.sh`

```bash
#!/bin/bash
# build-qjackctl.sh
# Build custom qjackctl with SYSTEM D-Bus support for jack-bridge
set -e

echo "Building custom qjackctl for jack-bridge..."

# Check for required packages
REQUIRED_PKGS="cmake g++ qtbase5-dev qttools5-dev-tools libjack-jackd2-dev libasound2-dev"
echo "Checking build dependencies..."
for pkg in $REQUIRED_PKGS; do
    if ! dpkg -l | grep -q "^ii  $pkg "; then
        echo "ERROR: Missing package: $pkg"
        echo "Install with: sudo apt install $REQUIRED_PKGS"
        exit 1
    fi
done

# Navigate to qjackctl source
cd qjackctl-1.0.4 || {
    echo "ERROR: qjackctl-1.0.4 directory not found!"
    exit 1
}

# Create patch marker to avoid re-patching
PATCH_MARKER=".jack-bridge-patched"
if [ ! -f "$PATCH_MARKER" ]; then
    echo "Applying jack-bridge SYSTEM bus patch..."
    
    # Patch line 2425: sessionBus() → systemBus()
    sed -i '2425s/QDBusConnection::sessionBus()/QDBusConnection::systemBus()/' \
        src/qjackctlMainForm.cpp
    
    # Verify patch applied
    if grep -n "QDBusConnection::systemBus()" src/qjackctlMainForm.cpp | grep -q "^2425:"; then
        echo "  ✓ Patch applied successfully (line 2425)"
        touch "$PATCH_MARKER"
    else
        echo "  ✗ Patch failed! Manual intervention required."
        exit 1
    fi
else
    echo "Source already patched (found $PATCH_MARKER)"
fi

# Clean previous build
echo "Cleaning previous build..."
rm -rf build

# Configure with CMake
echo "Configuring qjackctl..."
cmake -B build \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DCONFIG_DBUS=ON \
    -DCONFIG_JACK_MIDI=ON \
    -DCONFIG_ALSA_SEQ=ON \
    -DCONFIG_JACK_PORT_ALIASES=ON \
    -DCONFIG_JACK_METADATA=ON \
    -DCONFIG_JACK_SESSION=ON \
    -DCONFIG_SYSTEM_TRAY=ON

# Build
echo "Building qjackctl (this may take a few minutes)..."
cmake --build build --parallel $(nproc)

# Install binary to contrib/bin
echo "Installing to contrib/bin..."
mkdir -p ../contrib/bin
cp build/src/qjackctl ../contrib/bin/qjackctl
chmod 755 ../contrib/bin/qjackctl

echo ""
echo "✓ Custom qjackctl built successfully!"
echo "Binary: contrib/bin/qjackctl"
echo "Size: $(du -h ../contrib/bin/qjackctl | cut -f1)"
echo ""
echo "Run 'sudo ./contrib/install.sh' to install jack-bridge with custom qjackctl"
```

---

## Installer Integration Changes

**File:** `contrib/install.sh`

### Change 1: Remove qjackctl from Package List

**Line 25 - Original:**
```bash
REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse qjackctl ..."
```

**Line 25 - Modified:**
```bash
# Note: qjackctl removed (we provide custom build in contrib/bin)
REQUIRED_PACKAGES="jackd2 alsa-utils libasound2-plugins apulse libasound2-plugin-equal ..."
```

### Change 2: Install Custom qjackctl Binary

**Add after line 137 (after jack-watchdog removal):**
```bash
# Install custom qjackctl binary (SYSTEM bus integration)
if [ -f "contrib/bin/qjackctl" ]; then
    echo "Installing custom qjackctl (jack-bridge SYSTEM bus integration)..."
    install -m 0755 contrib/bin/qjackctl /usr/local/bin/qjackctl
    echo "  ✓ Installed custom qjackctl to /usr/local/bin/qjackctl"
    echo "  ✓ This binary connects to SYSTEM D-Bus (not SESSION bus)"
else
    echo "WARNING: Custom qjackctl binary not found at contrib/bin/qjackctl"
    echo "         Run './build-qjackctl.sh' to build it from qjackctl-1.0.4/ source"
    echo "         Installing distro qjackctl as fallback..."
    apt install -y qjackctl || true
    echo "  ! Distro qjackctl uses SESSION bus (will not integrate with jack-bridge)"
fi
```

### Change 3: Remove qjackctl Autostart

**Lines 249-264 - DELETE ENTIRE SECTION:**
```bash
# Install qjackctl autostart entry ...
# ... DELETE THIS ENTIRE BLOCK ...
```

**Replace with:**
```bash
# NOTE: qjackctl autostart removed
# Users launch qjackctl manually when they want graph/patchbay visualization
# JACK server is already running via jackd-rt init service
```

---

## Additional Cleanup Tasks

### 1. Remove jack-watchdog References

**Search terms:** `jack-watchdog`, `WATCHDOG_PIDFILE`, `watchdog_pid`

**Files to update:**

#### A. contrib/install.sh
Remove lines 134-137:
```bash
if [ -f "contrib/usr/lib/jack-bridge/jack-watchdog" ]; then
    install -m 0755 contrib/usr/lib/jack-bridge/jack-watchdog "${USR_LIB_DIR}/jack-watchdog"
    echo "Installed jack-watchdog to ${USR_LIB_DIR}/jack-watchdog"
fi
```

#### B. contrib/init.d/jackd-rt
Remove lines:
- Line 35: `: ${WATCHDOG_PIDFILE:=/var/run/jack-watchdog.pid}`
- Lines 169-173: Watchdog startup code
- Lines 218-227: Watchdog shutdown code

#### C. README.md
Search for "watchdog" and remove/update references

### 2. Fix BOM Corruption

**File:** `contrib/default/jackd-rt`

**Problem:** Line 1 has UTF-8 BOM (`﻿`) causing parse errors

**Fix:**
```bash
# Remove BOM bytes
sed -i '1s/^\xEF\xBB\xBF//' contrib/default/jackd-rt
```

### 3. Fix jack-bridge-ports Shutdown Errors

**File:** `contrib/init.d/jack-bridge-ports`

**Problem:** During shutdown, init script tries to work with JACK ports after JACK is stopped

**Solution:** Add JACK runtime check at start of `stop()` function:

```bash
stop() {
    log_daemon_msg "Stopping jack-bridge persistent ports" "jack-bridge-ports"
    
    # Check if JACK is still running (don't fail if already stopped)
    if ! pgrep -x jackd >/dev/null 2>&1; then
        log_progress_msg "JACK not running, nothing to stop"
        log_end_msg 0
        return 0
    fi
    
    # ... rest of existing stop() code ...
}
```

---

## Testing & Verification

### After Build

```bash
# Check binary size (should be ~2-3 MB)
ls -lh contrib/bin/qjackctl

# Verify patch applied
strings contrib/bin/qjackctl | grep -c systemBus
# Should show multiple hits (our patch successful)

strings contrib/bin/qjackctl | grep -c "org.jackaudio.service"
# Should show hits (D-Bus service name compiled in)
```

### After Installation

```bash
# 1. Verify custom binary installed
which qjackctl
# Output: /usr/local/bin/qjackctl (not /usr/bin/qjackctl)

# 2. Verify D-Bus service available
dbus-send --system --print-reply \
    --dest=org.jackaudio.service \
    /org/jackaudio/Controller \
    org.jackaudio.JackControl.IsStarted
# Should return: boolean true

# 3. Launch qjackctl
qjackctl &

# 4. Check status display
# Should show: "D-Bus JACK" or "Started" 
# Should NOT show: "(default) active"

# 5. Verify no duplicate processes
ps aux | grep -E "(jackd|jackdbus)" | grep -v grep
# Should show ONLY: /usr/bin/jackd -R -P70 ...
# Should NOT show: /usr/bin/jackdbus auto

# 6. Test Stop button
# Click Stop in qjackctl → check logs:
tail -f /var/log/jackd-rt.log
# Should see service stop message

# 7. Test Start button
# Click Start in qjackctl → check logs:
tail -f /var/log/jackd-rt.log
# Should see service start message
```

---

## Success Criteria

After implementation, all these must work:

- [ ] Only ONE line changed in qjackctl source (professional minimal patch)
- [ ] Custom qjackctl binary built successfully
- [ ] Custom binary installed to `/usr/local/bin/qjackctl`
- [ ] qjackctl shows "D-Bus JACK" mode (not "active")
- [ ] qjackctl Start button controls system service
- [ ] qjackctl Stop button controls system service
- [ ] qjackctl Settings dialog reads/writes `/etc/default/jackd-rt`
- [ ] Only ONE jackd process running
- [ ] No `jackdbus auto` spawned
- [ ] jack-watchdog completely removed from repo
- [ ] BOM corruption fixed in defaults file
- [ ] No "failed to spawn" errors during shutdown
- [ ] Clean reboot (no init script errors)
- [ ] Graph/Patchbay work unchanged
- [ ] mxeq GUI still functional

---

## Implementation Workflow

### Step 1: Create build script
```bash
./build-qjackctl.sh
```

### Step 2: Remove jack-watchdog references
Search entire repo and remove all references

### Step 3: Fix BOM and shutdown issues
Clean up configuration files and init scripts

### Step 4: Update installer
Modify contrib/install.sh to use custom binary

### Step 5: Test on real hardware
```bash
sudo ./contrib/install.sh
sudo reboot
# Test qjackctl integration
```

---

## Risk Mitigation

**Risk:** Build fails on Devuan 5

**Mitigation:** Build script checks for required packages first, exits gracefully with clear error message

**Risk:** Patch doesn't apply cleanly

**Mitigation:** Use specific line number with sed, verify before marking as patched

**Risk:** Custom binary breaks other qjackctl features

**Mitigation:** Only one line changed (minimal invasive patch), all other features unchanged

**Risk:** PATH priority issues

**Mitigation:** `/usr/local/bin` naturally takes precedence over `/usr/bin` in default Linux PATH

---

## Next Steps

Proceed to Code mode to implement all changes:

1. Create `build-qjackctl.sh` script
2. Apply one-line patch to qjackctl source
3. Build custom binary
4. Remove jack-watchdog references throughout repo
5. Fix BOM corruption
6. Fix shutdown errors
7. Update installer
8. Prepare for real hardware testing

**Ready for implementation?**

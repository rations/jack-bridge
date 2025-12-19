# Custom qjackctl Build Plan for jack-bridge

**Status:** Implemented    
**Strategy:** Compile custom qjackctl binary with SYSTEM bus (like BlueALSA approach)

---

## Problem Summary

**Root Cause:** qjackctl connects to SESSION D-Bus, but jack-bridge-dbus service runs on SYSTEM D-Bus.

**Evidence:**
```cpp
// qjackctl-1.0.4/src/qjackctlMainForm.cpp:2425
QDBusConnection dbusc = QDBusConnection::sessionBus();  // ← PROBLEM!
```

**This Service:**
```c
// src/jack_bridge_dbus.c:515  
g_bus_own_name(G_BUS_TYPE_SYSTEM, ...)  // ← SYSTEM bus
```

**Result:** They never connect. qjackctl spawns its own `jackdbus` process.

---

## Solution: Custom qjackctl Binary

Just like BlueALSA, 
1. Modify qjackctl source to use SYSTEM bus instead of SESSION bus
2. Build custom qjackctl binary
3. Install to `contrib/bin/qjackctl`
4. Update installer to use our custom binary

---

## Required Source Changes

### Change 1: JACK D-Bus Control Connection 

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

## Installer Integration Changes

**File:** `contrib/install.sh`

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

## Success Criteria

After implementation, all these must work:

- [ ] Only ONE line changed in qjackctl source (professional minimal patch)
- [ ] Custom qjackctl binary built successfully
- [ ] Custom binary installed to `/usr/local/bin/qjackctl`
- [ ] qjackctl shows "Started" mode (not "active")
- [ ] qjackctl Start button controls system service
- [ ] qjackctl Stop button controls system service
- [ ] qjackctl Settings dialog reads/writes `/etc/default/jackd-rt`
- [ ] Only ONE jackd process running
- [ ] No `jackdbus auto` spawned
- [ ] Graph/Patchbay work unchanged
- [ ] mxeq GUI still functional
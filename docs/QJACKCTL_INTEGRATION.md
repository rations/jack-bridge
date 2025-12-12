# qjackctl Integration with jack-bridge

## Overview

jack-bridge now provides full integration with qjackctl (JACK Audio Connection Kit Qt GUI Interface) through a D-Bus bridge service. This allows users to control JACK using qjackctl's familiar interface while maintaining jack-bridge's robust SysV init architecture.

## What's Implemented

### Phase A: MVP (Minimum Viable Product) ✅

**qjackctl Start/Stop Control:**
- ✅ Start button → calls `service jackd-rt start`
- ✅ Stop button → calls `service jackd-rt stop`
- ✅ Status detection (qjackctl shows correct JACK state)
- ✅ Real-time signals (ServerStarted/ServerStopped)
- ✅ Password-less control for users in `audio` group

**Components:**
- `src/jack_bridge_dbus.c` - Main D-Bus service (pure C with GLib)
- `contrib/dbus/org.jackaudio.service.conf` - D-Bus policy
- `contrib/dbus/org.jackaudio.service.service` - Service activation
- `contrib/polkit/50-jack-bridge.rules` - Password-less authorization
- `contrib/init.d/jack-bridge-dbus` - SysV init script

### Phase B: Settings Integration (Planned)

**qjackctl Settings Dialog:**
- 🔨 View/modify sample rate via GUI
- 🔨 View/modify frames/period (buffer size)
- 🔨 View/modify periods/buffer
- 🔨 View/modify realtime priority
- 🔨 View/modify MIDI driver
- 🔨 Device selection (respects auto-detection)
- 🔨 Settings persistence in `/etc/default/jackd-rt`

### Phase C: Live Updates (Planned)

**Advanced Features:**
- 🔨 Live buffer size changes (via `jack_set_buffer_size()`)
- 🔨 Bidirectional settings sync
- 🔨 Comprehensive error handling

## Architecture

### Before Integration
```
qjackctl → spawns jackd as subprocess ✗ CONFLICT
jack-bridge → SysV init manages jackd as service
```

### After Integration
```
┌──────────────┐
│   qjackctl   │ (unchanged)
└──────┬───────┘
       │ D-Bus calls
       │ org.jackaudio.service
       ▼
┌──────────────────────────┐
│ jack-bridge-dbus service │ (NEW - our bridge)
│ - JackControl interface  │
│ - JackConfigure (Phase B)│
└──────┬───────────────────┘
       │
       ├──► /etc/default/jackd-rt (settings)
       │
       └──► service jackd-rt start/stop
```

## Installation

The installer (`contrib/install.sh`) automatically:

1. **Builds the D-Bus service:**
   ```bash
   make dbus  # Compiles src/jack_bridge_dbus.c
   ```

2. **Installs components:**
   - Binary: `/usr/local/bin/jack-bridge-dbus`
   - D-Bus policy: `/usr/share/dbus-1/system.d/org.jackaudio.service.conf`
   - Service activation: `/usr/share/dbus-1/system-services/org.jackaudio.service.service`
   - Polkit rules: `/etc/polkit-1/rules.d/50-jack-bridge.rules`
   - Init script: `/etc/init.d/jack-bridge-dbus`

3. **Registers service:**
   ```bash
   update-rc.d jack-bridge-dbus defaults 01 99
   service dbus reload
   ```

## Usage

### For End Users

1. **Install jack-bridge:**
   ```bash
   sudo ./contrib/install.sh
   ```

2. **Reboot** (or manually start services):
   ```bash
   sudo service jack-bridge-dbus start
   sudo service jackd-rt start
   ```

3. **Launch qjackctl:**
   - qjackctl will automatically connect to the D-Bus service
   - Start/Stop buttons control the system JACK service
   - Status updates in real-time

### For Developers

**Build D-Bus service only:**
```bash
make dbus
```

**Test D-Bus service manually:**
```bash
# Start service
sudo /usr/local/bin/jack-bridge-dbus

# Test with dbus-send
dbus-send --system --print-reply \
  --dest=org.jackaudio.service \
  /org/jackaudio/Controller \
  org.jackaudio.JackControl.IsStarted

# Start JACK via D-Bus
dbus-send --system --print-reply \
  --dest=org.jackaudio.service \
  /org/jackaudio/Controller \
  org.jackaudio.JackControl.StartServer
```

**Monitor D-Bus signals:**
```bash
dbus-monitor --system "type='signal',sender='org.jackaudio.service'"
```

## Security Model

### Authorization

**Polkit Rules** (`/etc/polkit-1/rules.d/50-jack-bridge.rules`):
- Users in `audio` group get password-less JACK control
- No root password required for Start/Stop
- Secure: Only controls JACK service, not system-wide

**D-Bus Policy** (`/usr/share/dbus-1/system.d/org.jackaudio.service.conf`):
- Root owns the service
- `audio` group can call methods
- Anyone can receive signals

### Why It's Safe

1. **Scoped permissions:** Only controls `jackd-rt` service
2. **Group-based:** Must be in `audio` group (standard for audio users)
3. **Standard Linux:** Uses polkit (same system desktop environments use)
4. **No privilege escalation:** Can't modify system files or run arbitrary commands

## Compatibility

### What's Preserved

✅ All existing jack-bridge functionality works unchanged:
- mxeq GUI (mixer, EQ, Bluetooth, recording)
- Device auto-detection
- USB/HDMI/Bluetooth routing
- Watchdog and connection manager
- SysV init scripts

✅ Both GUIs work simultaneously:
- mxeq controls routing/devices
- qjackctl controls Start/Stop and settings
- No conflicts

### qjackctl Graph

The qjackctl Graph (Patchbay) works perfectly with jack-bridge:
- Visualize all JACK connections
- Make manual connections
- Create/activate patchbays
- **No changes needed** - Graph uses JACK client API directly

## Troubleshooting

### qjackctl says "D-Bus JACK"

This is **correct**! qjackctl detected our D-Bus service and is using it.

### Start button doesn't work

Check D-Bus service is running:
```bash
sudo service jack-bridge-dbus status
```

If not running, start it:
```bash
sudo service jack-bridge-dbus start
```

### "Permission denied" when clicking Start/Stop

Verify you're in the `audio` group:
```bash
groups $USER
```

If not, add yourself:
```bash
sudo usermod -aG audio $USER
```

Then log out and back in.

### D-Bus service won't start

Check logs:
```bash
journalctl -xe | grep jack-bridge-dbus
```

Or run manually to see errors:
```bash
sudo /usr/local/bin/jack-bridge-dbus
```

Common issues:
- GLib/GIO not installed: `sudo apt install libglib2.0-dev`
- D-Bus not running: `sudo service dbus start`

## Implementation Details

### D-Bus Interface

**Service:** `org.jackaudio.service`  
**Object Path:** `/org/jackaudio/Controller`  
**Interface:** `org.jackaudio.JackControl`

**Methods:**
- `IsStarted() → boolean` - Check if JACK is running
- `StartServer()` - Start JACK service
- `StopServer()` - Stop JACK service
- `SwitchMaster()` - No-op (compatibility)

**Signals:**
- `ServerStarted()` - Emitted when JACK starts
- `ServerStopped()` - Emitted when JACK stops

### State Monitoring

The D-Bus service polls `/var/run/jackd-rt.pid` every 1 second to detect state changes. This is:
- **Low overhead** (simple file check)
- **Reliable** (works with any init system)
- **Real-time** (1s latency is imperceptible)

Alternative approaches (inotify, etc.) add complexity without benefit.

### Process Flow

**Start JACK (via qjackctl):**
1. User clicks Start in qjackctl
2. qjackctl calls `org.jackaudio.JackControl.StartServer()` via D-Bus
3. jack-bridge-dbus executes `service jackd-rt start`
4. SysV init script starts jackd with correct settings
5. jack-bridge-dbus detects JACK running (pidfile)
6. jack-bridge-dbus emits `ServerStarted()` signal
7. qjackctl receives signal and updates UI

**Stop JACK (via qjackctl):**
1. User clicks Stop in qjackctl
2. qjackctl calls `org.jackaudio.JackControl.StopServer()` via D-Bus
3. jack-bridge-dbus executes `service jackd-rt stop`
4. SysV init script stops jackd cleanly
5. jack-bridge-dbus detects JACK stopped (pidfile gone)
6. jack-bridge-dbus emits `ServerStopped()` signal
7. qjackctl receives signal and updates UI

## Next Steps

### Phase B: Settings Integration

Will implement:
- `JackConfigure` D-Bus interface
- Settings translation (D-Bus ↔ `/etc/default/jackd-rt`)
- qjackctl Setup dialog functionality
- Parameter validation
- Settings persistence

### Phase C: Live Updates

Will implement:
- Live buffer size changes (no restart)
- Bidirectional settings sync
- Advanced error handling
- User documentation

## References

- **jack-bridge Project:** https://github.com/your-repo
- **qjackctl:** https://qjackctl.sourceforge.io/
- **JACK Audio:** https://jackaudio.org/
- **D-Bus Specification:** https://dbus.freedesktop.org/doc/dbus-specification.html
- **Polkit Manual:** https://www.freedesktop.org/software/polkit/docs/latest/
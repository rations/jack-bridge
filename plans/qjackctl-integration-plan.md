# qjackctl Integration Plan for jack-bridge

**Status:** Design Phase  
**Priority:** High  
**Complexity:** High - Requires D-Bus service, polkit rules, and bidirectional settings sync  
**Risk:** Medium - Must not break existing jack-bridge functionality  

---

## Executive Summary

Integrate qjackctl's Start/Stop and Settings features with jack-bridge's SysV init architecture by implementing a D-Bus bridge service that translates qjackctl's process-control model into init service commands.

**Key Principle:** Users can use EITHER qjackctl OR mxeq GUI interchangeably to control the same underlying JACK service.

---

## Architecture Analysis

### Current Incompatibility

```
qjackctl Expectation:          jack-bridge Reality:
┌─────────────────┐            ┌──────────────────┐
│ qjackctl spawns │            │ SysV init starts │
│ jackd process   │ ✗ CONFLICT │ jackd at boot    │
│ as subprocess   │            │ as service       │
└─────────────────┘            └──────────────────┘
```

### Integration Solution: D-Bus Bridge

```
┌──────────────┐
│   qjackctl   │ (unchanged Qt app)
└──────┬───────┘
       │ D-Bus method calls
       │ org.jackaudio.service
       ▼
┌──────────────────────────────┐
│  jack-bridge-dbus service    │ (NEW - our bridge)
│  - Implements JackControl    │
│  - Implements JackConfigure  │
└──────┬───────────────────────┘
       │
       ├──► /etc/default/jackd-rt (settings)
       │
       └──► service jackd-rt restart (process control)
```

---

## Detailed Implementation Plan

### Phase 1: D-Bus Service Foundation (Core Bridge)

**File:** `src/jack_bridge_dbus.c`  
**Service Name:** `org.jackaudio.service`  
**Object Path:** `/org/jackaudio/Controller`  
**Interfaces:** `org.jackaudio.JackControl`, `org.jackaudio.Configure`

#### 1.1 D-Bus Interface Definition

**Required Methods (qjackctl compatibility):**

**JackControl Interface:**
- `IsStarted() → boolean` - Check if jackd-rt service is running
- `StartServer() → void` - Call `service jackd-rt start`
- `StopServer() → void` - Call `service jackd-rt stop`
- `SwitchMaster() → void` - Not applicable (no-op for init service)

**JackConfigure Interface:**
- `SetParameterValue(path: string array, value: variant) → void`
- `GetParameterValue(path: string array) → (is_set: bool, default: variant, value: variant)`
- `ResetParameterValue(path: string array) → void`
- `GetParameterConstraint(path: string array) → constraint_data`

**Signals:**
- `ServerStarted()` - Emitted when jackd-rt starts
- `ServerStopped()` - Emitted when jackd-rt stops

#### 1.2 Settings Translation Map

| qjackctl D-Bus Path | /etc/default/jackd-rt Variable | Type | Default |
|---------------------|--------------------------------|------|---------|
| `["engine", "driver"]` | N/A (always "alsa") | string | "alsa" |
| `["engine", "realtime"]` | Derived from `-R` flag | bool | true |
| `["engine", "realtime-priority"]` | `JACKD_PRIORITY` | uint | 70 |
| `["engine", "port-max"]` | N/A (use jackd default) | uint | 256 |
| `["engine", "sync"]` | N/A | bool | false |
| `["driver", "device"]` | `JACKD_DEVICE` | string | auto-detected |
| `["driver", "rate"]` | `JACKD_SR` | uint | 48000 |
| `["driver", "period"]` | `JACKD_PERIOD` | uint | 256 |
| `["driver", "nperiods"]` | `JACKD_NPERIODS` | uint | 3 |
| `["driver", "midi-driver"]` | `JACKD_MIDI` | string | "seq" |

#### 1.3 Implementation Strategy

**File Structure:**
```
src/jack_bridge_dbus.c          - Main D-Bus service
src/jack_bridge_dbus_control.c  - JackControl interface impl
src/jack_bridge_dbus_config.c   - JackConfigure interface impl
src/jack_bridge_settings_sync.c - Settings file I/O
contrib/dbus/org.jackaudio.service.conf  - D-Bus policy
contrib/dbus/org.jackaudio.service.xml   - Interface definition
contrib/polkit/50-jack-bridge.rules      - Polkit authorization
```

**Dependencies:**
- `libdbus-1-dev` or `libglib2.0-dev` (use GDBus from GLib for easier implementation)
- No Qt dependencies (pure C with GLib)

**Service Lifecycle:**
```c
// Pseudo-code structure
int main() {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    
    // Connect to system bus
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    
    // Register JackControl interface
    g_dbus_connection_register_object(bus, "/org/jackaudio/Controller",
        introspection_data, &control_vtable, NULL, NULL, &error);
    
    // Register JackConfigure interface  
    g_dbus_connection_register_object(bus, "/org/jackaudio/Controller",
        introspection_data, &configure_vtable, NULL, NULL, &error);
    
    // Request bus name
    g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.jackaudio.service",
        G_BUS_NAME_OWNER_FLAGS_REPLACE, ...);
    
    // Monitor jackd-rt service state (poll or inotify pidfile)
    g_timeout_add_seconds(1, monitor_jackd_state, NULL);
    
    g_main_loop_run(loop);
    return 0;
}
```

---

### Phase 2: Polkit Authorization

**File:** `contrib/polkit/50-jack-bridge.rules`

Allow users in `audio` group to:
1. Control jackd-rt service (start/stop/restart)
2. Modify `/etc/default/jackd-rt`
3. Read system configuration

```javascript
/* Allow audio group to control JACK service */
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.systemd1.manage-units" &&
        action.lookup("unit") == "jackd-rt.service" &&
        subject.isInGroup("audio")) {
        return polkit.Result.YES;
    }
});

/* Allow audio group to modify JACK configuration */
polkit.addRule(function(action, subject) {
    if (action.id == "org.jackaudio.configure" &&
        subject.isInGroup("audio")) {
        return polkit.Result.YES;
    }
});
```

**Note:** For SysV (not systemd), we'll need to implement our own polkit action in the D-Bus service.

---

### Phase 3: Settings Synchronization

#### 3.1 Configuration File Handling

**Challenge:** qjackctl stores settings in Qt QSettings format, jack-bridge uses shell variables.

**Solution:** D-Bus service maintains `/etc/default/jackd-rt` as source of truth and syncs to qjackctl config on startup.

**Sync Strategy:**

1. **On qjackctl startup:**
   - qjackctl reads from `~/.config/rncbc.org/QjackCtl.conf`
   - If D-Bus service available, query current settings via `GetParameterValue`
   - Sync any mismatches to UI

2. **On settings change:**
   - qjackctl calls `SetParameterValue` via D-Bus
   - D-Bus service writes to `/etc/default/jackd-rt`
   - Service prompts for restart if JACK is running

3. **Atomic file updates:**
   ```c
   int write_jackd_rt_config(const char *key, const char *value) {
       char tmp[] = "/etc/default/jackd-rt.tmp.XXXXXX";
       int fd = mkstemp(tmp);
       
       // Read existing file, replace key=value, write to temp
       // ...
       
       // Atomic rename
       rename(tmp, "/etc/default/jackd-rt");
   }
   ```

#### 3.2 Variable Mapping Logic

```c
typedef struct {
    const char *dbus_path[3];     // ["engine", "realtime-priority"]
    const char *shell_var;         // "JACKD_PRIORITY"
    enum { TYPE_INT, TYPE_BOOL, TYPE_STRING } type;
    const char *default_value;
} ParamMapping;

static const ParamMapping PARAM_MAP[] = {
    { {"driver", "rate", NULL}, "JACKD_SR", TYPE_INT, "48000" },
    { {"driver", "period", NULL}, "JACKD_PERIOD", TYPE_INT, "256" },
    { {"driver", "nperiods", NULL}, "JACKD_NPERIODS", TYPE_INT, "3" },
    { {"engine", "realtime-priority", NULL}, "JACKD_PRIORITY", TYPE_INT, "70" },
    { {"driver", "midi-driver", NULL}, "JACKD_MIDI", TYPE_STRING, "seq" },
    { {"driver", "device", NULL}, "JACKD_DEVICE", TYPE_STRING, "" },
    // ... more mappings
};
```

---

### Phase 4: Init Script Modifications

**File:** `contrib/init.d/jackd-rt` (enhanced)

#### 4.1 Add D-Bus Triggered Restart Support

```bash
# New function: reload settings without full restart if possible
reload() {
    log_daemon_msg "Reloading JACK configuration" "jackd-rt"
    
    # Re-source defaults
    if [ -r /etc/default/jackd-rt ]; then
        . /etc/default/jackd-rt
    fi
    
    # If only buffer size changed, use jack_bufsize instead of full restart
    # (requires jack_bufsize utility or D-Bus service to call jack_set_buffer_size)
    
    # For other settings, full restart required
    restart
}

case "$1" in
    start|stop|restart|status)
        # existing...
        ;;
    reload)
        reload
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|reload|status}"
        exit 2
        ;;
esac
```

#### 4.2 Settings Validation

Add validation to reject invalid settings before attempting to start:

```bash
validate_settings() {
    # Check sample rate is valid
    case "$JACKD_SR" in
        22050|44100|48000|88200|96000|176400|192000) ;;
        *) log_warning_msg "Invalid sample rate: $JACKD_SR"; return 1 ;;
    esac
    
    # Check period is power of 2
    # Check nperiods is >= 2
    # etc.
}
```

---

### Phase 5: Audio Buffer Settings Management

**Challenge:** Users need to adjust both buffer size (frames/period) and buffer multiplier (periods/buffer) for their system's needs. These settings directly impact latency and stability.

**Critical Settings:**
- **Frames/Period (`JACKD_PERIOD`)**: Buffer size per processing cycle
  - Lower = lower latency, higher CPU load
  - Higher = higher latency, more stable
  - Can potentially be changed live via `jack_set_buffer_size()`
  
- **Periods/Buffer (`JACKD_NPERIODS`)**: Number of periods in the buffer
  - Typically 2-3 for low latency, 3-4 for stability
  - ALWAYS requires full JACK restart to change
  - Not changeable via live API

#### 5.1 Frames/Period Live Change Handler

```c
void handle_period_change(uint32_t new_period) {
    jack_client_t *client = jack_client_open("jack-bridge-dbus-ctrl",
                                              JackNoStartServer, NULL);
    if (!client) {
        // JACK not running, just write to config file
        write_config_value("JACKD_PERIOD", new_period);
        return;
    }
    
    // Try live buffer size change
    if (jack_set_buffer_size(client, new_period) == 0) {
        // Success! Update config file too
        write_config_value("JACKD_PERIOD", new_period);
        
        // Emit notification
        emit_dbus_signal("BufferSizeChanged", new_period);
    } else {
        // Failed, requires full restart
        write_config_value("JACKD_PERIOD", new_period);
        // Return error to qjackctl suggesting restart
    }
    
    jack_client_close(client);
}
```

#### 5.2 Periods/Buffer Change Handler

```c
void handle_nperiods_change(uint32_t new_nperiods) {
    // Validate range (2-8 typical, 2 minimum)
    if (new_nperiods < 2) {
        return_dbus_error("periods/buffer must be >= 2");
        return;
    }
    if (new_nperiods > 8) {
        g_warning("periods/buffer > 8 may cause issues");
    }
    
    // Write to config file
    write_config_value("JACKD_NPERIODS", new_nperiods);
    
    // Check if JACK is running
    if (is_jack_running()) {
        // Notify user that restart is required
        return_dbus_message("success_restart_required",
            "Periods/buffer changed. JACK restart required for changes to take effect.");
    } else {
        return_dbus_message("success", "Periods/buffer changed.");
    }
}
```

#### 5.3 Combined Latency Calculation

When qjackctl queries latency, calculate from both parameters:

```c
double get_theoretical_latency_ms(void) {
    uint32_t period = read_config_int("JACKD_PERIOD", 256);
    uint32_t nperiods = read_config_int("JACKD_NPERIODS", 3);
    uint32_t sample_rate = read_config_int("JACKD_SR", 48000);
    
    // Total buffer = period * nperiods
    double latency_ms = (period * nperiods * 1000.0) / sample_rate;
    return latency_ms;
}
```

**Example calculations:**
- 256 frames/period × 3 periods @ 48kHz = 16ms latency
- 128 frames/period × 2 periods @ 48kHz = 5.3ms latency
- 512 frames/period × 3 periods @ 48kHz = 32ms latency

---

### Phase 6: Process State Monitoring

D-Bus service must monitor jackd-rt service state to emit `ServerStarted`/`ServerStopped` signals.

#### 6.1 Service State Detection

**Option A: Poll pidfile** (simple, reliable)
```c
gboolean monitor_jackd_state(gpointer user_data) {
    static gboolean was_running = FALSE;
    gboolean is_running = check_pidfile("/var/run/jackd-rt.pid");
    
    if (is_running && !was_running) {
        // JACK started
        emit_dbus_signal("ServerStarted", NULL);
    } else if (!is_running && was_running) {
        // JACK stopped
        emit_dbus_signal("ServerStopped", NULL);
    }
    
    was_running = is_running;
    return TRUE; // Keep polling
}

// Poll every second
g_timeout_add_seconds(1, monitor_jackd_state, NULL);
```

**Option B: Inotify on pidfile** (efficient, complex)
- Watch `/var/run/jackd-rt.pid` for CREATE/DELETE events
- More efficient but requires inotify setup

**Recommendation:** Use Option A (polling) for simplicity and reliability.

#### 6.2 JACK Client Connection Detection

Service should also detect when it can successfully connect as JACK client:

```c
gboolean can_connect_to_jack(void) {
    jack_client_t *client = jack_client_open("jack-bridge-dbus-probe",
                                              JackNoStartServer, NULL);
    if (client) {
        jack_client_close(client);
        return TRUE;
    }
    return FALSE;
}
```

---

### Phase 7: Device Detection Integration

**Critical:** Maintain jack-bridge's auto-detection; never hardcode devices.

#### 7.1 Device Parameter Handling

When qjackctl queries `["driver", "device"]`:

```c
GVariant *get_device_parameter(void) {
    // Read from /etc/default/jackd-rt
    char *device = read_config_value("JACKD_DEVICE");
    
    if (device && strlen(device) > 0) {
        // User has overridden device
        return g_variant_new_string(device);
    }
    
    // Return empty to indicate auto-detection
    // (init script will call detect-alsa-device.sh)
    return g_variant_new_string("");
}
```

When qjackctl sets device:

```c
void set_device_parameter(const char *device) {
    if (!device || strlen(device) == 0) {
        // Clear override - use auto-detection
        write_config_value("JACKD_DEVICE", "");
        add_comment("# JACKD_DEVICE empty = auto-detect");
    } else {
        // User selected specific device
        write_config_value("JACKD_DEVICE", device);
    }
}
```

#### 7.2 Device List Enumeration

When qjackctl requests available devices via `GetParameterConstraint(["driver", "device"])`:

```c
GVariant *get_device_list(void) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("av"));
    
    // Parse aplay -l output (same logic as detect-alsa-device.sh)
    FILE *fp = popen("aplay -l 2>/dev/null", "r");
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // Parse "card N: NAME [DESC]" format
        // Extract hw:N or hw:CARD=NAME
        // Add to variant builder
    }
    pclose(fp);
    
    return g_variant_builder_end(&builder);
}
```

---

### Phase 8: User Detection Integration

**Critical:** Never hardcode usernames - respect jack-bridge's auto-detection.

The D-Bus service runs as root (system bus service), but JACK must run as detected user.

#### 8.1 User Parameter Handling

```c
const char *get_jackd_user(void) {
    // Read JACKD_USER from /etc/default/jackd-rt
    char *user = read_config_value("JACKD_USER");
    
    if (user && strlen(user) > 0) {
        return user; // User override
    }
    
    // Fall back to init script detection logic
    // (call detect-user.sh or replicate inline)
    return detect_user_uid_1000_in_audio_group();
}
```

**No user selection in qjackctl:** qjackctl doesn't have UI for selecting runtime user. The init script auto-detection remains authoritative.

---

### Phase 9: Restart Coordination

When settings change while JACK is running, coordinate restart carefully.

#### 9.1 Restart Decision Logic

```c
typedef struct {
    gboolean needs_restart;
    gboolean can_apply_live;
    char *param_changed;
    char *user_message;  // Description for user
} ChangeImpact;

ChangeImpact assess_parameter_change(const char *param, const char *new_value) {
    ChangeImpact impact = {FALSE, FALSE, NULL, NULL};
    
    if (strcmp(param, "JACKD_PERIOD") == 0) {
        // Frames/Period: Buffer size can sometimes change live
        impact.can_apply_live = TRUE;
        impact.needs_restart = FALSE;
        impact.param_changed = "frames/period";
        impact.user_message = "Buffer size changed. Attempting live update...";
    } else if (strcmp(param, "JACKD_NPERIODS") == 0) {
        // Periods/Buffer: ALWAYS requires restart
        impact.needs_restart = TRUE;
        impact.can_apply_live = FALSE;
        impact.param_changed = "periods/buffer";
        impact.user_message = "Periods/buffer changed. JACK restart required.";
    } else if (strcmp(param, "JACKD_SR") == 0) {
        // Sample rate: requires restart
        impact.needs_restart = TRUE;
        impact.can_apply_live = FALSE;
        impact.param_changed = "sample rate";
        impact.user_message = "Sample rate changed. JACK restart required.";
    } else if (strcmp(param, "JACKD_DEVICE") == 0) {
        // Device: requires restart
        impact.needs_restart = TRUE;
        impact.can_apply_live = FALSE;
        impact.param_changed = "audio device";
        impact.user_message = "Audio device changed. JACK restart required.";
    } else if (strcmp(param, "JACKD_PRIORITY") == 0) {
        // Realtime priority: requires restart
        impact.needs_restart = TRUE;
        impact.can_apply_live = FALSE;
        impact.param_changed = "realtime priority";
        impact.user_message = "Realtime priority changed. JACK restart required.";
    } else if (strcmp(param, "JACKD_MIDI") == 0) {
        // MIDI driver: requires restart
        impact.needs_restart = TRUE;
        impact.can_apply_live = FALSE;
        impact.param_changed = "MIDI driver";
        impact.user_message = "MIDI driver changed. JACK restart required.";
    }
    
    return impact;
}
```

#### 9.2 Settings Change Summary

**Parameters by restart requirement:**

| Parameter | Live Update? | Requires Restart? | Notes |
|-----------|--------------|-------------------|-------|
| `JACKD_PERIOD` (Frames/Period) | Yes (try first) | Maybe | Use `jack_set_buffer_size()`, fallback to restart |
| `JACKD_NPERIODS` (Periods/Buffer) | **NO** | **YES** | ALWAYS requires full restart |
| `JACKD_SR` (Sample Rate) | NO | YES | Cannot change while running |
| `JACKD_DEVICE` (ALSA Device) | NO | YES | Cannot change while running |
| `JACKD_PRIORITY` (RT Priority) | NO | YES | Set at startup only |
| `JACKD_MIDI` (MIDI Driver) | NO | YES | Set at startup only |
| `JACKD_USER` (Runtime User) | NO | YES | Set at startup only |

#### 9.3 User Confirmation via D-Bus

qjackctl will show its own restart prompt. Our D-Bus service should:
1. Accept the `SetParameterValue` call
2. Write to `/etc/default/jackd-rt`
3. Return success (qjackctl handles restart prompting)

---

### Phase 10: Installation Integration

#### 10.1 Build System Changes

**Makefile additions:**
```makefile
# Build jack-bridge-dbus service
DBUS_TARGET = contrib/bin/jack-bridge-dbus
DBUS_SRCS = src/jack_bridge_dbus.c src/jack_bridge_dbus_control.c \
            src/jack_bridge_dbus_config.c src/jack_bridge_settings_sync.c
DBUS_PKGS = glib-2.0 gio-2.0 dbus-1
DBUS_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(DBUS_PKGS))
DBUS_LIBS = $(shell $(PKG_CONFIG) --libs $(DBUS_PKGS))

dbus: $(BIN_DIR) $(DBUS_TARGET)

$(DBUS_TARGET): $(DBUS_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(DBUS_CFLAGS) -o $@ $(DBUS_SRCS) $(DBUS_LIBS)

all: mxeq manager dbus
```

#### 10.2 Installer Updates

**File:** `contrib/install.sh` (additions)

```bash
# Install D-Bus service binary
if [ -f "contrib/bin/jack-bridge-dbus" ]; then
    install -m 0755 contrib/bin/jack-bridge-dbus /usr/local/bin/jack-bridge-dbus
    echo "Installed jack-bridge-dbus service"
fi

# Install D-Bus service file
mkdir -p /usr/share/dbus-1/system-services
cat > /usr/share/dbus-1/system-services/org.jackaudio.service <<'EOF'
[D-BUS Service]
Name=org.jackaudio.service
Exec=/usr/local/bin/jack-bridge-dbus
User=root
SystemdService=jack-bridge-dbus.service
EOF

# Install D-Bus policy
install -m 0644 contrib/dbus/org.jackaudio.service.conf \
    /usr/share/dbus-1/system.d/org.jackaudio.service.conf

# Install polkit rules
install -m 0644 contrib/polkit/50-jack-bridge.rules \
    /etc/polkit-1/rules.d/50-jack-bridge.rules

# Reload D-Bus
if command -v service >/dev/null 2>&1; then
    service dbus reload || true
fi

# Reload polkit
if pidof polkitd >/dev/null 2>&1; then
    kill -HUP $(pidof polkitd | awk '{print $1}') || true
fi
```

#### 10.3 Init Script for D-Bus Service

**File:** `contrib/init.d/jack-bridge-dbus`

```bash
#!/bin/sh
### BEGIN INIT INFO
# Provides:          jack-bridge-dbus
# Required-Start:    $local_fs $remote_fs dbus
# Required-Stop:     $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: D-Bus service for qjackctl integration
### END INIT INFO

DAEMON=/usr/local/bin/jack-bridge-dbus
PIDFILE=/var/run/jack-bridge-dbus.pid

case "$1" in
    start)
        start-stop-daemon --start --background --make-pidfile \
            --pidfile $PIDFILE --exec $DAEMON
        ;;
    stop)
        start-stop-daemon --stop --pidfile $PIDFILE --retry=TERM/5/KILL/2
        ;;
    restart)
        $0 stop
        sleep 1
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 2
        ;;
esac
```

Register with `update-rc.d jack-bridge-dbus defaults 01 99` (start early, stop late).

---

### Phase 11: Configuration File Format

#### 11.1 Enhanced `/etc/default/jackd-rt`

Add structured comments for D-Bus service parsing:

```bash
# /etc/default/jackd-rt - JACK Audio Server Configuration
# This file is managed by jack-bridge D-Bus service and read by init script.
# Changes take effect after: service jackd-rt restart

# Sample Rate (Hz)
# Valid: 22050, 44100, 48000, 88200, 96000, 176400, 192000
# Default: 48000
JACKD_SR=48000

# Buffer Size (frames/period)
# Valid: Power of 2 between 16 and 4096
# Default: 256
# Note: Can be changed live via qjackctl Settings → Apply
JACKD_PERIOD=256

# Number of Periods (buffer multiplier)
# Valid: 2-8
# Default: 3
JACKD_NPERIODS=3

# Realtime Priority
# Valid: 0-89 (0=disabled, 10-89=realtime)
# Default: 70
JACKD_PRIORITY=70

# MIDI Driver
# Valid: "seq" (ALSA sequencer), "raw" (raw MIDI), "none" (disable)
# Default: "seq"
JACKD_MIDI="seq"

# ALSA Device Override
# Empty = auto-detect using detect-alsa-device.sh
# Examples: "hw:0", "hw:CARD=PCH", "hw:CARD=USB"
# Default: (empty = auto-detect)
JACKD_DEVICE=""

# User Override
# Empty = auto-detect first UID>=1000 user in audio group
# Default: (empty = auto-detect)
JACKD_USER=""
```

#### 11.2 Parsing Logic

```c
typedef struct {
    GHashTable *vars;  // key=JACKD_SR, value="48000"
} JackdConfig;

JackdConfig *parse_jackd_rt_file(const char *path) {
    JackdConfig *conf = g_new0(JackdConfig, 1);
    conf->vars = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    
    FILE *f = fopen(path, "r");
    if (!f) return conf;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse KEY=VALUE or KEY="VALUE"
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        // Extract key and value
        // Handle quotes properly
        // Add to hash table
    }
    
    fclose(f);
    return conf;
}
```

---

### Phase 12: Testing Strategy

#### 12.1 Test Cases

**Test 1: Start/Stop via qjackctl**
```
1. Boot system (jackd-rt auto-starts)
2. Launch qjackctl
3. Click Stop → should call D-Bus → service jackd-rt stop
4. Verify JACK stops, mxeq mixer still works
5. Click Start → should call D-Bus → service jackd-rt start
6. Verify JACK starts, graph reconnects
```

**Test 2: Settings Change**
```
1. Open qjackctl → Setup
2. Change Sample Rate: 48000 → 44100
3. Click Apply
4. Verify:
   - /etc/default/jackd-rt updated (JACKD_SR=44100)
   - qjackctl prompts for restart
   - After restart: jackd runs at 44100
```

**Test 3: Frames/Period Live Change**
```
1. JACK running at period=256
2. qjackctl → Setup → Frames/Period → 512 → Apply
3. Verify:
   - Buffer size changes immediately (no restart)
   - Connections maintained
   - /etc/default/jackd-rt updated (JACKD_PERIOD=512)
```

**Test 4: Periods/Buffer Change (Requires Restart)**
```
1. JACK running with nperiods=3 (latency = 16ms @ 48kHz, 256 frames)
2. qjackctl → Setup → Periods/Buffer → 2 → Apply
3. Verify:
   - qjackctl shows "restart required" message
   - /etc/default/jackd-rt updated (JACKD_NPERIODS=2)
   - JACK still running with old value (3)
4. Click "Yes" to restart
5. Verify:
   - JACK restarts with nperiods=2
   - New latency = 10.7ms (256×2 @ 48kHz)
   - All connections restored by jack-autoconnect
```

**Test 5: Device Auto-Detection Preserved**
```
1. Ensure JACKD_DEVICE="" in /etc/default/jackd-rt
2. Restart JACK via qjackctl
3. Verify detect-alsa-device.sh still called
4. Verify USB not hijacked as main device
```

**Test 6: Multi-User Support**
```
1. Install on system with multiple users (UID>=1000)
2. Boot system
3. Verify correct user auto-detected
4. User A launches qjackctl → controls same JACK instance
5. User B cannot start second JACK (already running)
```

#### 12.2 Regression Tests

Verify existing functionality unchanged:

- [ ] mxeq GUI works (mixer, EQ, Bluetooth, recording)
- [ ] Device switching works (internal/USB/HDMI/Bluetooth)
- [ ] jack-route-select routing works
- [ ] Bluetooth on-demand port spawning works
- [ ] USB/HDMI persistent ports work
- [ ] Auto-detection runs on each boot
- [ ] Watchdog restarts JACK on crash
- [ ] SysV init scripts work (start/stop/restart/status)

---

## Implementation Checklist

### Core D-Bus Service

- [ ] Implement `src/jack_bridge_dbus.c` - Main service loop
- [ ] Implement `src/jack_bridge_dbus_control.c` - JackControl interface
- [ ] Implement `src/jack_bridge_dbus_config.c` - JackConfigure interface  
- [ ] Implement `src/jack_bridge_settings_sync.c` - Config file I/O
- [ ] Create `contrib/dbus/org.jackaudio.service.conf` - D-Bus policy
- [ ] Create `contrib/dbus/org.jackaudio.service.xml` - Interface introspection

### Authorization

- [ ] Create `contrib/polkit/50-jack-bridge.rules` - Polkit rules
- [ ] Define custom polkit action `org.jackaudio.configure`
- [ ] Test polkit authorization for users in `audio` group

### Init System Integration

- [ ] Create `contrib/init.d/jack-bridge-dbus` - D-Bus service init script
- [ ] Add `reload` action to `contrib/init.d/jackd-rt`
- [ ] Add settings validation to jackd-rt init script
- [ ] Test service start/stop/restart via D-Bus

### Settings Synchronization

- [ ] Implement bidirectional config sync
- [ ] Handle live buffer size changes via `jack_set_buffer_size()`
- [ ] Preserve device/user auto-detection
- [ ] Add atomic file write for `/etc/default/jackd-rt`

### Build System

- [ ] Add `dbus` target to Makefile
- [ ] Add dependencies check for GLib/GIO/D-Bus
- [ ] Update `contrib/install.sh` to install D-Bus components
- [ ] Update `contrib/uninstall.sh` to remove D-Bus components

### Testing & Validation

- [ ] Test on clean Devuan 5 VM
- [ ] Test with multiple users
- [ ] Test device hot-plug scenarios
- [ ] Verify mxeq and qjackctl can coexist
- [ ] Test settings changes from both GUIs
- [ ] Verify watchdog still works
- [ ] Test graceful shutdown on reboot

### Documentation

- [ ] Create `docs/QJACKCTL_INTEGRATION.md` - User guide
- [ ] Update README.md with qjackctl usage
- [ ] Document D-Bus service architecture
- [ ] Add troubleshooting section

---

## Risk Mitigation

### Risk 1: Breaking Existing Functionality

**Mitigation:**
- D-Bus service is **optional** - install.sh detects if qjackctl is installed
- If D-Bus service not installed, jack-bridge works exactly as before
- All changes to init script are backward-compatible

### Risk 2: Settings Conflict Between GUIs

**Mitigation:**
- `/etc/default/jackd-rt` is single source of truth
- Both GUIs read/write through D-Bus service
- Service enforces atomic updates
- Last-write-wins for conflicts

### Risk 3: Hardcoded Device/User Assumptions

**Mitigation:**
- D-Bus service **never overrides** empty JACKD_DEVICE or JACKD_USER
- Init script detection logic unchanged
- Settings sync preserves auto-detection semantics

### Risk 4: Polkit/D-Bus Not Available

**Mitigation:**
- Installer checks for D-Bus and polkit before installing service
- Provides clear error if requirements missing
- System degrades gracefully (qjackctl graph-only mode still works)

---

## Implementation Phases

### Phase A: Minimum Viable Product (MVP)

**Goal:** Basic start/stop control via qjackctl

**Deliverables:**
1. D-Bus service with IsStarted/StartServer/StopServer
2. Polkit rules for service control
3. Service state monitoring (pidfile polling)
4. Installer integration

**Testing:** qjackctl can start/stop JACK via init service

**Estimated Complexity:** Medium (2-3 days development)

### Phase B: Settings Integration

**Goal:** qjackctl Settings dialog functional

**Deliverables:**
1. JackConfigure interface implementation
2. Settings file parser/writer
3. Parameter mapping layer
4. Settings validation

**Testing:** qjackctl Setup dialog can view/modify jack-bridge settings

**Estimated Complexity:** High (3-5 days development)

### Phase C: Live Updates & Polish

**Goal:** Seamless user experience

**Deliverables:**
1. Live buffer size changes
2. Bidirectional settings sync
3. Comprehensive error handling
4. User documentation

**Testing:** Both GUIs work interchangeably, settings stay synchronized

**Estimated Complexity:** Medium (2-3 days development)

---

## Alternative: Simpler Approach

If D-Bus service proves too complex, consider **extending mxeq** instead:

### Option B: Add Start/Stop to mxeq

**Pros:**
- Much simpler (shell commands from GTK)
- No D-Bus/polkit complexity
- Keeps jack-bridge architecture simple

**Implementation:**
```c
// In mxeq.c
static void on_jack_start_clicked(GtkButton *b, gpointer ud) {
    GError *err = NULL;
    gchar *cmd = "pkexec service jackd-rt start";
    if (!g_spawn_command_line_async(cmd, &err)) {
        show_error_dialog("Failed to start JACK: %s", 
                         err ? err->message : "unknown");
    }
}

static void on_jack_stop_clicked(GtkButton *b, gpointer ud) {
    GError *err = NULL;
    gchar *cmd = "pkexec service jackd-rt stop";
    if (!g_spawn_command_line_async(cmd, &err)) {
        show_error_dialog("Failed to stop JACK: %s",
                         err ? err->message : "unknown");
    }
}
```

Add settings dialog to edit `/etc/default/jackd-rt` (using pkexec for elevation).

**Cons:**
- Users need to learn new GUI (not qjackctl)
- Doesn't leverage qjackctl's mature settings UI

---

## Recommendation

Given your requirements (terminal-free, file-edit-free, works on any system), I recommend:

**Primary Path: Implement D-Bus Bridge Service (Phases A → B → C)**

This provides:
✓ Full qjackctl integration (Start/Stop + Settings)  
✓ No terminal required (users click buttons)  
✓ No file editing (settings via GUI)  
✓ Works on any Devuan/Debian SysV system  
✓ Preserves auto-detection (no hardcoding)  
✓ Both mxeq and qjackctl functional  

**Fallback: If D-Bus proves too complex, extend mxeq with basic controls**

---

## Next Steps

1. **Review this plan** - Are you comfortable with the D-Bus service complexity?
2. **Prioritize phases** - Do you want MVP (start/stop only) first, or full settings integration?
3. **Development approach** - Would you like me to switch to Code mode to start implementing, or do you want to refine the architecture first?

The D-Bus service approach is **definitely feasible** and is the correct architectural solution for your requirements. It's complex but not impossibly so - roughly 7-10 days of careful development and testing.

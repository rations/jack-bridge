# Gaming Bridge Integration Plan

This document outlines the detailed plan for integrating the Gaming bridge into the jack-bridge project. The Gaming bridge will allow users to route audio from Proton/Wine games to the JACK audio system without regressions or tight coupling.

## Overview

The Gaming bridge will be treated as a first-class bridge type, similar to Bluetooth. It will use ALSA loopback (`snd-aloop`) to create a virtual audio device that Proton/Wine can use. The Gaming bridge will be spawned on-demand when the user selects Gaming mode in the GUI.

## Key Components

1. **ALSA Loopback (`snd-aloop`)**: The Gaming bridge relies on ALSA loopback to create a virtual audio device that Proton/Wine can use.
2. **Gaming Bridge Helper Functions**: Functions to manage the lifecycle of the Gaming bridge (`ensure_loopback`, `spawn_gaming`, `stop_gaming`).
3. **ALSA Configuration**: A static ALSA configuration (`pcm.gaming`) to define the Gaming output device.
4. **GUI Integration**: Add a "Gaming" option to the Devices panel in `mxeq.c`.
5. **Routing Integration**: Update `jack-route-select` and `jack_connection_manager.c` to handle the Gaming bridge.

## Implementation Steps

### 1. Add Helper Functions to `jack-bridge-ports`

Add the following helper functions to `contrib/init.d/jack-bridge-ports`:

#### `ensure_loopback`
```sh
ensure_loopback() {
    if ! lsmod | grep -q '^snd_aloop'; then
        modprobe snd-aloop || return 1
        sleep 1
    fi
    return 0
}
```

#### `spawn_gaming`
```sh
spawn_gaming() {
    ensure_loopback || return 1

    if pgrep -f "alsa_in -j gaming_in" >/dev/null 2>&1; then
        return 0
    fi

    su -l "$USERNAME" -c \
      "nohup alsa_in -j gaming_in -d hw:Loopback,1,0 -r 48000 -p 1024 -n 2 >>\"$LOGFILE\" 2>&1 &"
}
```

#### `stop_gaming`
```sh
stop_gaming() {
    pkill -TERM -f "alsa_in -j gaming_in" 2>/dev/null || true
}
```

### 2. Integrate the Gaming Bridge into `jack-bridge-ports`

The Gaming bridge should be spawned on-demand when the user selects Gaming in the GUI. The script should call `ensure_loopback` and `spawn_gaming` when Gaming is selected.

### 3. Add ALSA Configuration for the Gaming Bridge

Add the ALSA configuration for the Gaming bridge (`pcm.gaming`) to `contrib/etc/asound.conf`:

```conf
pcm.gaming_hw {
    type hw
    card "Loopback"
    device 0
    subdevice 0
}

pcm.gaming {
    type dmix
    ipc_key 4096
    ipc_perm 0666
    slave {
        pcm "gaming_hw"
        rate 48000
        channels 2
        period_size 1024
        buffer_size 4096
    }
}
```

### 4. Integrate the Gaming Bridge into the GUI (`mxeq.c`)

Add a "Gaming" option to the Devices panel in `mxeq.c`:

- Add a radio button for "Gaming" in the `create_devices_panel` function.
- Signal the bridge manager (`jack-bridge-ports`) to spawn or teardown the Gaming bridge when the user selects or deselects Gaming mode.

### 5. Update `jack-route-select`

Add the "Gaming" option to the `route` function in `jack-route-select`:

- Add a case for "gaming" in the `route` function.
- Handle spawning the Gaming bridge and routing audio to the Gaming output.

### 6. Update `jack_connection_manager.c`

Add the Gaming bridge ports (e.g., `gaming_in:capture_1/2`) to the list of known sink ports:

- Update the `is_sink_port` function to recognize Gaming bridge ports.
- Ensure audio sources are routed to the Gaming bridge when the user selects Gaming mode.

### 7. Test the Gaming Bridge

- Test the Gaming bridge with Proton/Wine to ensure it works correctly.
- Validate that the Gaming bridge does not introduce regressions in the existing audio paths.

## Summary

This plan outlines the steps to integrate the Gaming bridge into the jack-bridge project. The Gaming bridge will be treated as a first-class bridge type, similar to Bluetooth, and will use ALSA loopback to create a virtual audio device that Proton/Wine can use. The Gaming bridge will be spawned on-demand when the user selects Gaming mode in the GUI.

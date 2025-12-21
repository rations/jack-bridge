# Gaming Bridge Implementation Plan

This document extends **Proton_Gaming_Audio_Architecture.md** and provides **implementation‑level guidance** for integrating the Gaming (Proton/Wine) audio path into the existing project *without regressions, hacks, or tight coupling*.

It is written so an IDE agent can understand *what to change, where, and why*.

---

## 1. Minimal Diff: `jack-bridge-ports`

### Design Goal

- Add Gaming as a **first‑class bridge type**
- Follow the same lifecycle model as Bluetooth
- Avoid touching `jack-rt`
- Avoid always‑on loopback or extra latency

### Core Principles

- Lazy loading only when Gaming is selected
- No failure if `snd-aloop` already exists
- No hardware assumptions

---

### 1.1 New Helper: Ensure ALSA Loopback

Add near existing Bluetooth helpers:

```sh
ensure_loopback() {
    if ! lsmod | grep -q '^snd_aloop'; then
        modprobe snd-aloop || return 1
        sleep 1
    fi
    return 0
}
```

- Safe to call multiple times
- No impact on non-gaming users

---

### 1.2 Spawn Gaming Bridge (on demand)

Add a new function, parallel to Bluetooth spawning:

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

Why `alsa_in`?
- Proton writes **playback** into loopback
- JACK must **capture** from loopback

---

### 1.3 Optional Teardown

For symmetry (not strictly required):

```sh
stop_gaming() {
    pkill -TERM -f "alsa_in -j gaming_in" 2>/dev/null || true
}
```

---

### 1.4 Invocation Model

`jack-bridge-ports` **does not decide when Gaming is active**.

It only exposes:
- `spawn_gaming`
- `stop_gaming`

These are triggered externally (GUI / control layer).

---

## 2. `mxeq.c`: Signaling Bridge Spawn / Teardown

### Design Goal

- Zero tight coupling
- No direct JACK control
- No shell logic embedded in DSP code

`mxeq.c` remains an **orchestrator**, not a daemon manager.

---

### 2.1 Control Mechanism (Recommended)

Reuse the existing project pattern:

- Write a small state file
- Let system services react

Example:

```
/etc/jack-bridge/requested_output
```

Values:

```
usb
hdmi
bluetooth
gaming
```

---

### 2.2 `mxeq.c` Responsibilities

When user selects **Gaming**:

1. Switch ALSA output PCM to `gaming`
2. Write:
   ```
   gaming
   ```
   to `requested_output`

When user switches away:

- Write new output type

No direct process management.

---

### 2.3 Bridge Manager Reaction

A lightweight watcher (existing or trivial):

```sh
case "$REQUESTED_OUTPUT" in
    gaming)
        spawn_gaming
        ;;
    bluetooth)
        spawn_bluetooth
        stop_gaming
        ;;
    *)
        stop_gaming
        ;;
esac
```

This mirrors your Bluetooth model exactly.

---

## 3. GUI Naming and User‑Facing Semantics

### Design Goal

Users should **never need to know**:
- ALSA
- JACK
- Loopback
- Proton internals

---

### 3.1 Output Device Names

Recommended radio button labels:

- USB
- HDMI
- Bluetooth
- Gaming (Wine / Proton)

Optional tooltip text for Gaming:

> Optimized audio path for Steam and Windows games

---

### 3.2 No ALSA Names in UI

Avoid exposing:
- `dmix`
- `Loopback`
- `hw:X,Y`

These remain implementation details.

---

### 3.3 Documentation Hint (Optional)

In advanced settings or docs only:

> Gaming mode uses a compatibility audio bridge for Windows games.

That’s all users need.

---

## 4. Proton / Steam Usage Model

Games are launched with:

```
WINEALSAOUTPUT=gaming %command%
```

Notes:
- No Steam runtime hacks
- No apulse
- No environment pollution

---

## 5. Why This Causes No Regressions

- Default audio path unchanged
- JACK continues owning hardware
- No always‑on loopback latency
- No assumptions about devices
- No distro‑specific behavior

---

## 6. Mental Model for Future Contributors

> “Gaming is just another bridge, like Bluetooth.”

If contributors understand that sentence, they understand the system.

---

## 7. Status

This plan is:
- Minimal
- Portable
- Aligned with JACK architecture
- Safe for long‑term maintenance

It completes the audio stack without compromising its design.


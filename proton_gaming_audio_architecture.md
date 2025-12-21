# Proton / Wine Gaming Audio Integration (ALSA + JACK, no Pulse)

## Project Context

This project implements a **pure ALSA → JACK audio stack** on Devuan (and similar non-systemd distros), with the following strict constraints:

- ❌ No systemd
- ❌ No PulseAudio
- ❌ No PipeWire
- ✅ JACK runs headless at boot (`jack-rt`)
- ✅ ALSA applications are bridged into JACK
- ✅ Dynamic routing is handled entirely at the JACK graph level
- ✅ Hardware abstraction is handled by persistent and on-demand JACK bridge ports

The system already works correctly for:
- Native JACK applications
- ALSA applications
- Browsers and media players
- Wine VSTs inside DAWs
- Native Linux games (outside Steam/Heroic)

The **only remaining failure case** is:

> **Windows games and launchers (Steam / Heroic / Proton / Wine)**

These applications fail to produce audio because they make **incorrect or incompatible assumptions** about ALSA device semantics and PulseAudio availability.

---

## Key Design Constraints (Non‑Negotiable)

1. The ALSA `equal` plugin **must be first** in the playback chain
2. JACK must **own all real hardware**
3. Device switching is done **only via JACK routing**, never ALSA rewrites
4. The solution must be **portable** (works on any machine running `install.sh`)
5. No hardcoded hardware IDs
6. No fragile Steam / Proton internals hacks

---

## Why Proton Audio Fails in This Architecture

Proton / Wine ALSA clients expect:
- A simple, hardware-backed ALSA PCM
- Exclusive or dmix-style semantics
- No JACK plugin underneath

Your architecture intentionally violates these assumptions:

```
App → ALSA → equal → JACK → routing → hardware
```

This is **correct and desirable**, but Proton does not tolerate it.

Attempts to force Proton to use:
- `pcm.jack`
- nested `plug → equal → jack`
- custom `pcm.steam`

fail because:
- JACK is not a valid `dmix` slave
- Proton probes devices in non-standard ways
- Steam still tries PulseAudio code paths internally

---

## Correct Architectural Fix: ALSA Loopback as a Gaming Bridge

### Core Insight

**ALSA loopback (`snd-aloop`) is the only abstraction layer that satisfies all constraints**:

- Appears as real hardware to Proton
- Does not require PulseAudio
- Is kernel-standard and portable
- Cleanly decouples Proton from JACK internals

Instead of forcing Proton into JACK directly:

```
Proton → ALSA Loopback → JACK (alsa_in) → routing → hardware
```

This matches professional audio system design and avoids all Proton edge cases.

---

## Conceptual Model: Gaming as a Bridge Mode

This project already treats audio outputs as **bridge roles**:

| Mode | Bridge Type | Spawned When |
|----|----|----|
| USB | alsa_out | boot |
| HDMI | alsa_out | boot |
| Bluetooth | alsa_out (bluealsa) | on demand |
| **Gaming** | **alsa_in (loopback)** | **on demand** |

**Gaming is not special.**
It is just another on-demand bridge, like Bluetooth.

---

## Division of Responsibility

### `jack-rt`
- Starts JACK headless
- Owns real hardware
- **Must NOT** care about gaming / Proton

### `jack-bridge-ports`
- Manages persistent and dynamic bridges
- Already handles:
  - USB
  - HDMI
  - Bluetooth
- **Is the correct place to manage Gaming (loopback)**

### `mxeq.c` / GUI
- User-facing control
- Device selection
- Signals when Gaming mode is selected

---

## ALSA Configuration (Portable and Static)

A single gaming PCM is provided system-wide:

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

Notes:
- `equal` remains the default PCM
- This PCM does nothing unless selected
- If `snd-aloop` is not loaded, ALSA open fails safely

---

## Runtime Behavior (End‑to‑End)

1. User selects **Gaming** in the GUI
2. GUI switches ALSA output to `pcm.gaming`
3. GUI signals `jack-bridge-ports`
4. `jack-bridge-ports`:
   - Loads `snd-aloop` if needed
   - Spawns:
     ```
     alsa_in -j gaming_in -d hw:Loopback,1,0
     ```
5. JACK graph now contains persistent `gaming_in`
6. Existing routing logic handles the rest

Proton games are launched with:

```
WINEALSAOUTPUT=gaming %command%
```

No wrappers. No Steam hacks. No Pulse.

---

## Why This Is Correct

- Uses only standard kernel and ALSA features
- Matches JACK’s intended design
- Avoids illegal ALSA constructs (dmix → jack)
- Portable across hardware and distros
- Fits existing project abstractions perfectly

---

## Next Implementation Steps (Optional)

The following can be done incrementally:

1. **Minimal diff to `jack-bridge-ports`**
   - Add lazy `snd-aloop` loader
   - Add on-demand `gaming_in` spawn

2. **mxeq.c integration**
   - Add “Gaming” radio button
   - Signal bridge spawn / teardown

3. **Documentation / UX polish**
   - Explain Gaming mode
   - Explain Proton launch option

---

## Summary

This is not a workaround.
It is the **missing abstraction layer** required to integrate hostile clients (Proton) into a clean ALSA + JACK system.

Once Gaming is treated as a first‑class bridge type, the architecture becomes complete.


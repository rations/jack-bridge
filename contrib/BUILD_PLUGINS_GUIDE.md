# Step-by-Step Guide: Build Matching ALSA Plugins

This guide will help you build `libasound_module_pcm_bluealsa.so` and `libasound_module_ctl_bluealsa.so` that match your prebuilt `bluealsad` daemon (commit b0dd89b).

---

## Why Build Matching Plugins?

Your daemon is custom-built from commit `b0dd89b`, but the distro ALSA plugin is from Feb 2023 (likely BlueALSA v4.0.0). Version mismatches can cause:
- D-Bus communication failures
- Device enumeration issues
- Audio stream initialization problems

**Solution**: Build plugins from the same commit as your daemon.

---

## Quick Start (Automated Script)

The easiest way is to use the provided script:

```bash
cd ~/build_a_bridge/jack-bridge
sh contrib/build-bluealsa-plugins.sh
```

The script will:
1. Detect your daemon version
2. Install build dependencies
3. Clone BlueALSA
4. Checkout matching commit
5. Build plugins
6. Copy to `contrib/bin/`
7. Provide next steps

**Total time**: ~5-10 minutes depending on your system.

---

## Manual Build (Step-by-Step)

If you prefer to build manually or the script fails:

### Step 1: Verify Daemon Version

```bash
/usr/local/bin/bluealsad --version
```

**Output**: `b0dd89b` (or different commit hash)

Save this commit hash - you'll need it.

---

### Step 2: Install Build Dependencies

```bash
sudo apt update
sudo apt install -y \
    git \
    build-essential \
    automake \
    libtool \
    pkg-config \
    libbluetooth-dev \
    libdbus-1-dev \
    libasound2-dev \
    libglib2.0-dev \
    libsbc-dev
```

**Optional** (for better codec support - AAC, aptX, LDAC, LC3):
```bash
sudo apt install -y \
    libfdk-aac-dev \
    libldacbt-abr-dev \
    libldacbt-enc-dev \
    liblc3-dev
```

**Time**: ~2-3 minutes

---

### Step 3: Clone BlueALSA Repository

```bash
cd /tmp
git clone https://github.com/Arkq/bluez-alsa.git
cd bluez-alsa
```

**Time**: ~30 seconds

---

### Step 4: Checkout Matching Commit

Replace `b0dd89b` with YOUR daemon's commit if different:

```bash
git checkout b0dd89b
```

**Expected output**:
```
Note: switching to 'b0dd89b'.
HEAD is now at b0dd89b [commit message]
```

---

### Step 5: Generate Build System

```bash
autoreconf --install --force
```

**Expected output**:
```
libtoolize: putting auxiliary files in '.'.
libtoolize: copying file './ltmain.sh'
...
configure.ac: installing './install-sh'
configure.ac: installing './missing'
```

**Time**: ~10-20 seconds

---

### Step 6: Configure Build

**Option A: With All Codecs** (recommended if dependencies installed):
```bash
./configure \
    --enable-aplay \
    --enable-rfcomm \
    --enable-cli \
    --enable-aac \
    --enable-aptx \
    --enable-ldac \
    --enable-lc3-swb
```

**Option B: Minimal** (if codec libraries missing):
```bash
./configure \
    --enable-aplay \
    --enable-rfcomm \
    --enable-cli
```

**Expected output** (end of configure):
```
BlueALSA configuration:
  - version: 4.x.x
  - codecs: SBC AAC aptX LDAC LC3-SWB
  - profiles: A2DP HFP HSP
  - ALSA plugins: yes
  ...
```

**Important**: Verify `ALSA plugins: yes` is shown!

**Time**: ~5-10 seconds

---

### Step 7: Build

```bash
make -j$(nproc)
```

This compiles BlueALSA. We only need the ALSA plugins, not the daemon (you already have the daemon prebuilt).

**Time**: ~2-5 minutes depending on CPU

**Expected output** (near end):
```
  CC       libasound_module_pcm_bluealsa_la-pcm.lo
  CC       libasound_module_ctl_bluealsa_la-ctl.lo
  CCLD     libasound_module_pcm_bluealsa.la
  CCLD     libasound_module_ctl_bluealsa.la
```

---

### Step 8: Verify Build Output

```bash
ls -lh src/.libs/libasound_module_*.so
```

**Expected output**:
```
-rwxr-xr-x 1 user user  55K [date] src/.libs/libasound_module_ctl_bluealsa.so
-rwxr-xr-x 1 user user 120K [date] src/.libs/libasound_module_pcm_bluealsa.so
```

**Note**: Actual sizes may vary by commit/codecs enabled.

---

### Step 9: Copy to Repository

```bash
cp src/.libs/libasound_module_pcm_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/
cp src/.libs/libasound_module_ctl_bluealsa.so ~/build_a_bridge/jack-bridge/contrib/bin/
```

**Verify**:
```bash
ls -lh ~/build_a_bridge/jack-bridge/contrib/bin/libasound_module_*.so
```

---

### Step 10: Cleanup Build Directory

```bash
cd ~
rm -rf /tmp/bluez-alsa
```

---

### Step 11: Commit to Repository

```bash
cd ~/build_a_bridge/jack-bridge
git add contrib/bin/libasound_module_pcm_bluealsa.so
git add contrib/bin/libasound_module_ctl_bluealsa.so
git commit -m "Add matching ALSA plugins for BlueALSA daemon b0dd89b

Built from bluez-alsa commit b0dd89b to ensure plugin/daemon API compatibility.
Includes PCM plugin (required) and CTL plugin (optional mixer controls)."
```

---

## Installation & Testing

### Install Plugins

```bash
cd ~/build_a_bridge/jack-bridge
sudo ./contrib/install.sh
```

The installer will now:
1. Backup distro plugins to `*.distro-backup`
2. Install your matching plugins
3. Verify ALSA configuration

### Reboot

```bash
sudo reboot
```

### Test

```bash
# After reboot, test ALSA direct playback
bluetoothctl connect MAC_ADDRESS
aplay -D "jackbridge_bluealsa:DEV=MAC_ADDRESS,PROFILE=a2dp" /usr/share/sounds/alsa/Front_Center.wav
```

**Expected**: You should hear audio through your Bluetooth device!

### Full Testing

```bash
sh contrib/test-bluetooth.sh MAC_ADDRESS
```

---

## Troubleshooting

### Configure fails with "Package requirements not met"

**Problem**: Missing development libraries.

**Solution**: Install the specific library mentioned:
```bash
sudo apt install -y lib[package]-dev
```

Common ones:
- `libbluetooth-dev` - Bluetooth support
- `libasound2-dev` - ALSA support
- `libsbc-dev` - SBC codec (required for A2DP)
- `libfdk-aac-dev` - AAC codec (optional, better quality)

### Build fails with compiler errors

**Problem**: Your commit may be very old and incompatible with current GCC.

**Solutions**:
1. Try updating to a more recent BlueALSA commit:
   ```bash
   cd /tmp/bluez-alsa
   git log --oneline | head -20  # See recent commits
   git checkout [recent-commit]
   ```

2. Or use compatibility flags:
   ```bash
   CFLAGS="-Wno-error" ./configure ...
   make CFLAGS="-Wno-error"
   ```

### Plugins not copied to correct location

**Check**:
```bash
ls -la ~/build_a_bridge/jack-bridge/contrib/bin/libasound_module_*.so
```

**If missing**:
```bash
cp /tmp/bluez-alsa/src/.libs/libasound_module_*.so ~/build_a_bridge/jack-bridge/contrib/bin/
```

### After install, ALSA still doesn't work

**Verify plugins installed**:
```bash
ls -la /usr/lib/x86_64-linux-gnu/alsa-lib/libasound_module_*bluealsa.so
```

**Check for backup**:
```bash
ls -la /usr/lib/x86_64-linux-gnu/alsa-lib/*.distro-backup
```

**Manual install**:
```bash
sudo cp contrib/bin/libasound_module_pcm_bluealsa.so /usr/lib/x86_64-linux-gnu/alsa-lib/
sudo cp contrib/bin/libasound_module_ctl_bluealsa.so /usr/lib/x86_64-linux-gnu/alsa-lib/
```

---

## Quick Reference: One-Liner Build

If you know your commit hash and have dependencies installed:

```bash
cd /tmp && \
git clone https://github.com/Arkq/bluez-alsa.git && \
cd bluez-alsa && \
git checkout b0dd89b && \
autoreconf --install --force && \
./configure --enable-aplay --enable-rfcomm --enable-cli && \
make -j$(nproc) && \
cp src/.libs/libasound_module_*.so ~/build_a_bridge/jack-bridge/contrib/bin/ && \
echo "✓ Plugins built and copied to contrib/bin/"
```

---

## Alternative: Use Automated Script

For convenience, just run:

```bash
cd ~/build_a_bridge/jack-bridge
sh contrib/build-bluealsa-plugins.sh
```

Follow the prompts. The script handles everything automatically.

---

## File Sizes Reference

Typical plugin sizes (will vary by commit and codecs):

- `libasound_module_pcm_bluealsa.so`: 50-150 KB
- `libasound_module_ctl_bluealsa.so`: 15-40 KB

If your files are much larger (>500KB) or smaller (<10KB), something may be wrong with the build.

---

## Summary

**Total Time**: ~10 minutes
**Difficulty**: Easy (if using script) to Moderate (if manual)
**Required**: Yes (for reliable Bluetooth audio)

After building and installing matching plugins, your Bluetooth audio should work reliably without version mismatch errors.
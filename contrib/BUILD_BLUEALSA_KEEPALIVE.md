# Building BlueALSA with Keep-Alive Support

## The Problem
Your prebuilt BlueALSA binaries don't support `--keep-alive`, which is why the A2DP transport keeps dropping and no JACK ports appear.

## Build Commands

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y git automake build-essential libtool pkg-config \
    libasound2-dev libbluetooth-dev libdbus-1-dev libglib2.0-dev \
    libsbc-dev libmp3lame-dev libmpg123-dev

# 2. Clone BlueALSA with keep-alive support
cd /tmp
git clone https://github.com/Arkq/bluez-alsa.git
cd bluez-alsa

# 3. Check for keep-alive support (added in v4.0.0+)
git checkout v4.1.0  # Latest stable with keep-alive

# 4. Build with all codecs and features
autoreconf --install
mkdir build && cd build
../configure \
    --enable-aac \
    --enable-mp3lame \
    --enable-mpg123 \
    --enable-ofono \
    --enable-debug \
    --with-libopenaptx \
    --prefix=/usr/local \
    --sysconfdir=/etc \
    --localstatedir=/var

make -j$(nproc)

# 5. Install new binaries
sudo make install

# 6. Copy to jack-bridge location
sudo cp /usr/local/bin/bluealsa /usr/local/bin/bluealsad
sudo cp /usr/local/bin/bluealsa-aplay /usr/local/bin/bluealsa-aplay

# 7. Verify keep-alive support
/usr/local/bin/bluealsad --help | grep keep-alive
```

## If Keep-Alive Still Not Available

Use the transport idle timeout approach:

```bash
# In bluealsad init script, add:
BLUEALSAD_ARGS="--io-rt-priority=90 --a2dp-volume --profile=a2dp-source --profile=a2dp-sink"

# Set transport idle timeout (different from keep-alive)
echo 0 > /sys/module/bluetooth/parameters/disable_esco
```

## Alternative: Force Persistent Ports with ALSA Loopback

If BlueALSA won't keep transports alive, create persistent ports using ALSA loopback:

```bash
# 1. Load loopback module at boot
echo "snd-aloop" >> /etc/modules
modprobe snd-aloop index=7 pcm_substreams=2

# 2. In jack-bridge-ports init script:
# Create persistent bt_out ports using loopback (always present)
alsa_out -j bt_out -d hw:Loopback,0,0 -r 48000 -p 256 -n 3 &

# 3. Create a simple bridge when device connects:
# When BlueALSA PCM appears, bridge it:
alsaloop -C hw:Loopback,1,0 -P bluealsa -t 100000 &
```

This gives you `bt_out:playback_1/2` ports that ALWAYS exist on boot, just like USB/HDMI.

## Testing After Build

```bash
# 1. Restart with new binaries
sudo service bluealsad stop
sudo service bluealsad start

# 2. Check if keep-alive is working
ps aux | grep bluealsad
# Should show: bluealsad --keep-alive=-1 ...

# 3. Connect device
bluetoothctl connect YOUR_MAC

# 4. Check if transport stays alive
bluealsactl list-pcms
# Should show PCM even when not playing audio

# 5. Check JACK ports
jack_lsp | grep bluealsa
# Should show ports immediately
```

## The Root Issue

BlueALSA was designed to free transports when idle to save power. The `--keep-alive` option was added in later versions (4.0.0+) specifically for scenarios like yours where persistent connections are needed.

Your prebuilt binaries are likely an older version without this feature, which is why the transport keeps dropping.
# Installer Fixes Plan

## Issues to Address

1. **Missing 50-jack.conf installation** - The installer doesn't install the 50-jack.conf file to the ALSA configuration directory
2. **qjackctl Setup button crash** - Clicking the Setup button in qjackctl crashes the GUI
3. **Missing icon in qjackctl desktop file** - The desktop file doesn't contain its icon

## Solutions

### 1. Add qjackctl Icon to Repository

**Problem**: The qjackctl desktop file references `Icon=qjackctl` but the icon is not installed system-wide.

**Solution**: 
- Copy qjackctl icon files from `qjackctl-1.0.4/src/images/` to `contrib/usr/share/icons/hicolor/`
- Install the icons during the installer execution
- Update the installer to install the icon to standard locations

**Files to create**:
- `contrib/usr/share/icons/hicolor/scalable/apps/qjackctl.svg`
- `contrib/usr/share/icons/hicolor/128x128/apps/qjackctl.png`
- `contrib/usr/share/icons/hicolor/64x64/apps/qjackctl.png`
- `contrib/usr/share/icons/hicolor/48x48/apps/qjackctl.png`
- `contrib/usr/share/icons/hicolor/32x32/apps/qjackctl.png`
- `contrib/usr/share/icons/hicolor/16x16/apps/qjackctl.png`

### 2. Install 50-jack.conf with Path Detection

**Problem**: The installer doesn't install 50-jack.conf, which is needed for ALSA to JACK bridging.

**Solution**:
- Add code to detect the correct ALSA configuration directory on the user's system
- Install 50-jack.conf to the detected directory
- Try multiple common locations: `/usr/share/alsa/alsa.conf.d`, `/etc/alsa/conf.d`, etc.

**Implementation**:
```bash
# Detect ALSA configuration directory
ALSA_CONF_DIRS="/usr/share/alsa/alsa.conf.d /etc/alsa/conf.d /usr/share/alsa/alsa.conf.d /etc/alsa/conf.d"
ALSA_CONF_INSTALLED=0

for DIR in $ALSA_CONF_DIRS; do
    if [ -d "$DIR" ]; then
        mkdir -p "$DIR"
        if install -m 0644 50-jack.conf "$DIR/50-jack.conf"; then
            echo "Installed 50-jack.conf to $DIR/50-jack.conf"
            ALSA_CONF_INSTALLED=1
            break
        fi
    fi
done

if [ "$ALSA_CONF_INSTALLED" -eq 0 ]; then
    echo "Warning: Could not determine ALSA configuration directory"
    echo "Please manually install 50-jack.conf to your ALSA configuration directory"
fi
```

### 3. Update qjackctl Desktop File

**Problem**: The desktop file has `Icon=qjackctl` but the icon may not be available.

**Solution**:
- Ensure the desktop file references the correct icon name
- Install the icon to standard locations so the desktop file can find it

**Desktop file should have**:
```
[Desktop Entry]
Name=QjackCtl
Comment=JACK Audio Connection Kit Control Panel
Exec=/usr/local/bin/qjackctl
Icon=qjackctl
Terminal=false
Type=Application
Categories=AudioVideo;Audio;Midi;
StartupNotify=true
```

### 4. Investigate qjackctl Setup Button Crash

**Problem**: Clicking Setup in qjackctl crashes the GUI.

**Solution**:
- This may be related to the custom build or configuration
- Need to investigate the qjackctl source code and build process
- Check if there are any missing dependencies or configuration issues
- May need to update the custom build script or qjackctl configuration

## Implementation Steps

1. **Add icons to repository**
   - Copy icon files from qjackctl-1.0.4 to contrib directory
   - Create multiple sizes for compatibility

2. **Update contrib/install.sh**
   - Add icon installation section
   - Add 50-jack.conf installation with path detection
   - Ensure proper permissions and directories

3. **Test the changes**
   - Run the installer and verify all components are installed
   - Test qjackctl functionality including Setup button
   - Verify icon appears in desktop environment

## Success Criteria

- [ ] 50-jack.conf is installed to the correct ALSA configuration directory
- [ ] qjackctl icon is installed and visible in desktop environment
- [ ] qjackctl Setup button works without crashing
- [ ] All changes work on different Linux distributions
- [ ] No hardcoded paths - installer detects correct locations

## Next Steps

Switch to Code mode to implement these changes.
// See devices.h.

#include "devices.h"

#include "platform/wakepipe.h"
#include "platform/fs.h"
#include "platform/proc.h"

#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace jackbridge
{

const char *Devices::kRouteHelper = "/usr/local/lib/jack-bridge/jack-route-select";

namespace
{

const char *const kSystemDevConf = "/etc/jack-bridge/devices.conf";

std::string userDevConf()
{
    return fs::configHome() + "/jack-bridge/devices.conf";
}

std::string lowered(const std::string &s)
{
    std::string out = s;
    for (char &c : out) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u >= 'A' && u <= 'Z')
            c = static_cast<char>(u - 'A' + 'a');
    }
    return out;
}

// Trim whitespace and double quotes from both ends -- what the shell config's values are wrapped in.
std::string trimValue(const std::string &s)
{
    const char *ws = " \t\r\n\"";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos)
        return std::string();
    const size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// The value of `key=` from a shell-style config, or empty.
std::string readConfKey(const std::string &path, const std::string &key)
{
    std::string body;
    if (!fs::readFile(path, body))
        return std::string();

    size_t pos = 0;
    while (pos < body.size()) {
        size_t eol = body.find('\n', pos);
        if (eol == std::string::npos)
            eol = body.size();
        const std::string line = body.substr(pos, eol - pos);
        pos = eol + 1;
        if (line.compare(0, key.size(), key) == 0 && line.size() > key.size() &&
            line[key.size()] == '=') {
            return trimValue(line.substr(key.size() + 1));
        }
    }
    return std::string();
}

} // namespace

//------------------------------------------------------------------------
const char *Devices::token(Output o)
{
    switch (o) {
        case Output::USB:
            return "usb";
        case Output::HDMI:
            return "hdmi";
        case Output::Bluetooth:
            return "bluetooth";
        case Output::Internal:
            break;
    }
    return "internal";
}

bool Devices::parseToken(const std::string &s, Output *out)
{
    if (s == "usb")
        *out = Output::USB;
    else if (s == "hdmi")
        *out = Output::HDMI;
    else if (s == "bluetooth")
        *out = Output::Bluetooth;
    else if (s == "internal")
        *out = Output::Internal;
    else
        return false;
    return true;
}

//------------------------------------------------------------------------
// The first card whose `aplay -l` section does not mention USB. Zero if none.
//
// THIS MUST KEEP AGREEING WITH jack-route-select's awk version -- see the header. The structure is
// kept line for line: a card header commits the previous candidate, and any line within a card's
// section mentioning "usb" disqualifies it. That covers USB named only on a subdevice line, which
// is why the test is per line and not only on the header.
int Devices::internalCardNumber()
{
    std::string out;
    if (proc::runCapture({"aplay", "-l"}, out) != 0)
        return 0;

    int cardNum = -1;
    int foundNonUsb = -1;

    size_t pos = 0;
    while (pos < out.size()) {
        size_t eol = out.find('\n', pos);
        if (eol == std::string::npos)
            eol = out.size();
        const std::string line = out.substr(pos, eol - pos);
        pos = eol + 1;

        int n = 0;
        if (sscanf(line.c_str(), "card %d:", &n) == 1) {
            // A new header commits the previous candidate: it survived its whole section.
            if (foundNonUsb != -1)
                return foundNonUsb;
            cardNum = n;
            foundNonUsb = n;
        }

        if (foundNonUsb == cardNum && lowered(line).find("usb") != std::string::npos)
            foundNonUsb = -1;
    }

    return foundNonUsb != -1 ? foundNonUsb : 0;
}

int Devices::usbCardNumber()
{
    std::string out;
    if (proc::runCapture({"aplay", "-l"}, out) != 0)
        return -1;

    size_t pos = 0;
    while (pos < out.size()) {
        size_t eol = out.find('\n', pos);
        if (eol == std::string::npos)
            eol = out.size();
        const std::string line = out.substr(pos, eol - pos);
        pos = eol + 1;

        int n = 0;
        if (sscanf(line.c_str(), "card %d:", &n) == 1 &&
            lowered(line).find("usb") != std::string::npos) {
            return n;
        }
    }
    return -1;
}

bool Devices::usbPresent()
{
    return usbCardNumber() >= 0;
}

bool Devices::btPresent()
{
    if (fs::exists("/usr/bin/bluealsa") || fs::exists("/usr/sbin/bluealsa"))
        return true;
    // `pidof` rather than a file test, because bluealsad may be installed anywhere; its exit status
    // is the whole answer, so nothing is captured.
    return proc::run({"pidof", "bluealsad"}) == 0;
}

std::string Devices::cardIdForNumber(int card)
{
    std::string body;
    if (!fs::readFile("/proc/asound/card" + std::to_string(card) + "/id", body))
        return std::string();
    const char *ws = " \t\r\n";
    const size_t b = body.find_first_not_of(ws);
    if (b == std::string::npos)
        return std::string();
    const size_t e = body.find_last_not_of(ws);
    return body.substr(b, e - b + 1);
}

//------------------------------------------------------------------------
bool Devices::jackdIsOnUsbCard()
{
    const int usbCard = usbCardNumber();
    if (usbCard < 0)
        return false;
    const std::string usbId = cardIdForNumber(usbCard);
    if (usbId.empty())
        return false;

    std::string out;
    if (proc::runCapture({"ps", "-o", "args=", "-C", "jackd"}, out) != 0)
        return false;

    size_t pos = 0;
    while (pos < out.size()) {
        size_t eol = out.find('\n', pos);
        if (eol == std::string::npos)
            eol = out.size();
        const std::string line = out.substr(pos, eol - pos);
        pos = eol + 1;

        // jackd-rt passes the device as `-P <device>`. The same parse lives in
        // jack-route-select's server_card_id().
        const size_t p = line.find("-P ");
        if (p == std::string::npos)
            continue;
        char dev[256] = {0};
        if (sscanf(line.c_str() + p + 3, "%255s", dev) != 1)
            continue;

        const std::string d(dev);
        if (d.compare(0, 8, "hw:CARD=") == 0) {
            std::string id = d.substr(8);
            const size_t comma = id.find(',');
            if (comma != std::string::npos)
                id.resize(comma);
            if (id == usbId)
                return true;
        } else if (d.compare(0, 3, "hw:") == 0) {
            if (atoi(d.c_str() + 3) == usbCard)
                return true;
        }
    }
    return false;
}

bool Devices::hdmiPortsExist()
{
    // Was popen("jack_lsp 2>/dev/null | grep -q '^hdmi_out:'"). One process instead of two, and the
    // exit status is jack_lsp's rather than grep's -- so "JACK is not running" and "JACK is running
    // and has no HDMI ports" are now distinguishable, which they were not.
    std::string out;
    if (proc::runCapture({"jack_lsp"}, out) != 0)
        return false;
    // Anchored at a line start, as the '^' in the original grep was.
    if (out.compare(0, 9, "hdmi_out:") == 0)
        return true;
    return out.find("\nhdmi_out:") != std::string::npos;
}

bool Devices::bluealsaPortsExist()
{
    std::string out;
    if (proc::runCapture({"jack_lsp"}, out) != 0)
        return false;
    return out.find("bluealsa:playback_1") != std::string::npos;
}

//------------------------------------------------------------------------
std::string Devices::loadPreferredOutput()
{
    // Per-user first, then system-wide. jack-route-select writes the per-user copy on every switch.
    for (const std::string &path : {userDevConf(), std::string(kSystemDevConf)}) {
        const std::string v = readConfKey(path, "PREFERRED_OUTPUT");
        if (!v.empty())
            return v;
    }
    return "internal";
}

std::string Devices::loadBluetoothDeviceMac()
{
    // BLUETOOTH_DEVICE="...DEV=AA:BB:CC:DD:EE:FF..." -- the MAC is the 17 characters after DEV=.
    // Per-user only, which is where jack-route-select writes it.
    const std::string v = readConfKey(userDevConf(), "BLUETOOTH_DEVICE");
    const size_t dev = v.find("DEV=");
    if (dev == std::string::npos)
        return std::string();
    const std::string mac = v.substr(dev + 4);
    if (mac.size() < 17)
        return std::string();
    return mac.substr(0, 17);
}

std::string Devices::macFromBluezObject(const std::string &s)
{
    if (s.empty())
        return std::string();
    if (s.find('/') == std::string::npos)
        return s; // already a MAC

    const size_t slash = s.find_last_of('/');
    std::string tail = s.substr(slash + 1);

    // Expected form "dev_XX_XX_XX_XX_XX_XX". Anything else is handled best-effort the same way the
    // GTK build did: take the tail and turn underscores into colons.
    if (tail.compare(0, 4, "dev_") == 0)
        tail = tail.substr(4);
    for (char &c : tail) {
        if (c == '_')
            c = ':';
    }
    return tail;
}

//------------------------------------------------------------------------
void Devices::ensureAsoundrcBootstrap()
{
    // The managed block, delimited so everything a user put in ~/.asoundrc themselves survives:
    //
    //     # BEGIN jack-bridge
    //     include "<config>/jack-bridge/current_input.conf"
    //     include "<config>/jack-bridge/current_output.conf"
    //     # END jack-bridge
    //
    // Only the includes live in ~/.asoundrc; the fragments they name are written by
    // jack-route-select on every switch. This runs once at start-up and writes nothing if the input
    // fragment already exists, which is what the GTK build's ensure_user_asoundrc_bootstrap did.
    const std::string dir = fs::configHome() + "/jack-bridge";
    const std::string inPath = dir + "/current_input.conf";
    const std::string outPath = dir + "/current_output.conf";

    std::string existing;
    if (fs::readFile(inPath, existing) && existing.find("slave.pcm") != std::string::npos)
        return; // already bootstrapped

    if (!fs::makeDirs(dir)) {
        fprintf(stderr, "jack-bridge: cannot create %s\n", dir.c_str());
        return;
    }

    // Mirror the system default if there is one, else input_card0 -- the same fallback the C used.
    std::string slave = "input_card0";
    std::string sys;
    if (fs::readFile("/etc/asound.conf.d/current_input.conf", sys)) {
        const size_t p = sys.find("slave.pcm");
        if (p != std::string::npos) {
            const size_t q1 = sys.find('"', p);
            const size_t q2 = q1 == std::string::npos ? std::string::npos : sys.find('"', q1 + 1);
            if (q2 != std::string::npos)
                slave = sys.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    fs::writeFileAtomic(inPath, "pcm.current_input {\n    type plug\n    slave.pcm \"" + slave +
                                    "\"\n}\n");

    // Now the include block in ~/.asoundrc, preserving everything outside the markers.
    const char *kBegin = "# BEGIN jack-bridge";
    const char *kEnd = "# END jack-bridge";
    const std::string rcPath = fs::homeDir() + "/.asoundrc";

    std::string rc;
    fs::readFile(rcPath, rc);

    // Strip any existing block. A begin with no end is malformed and is left alone rather than
    // truncating the rest of somebody's file.
    const size_t b = rc.find(kBegin);
    if (b != std::string::npos) {
        const size_t e = rc.find(kEnd, b);
        if (e != std::string::npos) {
            size_t after = e + strlen(kEnd);
            while (after < rc.size() && (rc[after] == '\r' || rc[after] == '\n'))
                ++after;
            rc = rc.substr(0, b) + rc.substr(after);
        }
    }

    while (!rc.empty() && (rc.back() == '\n' || rc.back() == '\r'))
        rc.pop_back();
    if (!rc.empty())
        rc += "\n";

    rc += std::string("\n") + kBegin + "\n";
    rc += "# Managed by jack-bridge -- do not edit between markers.\n";
    rc += "include \"" + inPath + "\"\n";
    rc += "include \"" + outPath + "\"\n";
    rc += std::string(kEnd) + "\n";

    // ATOMIC: ALSA may open ~/.asoundrc at any moment from any process on the machine, and a
    // truncate-and-write leaves a window where it reads half a file. fs::writeFileAtomic does
    // tmp-in-the-same-directory, fsync, rename -- which is what write_string_atomic did, plus the
    // fsync it did not.
    if (!fs::writeFileAtomic(rcPath, rc))
        fprintf(stderr, "jack-bridge: cannot write %s\n", rcPath.c_str());
}

//------------------------------------------------------------------------
bool Devices::available(Output o) const
{
    switch (o) {
        case Output::USB:
            return usbPresent();
        case Output::Bluetooth:
            return btPresent();
        case Output::HDMI:
            // Always offered: whether the card has an HDMI output is not something aplay tells us
            // reliably, and the routing helper reports its own failure. This is what the GTK build
            // did.
            return true;
        case Output::Internal:
            break;
    }
    return true;
}

std::string Devices::detail(Output o) const
{
    switch (o) {
        case Output::Internal:
        case Output::HDMI:
            break;
        case Output::USB:
            // USB is not bridged: jack-route-select restarts jackd on the interface (see the top of
            // devices.h), so choosing it here is not the live switch the other three are.
            if (!usbPresent())
                return "no USB interface connected";
            return "USB is not bridged, live device switching is only for Internal, HDMI and "
                   "Bluetooth";
        case Output::Bluetooth:
            if (!btPresent())
                return "BlueALSA is not installed or not running";
            if (bluetoothTargetMac().empty())
                return "no device selected";
            break;
    }
    return std::string();
}

//------------------------------------------------------------------------
void Devices::setBluetoothSelection(const std::string &objectPathOrMac)
{
    mBtSelection = objectPathOrMac;
}

std::string Devices::bluetoothTargetMac() const
{
    if (!mBtSelection.empty()) {
        const std::string mac = macFromBluezObject(mBtSelection);
        if (!mac.empty())
            return mac;
    }
    return loadBluetoothDeviceMac();
}

//------------------------------------------------------------------------
void Devices::applyMixerFor(Output o)
{
    switch (o) {
        case Output::USB: {
            const int card = usbCardNumber();
            if (card >= 0) {
                if (onMixerCard)
                    onMixerCard(card, false); // USB is never curated
            } else if (onMixerPlaceholder) {
                onMixerPlaceholder("The USB audio interface is not connected.");
            }
            return;
        }
        case Output::HDMI:
            if (onMixerPlaceholder) {
                onMixerPlaceholder("Mixer controls are not available for HDMI output.\n"
                                   "Use your display or receiver to adjust the volume.");
            }
            return;
        case Output::Bluetooth:
            if (onMixerPlaceholder) {
                onMixerPlaceholder("Mixer controls are not available for Bluetooth output.\n"
                                   "Use your Bluetooth device to adjust the volume.");
            }
            return;
        case Output::Internal:
            break;
    }
    if (onMixerCard)
        onMixerCard(internalCardNumber(), true); // the internal card is the curated one
}

//------------------------------------------------------------------------
// What to show at start-up, and the ONE case that routes.
//
// A device that is actually live wins over a saved preference; only when nothing is running do we
// fall back to PREFERRED_OUTPUT and apply it. USB needs that fallback as much as HDMI does: if the
// boot restore could not put jackd on the interface -- it can refuse to open for the first seconds
// after boot, and jackd-rt falls back to the internal card when it is absent -- then jackd is not on
// USB, and without the preference check the GUI silently selected Internal and left the user's saved
// choice unapplied.
void Devices::initialise()
{
    const std::string pref = loadPreferredOutput();

    const bool haveUsb = available(Output::USB);
    const bool haveBt = available(Output::Bluetooth);

    const bool btActive = haveBt && bluealsaPortsExist();
    const bool usbActive = haveUsb && jackdIsOnUsbCard();
    const bool hdmiActive = hdmiPortsExist();

    const char *startupRoute = nullptr;

    if (btActive) {
        mSelected = Output::Bluetooth;
    } else if (usbActive) {
        mSelected = Output::USB;
    } else if (hdmiActive) {
        mSelected = Output::HDMI;
    } else if (pref == "usb" && haveUsb) {
        mSelected = Output::USB;
        startupRoute = "usb"; // ports not yet running
    } else if (pref == "hdmi") {
        mSelected = Output::HDMI;
        startupRoute = "hdmi"; // ports not yet running
    } else {
        mSelected = Output::Internal;
    }

    applyMixerFor(mSelected);

    if (startupRoute) {
        if (fs::exists(kRouteHelper))
            proc::spawnAsync({kRouteHelper, startupRoute});
        else
            fprintf(stderr, "jack-bridge: routing helper missing: %s\n", kRouteHelper);
    }

    // BASELINE THE POLL FROM PREFERRED_OUTPUT, not from what was selected above. The poll's job is
    // to notice that PREFERRED_OUTPUT changed, so it has to start from the same value it will
    // compare against. Seeding it from the selection would make the first tick look like a change
    // whenever live state and saved preference disagree -- which is exactly the unplugged-interface
    // case -- and rebuild the mixer for nothing.
    mPolledOutput = pref;
    mPolledUsbPresent = usbPresent();
}

//------------------------------------------------------------------------
void Devices::select(Output o)
{
    if (o == Output::Bluetooth) {
        mSelected = o;
        applyMixerFor(o);
        if (onChanged)
            onChanged();

        // If the bridge is already up there is nothing to do -- re-routing here would also raise a
        // spurious "no device selected" message when the list has no selection yet, which is the
        // case when the GUI is opened while alsa_out is already running.
        if (bluealsaPortsExist())
            return;

        const std::string mac = bluetoothTargetMac();
        if (mac.empty()) {
            if (onMessage) {
                onMessage("No Bluetooth device selected. Open the Bluetooth page, connect a "
                          "device, and try again.",
                          true);
            }
            revertToInternal();
            return;
        }
        btRouteBegin(mac);
        return;
    }

    mSelected = o;
    applyMixerFor(o);

    if (!fs::exists(kRouteHelper)) {
        if (onMessage) {
            onMessage(std::string("Routing helper missing: ") + kRouteHelper +
                          " -- run sudo ./contrib/install.sh",
                      true);
        }
    } else if (proc::spawnAsync({kRouteHelper, token(o)}) <= 0) {
        if (onMessage)
            onMessage("Could not run the routing helper.", true);
    }

    if (onChanged)
        onChanged();
}

//------------------------------------------------------------------------
void Devices::revertToInternal()
{
    // Selecting Internal deliberately ROUTES: leaving the selection on Bluetooth with no bluealsa
    // ports would show a device that is not actually carrying audio.
    mSelected = Output::Internal;
    applyMixerFor(Output::Internal);
    if (fs::exists(kRouteHelper))
        proc::spawnAsync({kRouteHelper, "internal"});
    if (onChanged)
        onChanged();
}

void Devices::btRouteFail(const std::string &message)
{
    if (mBtPollTimer && removeTimer) {
        removeTimer(mBtPollTimer);
        mBtPollTimer = 0;
    }
    mBtRouteActive = false;
    mBtRouteMac.clear();
    if (onMessage)
        onMessage(message, true);
    revertToInternal();
}

void Devices::btRouteBegin(const std::string &mac)
{
    if (mBtRouteActive)
        return; // one switch at a time
    if (mac.empty())
        return;

    if (!fs::exists(kRouteHelper)) {
        if (onMessage) {
            onMessage(std::string("Routing helper missing: ") + kRouteHelper +
                          " -- run sudo ./contrib/install.sh",
                      true);
        }
        revertToInternal();
        return;
    }

    // An argv, never a command line: a MAC cannot contain a shell metacharacter, but the helper is
    // also given device names elsewhere and there is no reason for two conventions.
    const pid_t pid = proc::spawnAsync({kRouteHelper, "bluetooth", mac});
    if (pid <= 0) {
        if (onMessage)
            onMessage("Could not run the routing helper.", true);
        revertToInternal();
        return;
    }

    mBtRouteActive = true;
    mBtRouteMac = mac;
    if (onMessage)
        onMessage("Switching to " + mac + "...", false);

    wakepipe::watch(pid, [this](int status) { btRouteChildExit(status); });
}

void Devices::btRouteChildExit(int status)
{
    if (!(status == -1 || (WIFEXITED(status) && WEXITSTATUS(status) == 0))) {
        btRouteFail("Failed to set Bluetooth output. The routing helper reported an error -- "
                    "check that the device is connected and BlueALSA is running.");
        return;
    }

    // THE HELPER'S EXIT STATUS IS NOT ENOUGH: it exits zero without distinguishing a failed
    // alsa_out spawn. Confirm the ports really came up. Twenty polls at 250 ms is five seconds,
    // which is what the helper itself waits for.
    mBtPollsLeft = 20;
    if (!addTimer) {
        // No loop to poll on. Answer from one look rather than claiming success.
        if (bluealsaPortsExist()) {
            mBtRouteActive = false;
            if (onMessage)
                onMessage("Bluetooth output ready: " + mBtRouteMac, false);
        } else {
            btRouteFail("Bluetooth ports did not appear.");
        }
        return;
    }

    mBtPollTimer = addTimer(250, [this] {
        if (bluealsaPortsExist()) {
            if (mBtPollTimer && removeTimer) {
                removeTimer(mBtPollTimer);
                mBtPollTimer = 0;
            }
            mBtRouteActive = false;
            mSelected = Output::Bluetooth;
            if (onMessage) {
                onMessage("Bluetooth output ready: " + mBtRouteMac +
                              " (bluealsa:playback_1/2)",
                          false);
            }
            mBtRouteMac.clear();
            if (onChanged)
                onChanged();
            return;
        }

        if (--mBtPollsLeft <= 0) {
            btRouteFail("Bluetooth ports failed to appear. The device may have disconnected, "
                        "BlueALSA may not be running, or there is no active A2DP transport. "
                        "See /tmp/jack-route-select.log");
        }
    });
}

//------------------------------------------------------------------------
void Devices::poll()
{
    const std::string pref = loadPreferredOutput();

    const bool usb = usbPresent();
    if (usb != mPolledUsbPresent) {
        mPolledUsbPresent = usb;
        if (onChanged)
            onChanged(); // the USB entry's availability and detail line both changed
    }

    if (pref == mPolledOutput)
        return;

    // A saved "usb" with no interface attached is the hotplug fallback's doing: the server is back
    // on the internal card and the user's choice is being kept for when the cable returns. Show
    // what is actually playing, and LEAVE devices.conf ALONE.
    std::string shown = pref;
    if (pref == "usb" && !usb)
        shown = "internal";

    fprintf(stderr, "jack-bridge: output changed elsewhere (%s -> %s); updating the panel\n",
            mPolledOutput.c_str(), shown.c_str());

    Output o = Output::Internal;
    parseToken(shown, &o);
    mSelected = o;
    applyMixerFor(o);

    mPolledOutput = pref;
    if (onChanged)
        onChanged();
}

} // namespace jackbridge

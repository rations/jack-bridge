// Which output the machine is playing through, and how to change it.
//
// Ported from mxeq.c's devices section. NOTHING HERE CALLS THE JACK API: CLAUDE.md records that
// mxeq deliberately does not link libjack, so there is no `mxeq:*` client in the graph, and every
// JACK question is a `jack_lsp` subprocess through platform/proc. Do not "improve" that.
//
//------------------------------------------------------------------------------------------------
// FOUR RULES THAT WERE PAID FOR, and each is a comment in the C this replaces.
//
//   1. internalCardNumber() MUST KEEP AGREEING WITH jack-route-select. That shell helper carries an
//      awk implementation of exactly this rule (its own get_internal_card_number). If the two
//      drift, the GUI shows one card's mixer while audio routes to another. They were
//      differential-tested against each other over gaps in card numbering, two-digit card numbers,
//      a matching card appearing last, and USB named only on a subdevice line. CHANGE BOTH
//      TOGETHER -- CLAUDE.md says so, and this port changes neither.
//
//   2. jackdIsOnUsbCard() ASKS WHICH CARD THE SERVER WAS STARTED WITH, not whether a usb_out client
//      exists. USB is no longer bridged: jack-route-select restarts jackd ON the interface, so
//      there is no usb_out client to look for and system:playback_* IS the interface. jackd-rt
//      passes the device as `-P <device>`; the same parse lives in jack-route-select's
//      server_card_id().
//
//   3. THE BLUETOOTH SWITCH IS ASYNCHRONOUS THROUGHOUT. The original spawned the routing helper
//      synchronously and then slept another one to two seconds; since the helper itself waits up to
//      five seconds for its ports, that froze the whole window on every Bluetooth switch. Here the
//      helper is spawned async through childreaper and the ports are polled on a timer.
//
//   4. THE HELPER'S EXIT STATUS IS NOT ENOUGH. It exits zero without distinguishing a failed
//      alsa_out spawn, so success is confirmed by polling for bluealsa:playback_1 -- twenty times
//      at 250 ms, five seconds in total.
//
// A saved preference of "usb" with no interface attached is what the hotplug fallback leaves
// behind: the server is back on the internal card and the user's choice is kept for when the cable
// returns. shownOutput() reports what is actually playing and devices.conf is LEFT ALONE.

#pragma once

#include <functional>
#include <string>

namespace jackbridge
{

class Devices
{
public:
    enum class Output { Internal, USB, HDMI, Bluetooth };

    // The token jack-route-select takes, and the value stored in devices.conf as PREFERRED_OUTPUT.
    // Dispatch is on this, NEVER on a visible label: the GTK build attached it to each radio with
    // g_object_set_data for exactly that reason, so renaming a button in the UI cannot silently
    // break routing.
    static const char *token(Output o);
    static bool parseToken(const std::string &s, Output *out);

    Devices() = default;

    // Something the user needs to read, for the page's message strip.
    std::function<void(const std::string &message, bool isError)> onMessage;

    // The selected output changed, for whatever reason: the user picked one, a Bluetooth switch
    // completed or failed back to Internal, or the two-second poll noticed another process changed
    // it. The panel re-reads selected() and the mixer follows.
    std::function<void()> onChanged;

    // Called when the mixer should point somewhere else, with the card to open and whether to
    // curate it, or with a placeholder message when the output has no mixer at all. Extracted for
    // the same reason apply_device_mixer() was: three callers -- the click, the start-up sync and
    // the live poll -- must not drift into showing different controls for the same device.
    std::function<void(int cardNumber, bool curate)> onMixerCard;
    std::function<void(const std::string &placeholder)> onMixerPlaceholder;

    // Reads the live state and the saved preference and settles on what to show. Routes nothing
    // except the one on-demand case described in the .cpp. Call once at start-up, BEFORE any
    // click can arrive.
    void initialise();

    // The user chose an output. Routes, points the mixer, and for Bluetooth starts the async flow.
    void select(Output o);

    Output selected() const
    {
        return mSelected;
    }

    // Whether each output can be chosen at all. A device that cannot be used is DRAWN DISABLED AND
    // LEFT VISIBLE, never hidden -- see palette.h.
    bool available(Output o) const;

    // A line under each radio saying which card it resolved to, or why it is not available. The
    // GTK build had nowhere to put this.
    std::string detail(Output o) const;

    // Call every two seconds. Notices an output change made by ANOTHER PROCESS -- jack-graph's
    // JACK settings writes PREFERRED_OUTPUT, and a USB hotplug moves the server with no GUI
    // involved at all -- and notices the interface being unplugged.
    //
    // PREFERRED_OUTPUT is the whole input, because the mixer content is a pure function of it. That
    // keeps a tick to one small file read plus the /proc checks in usbPresent().
    void poll();

    // The MAC to route Bluetooth to: what the Bluetooth page has selected, else the device
    // jack-route-select saved on the last successful session. Empty if neither.
    void setBluetoothSelection(const std::string &objectPathOrMac);
    std::string bluetoothTargetMac() const;

    //--- card and presence queries, static because nothing here holds state ---
    static int internalCardNumber();
    static int usbCardNumber();
    static bool usbPresent();
    static bool btPresent();
    // The stable ALSA card id ("U192k") for a card number, or empty. jack-route-select has the same
    // lookup in shell (card_id_for_number).
    static std::string cardIdForNumber(int card);
    static bool jackdIsOnUsbCard();
    static bool hdmiPortsExist();
    static bool bluealsaPortsExist();

    // PREFERRED_OUTPUT from ~/.config/jack-bridge/devices.conf, falling back to
    // /etc/jack-bridge/devices.conf, falling back to "internal".
    static std::string loadPreferredOutput();
    // The MAC out of BLUETOOTH_DEVICE="...DEV=AA:BB:CC:DD:EE:FF...", or empty.
    static std::string loadBluetoothDeviceMac();

    // "AA:BB:CC:DD:EE:FF" from a BlueZ object path ending in "dev_AA_BB_CC_DD_EE_FF". A string with
    // no '/' in it is assumed to be a MAC already and returned unchanged.
    static std::string macFromBluezObject(const std::string &s);

    // Where jack-route-select is installed. Its absence is reported to the user rather than
    // surfacing as a failed spawn, because "run sudo ./contrib/install.sh" is actionable and
    // "could not execute" is not.
    static const char *kRouteHelper;

    //--- the ~/.asoundrc managed block ----------------------------------
    // Ensures ~/.asoundrc includes the per-user fragments, PRESERVING EVERYTHING OUTSIDE THE
    // MARKERS. Run once at start-up. Writes nothing if the fragment is already there.
    static void ensureAsoundrcBootstrap();

private:
    // True while a Bluetooth switch is in flight, so a second click cannot start another.
    bool mBtRouteActive = false;
    int mBtPollsLeft = 0;
    std::string mBtRouteMac;
    // Owned by the app: it registers this and cancels it. Set to the token addTimer returned, or 0.
    int mBtPollTimer = 0;

    void btRouteBegin(const std::string &mac);
    void btRouteChildExit(int status);
    void btRouteFail(const std::string &message);
    void revertToInternal();
    void applyMixerFor(Output o);

    Output mSelected = Output::Internal;
    std::string mBtSelection; // a BlueZ object path from the Bluetooth page

    // What the poll last saw, so it only acts when something actually differs.
    std::string mPolledOutput;
    bool mPolledUsbPresent = false;

public:
    // The app owns the timer, because only it has the window. The poll flow calls these.
    std::function<int(int intervalMs, std::function<void()>)> addTimer;
    std::function<void(int token)> removeTimer;
};

} // namespace jackbridge

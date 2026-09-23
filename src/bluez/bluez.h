// The BlueZ client: everything mxeq asks of org.bluez, over libdbus-1.
//
// This replaces src/gui_bt.c, which was GDBus. The surface below was enumerated FROM THAT FILE
// rather than from memory -- every method here is a call the GTK build makes today, and nothing it
// makes is missing. The tables in the plan and in bus.h record the signatures; each was checked
// against org.freedesktop.DBus.Introspectable.Introspect on the running daemon and against
// refs/bluez/doc/org.bluez.*.rst before it was written.
//
// FOUR BEHAVIOURS ARE NON-OBVIOUS AND MUST SURVIVE. Each is a comment in the C this replaces:
//
//   1. IF THE ADAPTER IS ALREADY DISCOVERING, StopDiscovery FIRST (gui_bt.c:462-465). BlueZ does
//      not re-emit InterfacesAdded for devices it has already cached, so on a plain StartDiscovery
//      the devices you can already see never appear in the list. Stopping and restarting is what
//      makes them arrive.
//
//   2. SetDiscoveryFilter IS DELIBERATELY NOT CALLED (gui_bt.c:457). It stays uncalled.
//
//   3. UnregisterAgent BEFORE RegisterAgent (bt_agent.c:185). A crashed previous run leaves an
//      agent entry behind and RegisterAgent then fails with "already registered".
//
//   4. EVERY PENDING CALL IS CANCELLED ON SHUTDOWN. g_shutting_down guarded this in the GTK build;
//      here the flag AND the cancellation both exist, because a notify firing into a destroyed
//      panel is the crash that guard was added for.
//
// THREADS: THERE ARE NONE. gui_bt.c pushed every UI update through g_idle_add because a GDBus
// signal could arrive on a worker thread. libdbus here is driven from the X11Window's select() loop
// via dbus_connection_set_watch_functions, so every callback below already runs on the main thread
// and the __gui_bt_*_idle trampolines are gone. That is safe BECAUSE of the loop integration, not
// by luck -- bus.h spells out why the one-descriptor shortcut would not have been.

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

class Bus;

struct BluezDeviceProps {
    std::string path;
    std::string name; // Alias if BlueZ gave one, else Name, else empty
    bool paired = false;
    bool trusted = false;

    // BlueZ's Device1.Connected, VERBATIM -- and it does not mean what a user means by it. BlueZ
    // sets it on the kernel's MGMT_EV_DEVICE_CONNECTED (src/adapter.c, connected_callback ->
    // device_add_connection), which is the baseband ACL link. Pairing cannot happen without that
    // link, so Connected goes true DURING Pair and stays true for the few seconds the link idles
    // afterwards; bluetoothctl shows "Connected: yes" at the same moments. For a speaker that is a
    // radio link with no audio on it. Use connectedForUse() for anything a user sees.
    bool connected = false;

    // The device advertises the A2DP Audio Sink service (0000110b-...): it can play what we send.
    // Read from Device1.UUIDs. This is what makes a device a candidate for "Set as Output".
    bool audioSink = false;

    // A2DP IS ACTUALLY UP: BlueZ holds a org.bluez.MediaTransport1 for this device. BlueZ creates
    // one in set_configuration() when the stream is configured against an endpoint (bluealsa's)
    // and destroys it in clear_configuration() when A2DP goes down (profiles/audio/media.c); it
    // exists in state "idle" too, so it means connected, not merely streaming.
    bool audioConnected = false;

    // What "Connected" means to a person. For a device that can take our audio, it is connected
    // when the audio is -- a paired speaker whose link is up only because pairing just finished is
    // NOT connected in any sense that helps anyone. For anything else (a game controller) BlueZ's
    // link-level Connected is the only signal there is, and for HID it is the right one: the link
    // IS the input connection.
    bool connectedForUse() const
    {
        return audioSink ? audioConnected : connected;
    }
};

class Bluez
{
public:
    Bluez();
    ~Bluez();

    Bluez(const Bluez &) = delete;
    Bluez &operator=(const Bluez &) = delete;

    //--- events, all delivered on the main thread ----------------------
    // A device appeared (InterfacesAdded) or its properties changed (PropertiesChanged on
    // org.bluez.Device1). The same callback serves both: the panel upserts by path either way, and
    // giving them separate callbacks would mean two code paths that have to agree.
    std::function<void(const BluezDeviceProps &)> onDevice;

    // A device went away (InterfacesRemoved).
    std::function<void(const std::string &path)> onDeviceRemoved;

    // The adapter's Powered, Discoverable or Discovering changed, or the adapter itself appeared or
    // went away. The panel re-reads the getters below.
    std::function<void()> onAdapterChanged;

    // An asynchronous operation finished. `message` carries BlueZ's own error text on failure,
    // wrapped in the hints compose_hint_message() added -- see hintMessage().
    std::function<void(bool success, const std::string &message)> onOperation;

    // A pairing agent request that wants a human: a passkey to confirm, or a PIN to read out. The
    // GTK build had nowhere to show these and auto-accepted silently; there is a panel now. If this
    // is not set, the agent auto-accepts exactly as before.
    std::function<void(const std::string &prompt)> onAgentPrompt;

    //--- lifecycle ------------------------------------------------------
    // Connects to the system bus, finds the adapter, registers the agent and subscribes to the four
    // signals. Returns false having warned: no bus, or no adapter. NOT FATAL TO THE CALLER -- mxeq
    // must start and stay up with bluetoothd stopped, with the Bluetooth page inert.
    // A false return means the Bluetooth page will be inert AS THINGS STAND, not for ever: the
    // connection stays open if the bus was reachable, and an adapter plugged in later arrives on
    // InterfacesAdded and brings the page to life through onAdapterChanged.
    bool open();
    void close();

    bool isOpen() const;
    // True when there is an adapter to talk to. The Bluetooth page's controls are gated on this.
    bool adapterReady() const;

    // The descriptors libdbus wants waited on. The set CHANGES AT RUNTIME -- libdbus adds a watch
    // during authentication, and toggles watches on and off as its outgoing queue fills and drains
    // -- so the app re-reads this whenever onWatchesChanged fires rather than registering once.
    //
    // A descriptor can be wanted for WRITING as well as reading; bus.h explains when and why that
    // is safe against spinning the event loop.
    std::function<void()> onWatchesChanged;
    struct Watch {
        int fd;
        bool write;
    };
    std::vector<Watch> watchDescriptors() const;
    // One watched descriptor became ready. Dispatches until DBUS_DISPATCH_COMPLETE, because one
    // read can carry several messages.
    void handleWatch(int fd, bool writable);
    // libdbus's own timeouts. Called on a fixed tick; this is the half of the loop integration
    // that makes a call which times out with no further traffic on the socket actually complete.
    void handleTimeouts();

    //--- adapter --------------------------------------------------------
    bool powered() const;
    bool discoverable() const;
    bool discovering() const;

    void setPowered(bool on);
    void setDiscoverable(bool on);

    // Rule 1 above lives in here: if the adapter is already discovering, this stops first.
    void startDiscovery();
    void stopDiscovery();

    //--- devices --------------------------------------------------------
    // The list BlueZ already knows about, from ObjectManager.GetManagedObjects.
    //
    // CALLED AT open() AND AGAIN ON EVERY StartDiscovery, which is not redundant: it is the other
    // half of rule 1 above. BlueZ emits InterfacesAdded only for devices it did not already have
    // cached, so on a machine that has paired before, a Scan produces no signals at all for the
    // devices the user most wants to see. gui_bt.c calls gui_bt_populate_existing_devices() from
    // inside gui_bt_start_discovery() for exactly that reason; startDiscovery() below does the
    // same by reporting each of these through onDevice.
    std::vector<BluezDeviceProps> knownDevices();

    // All asynchronous, all reporting through onOperation. The GTK build made these async precisely
    // so the UI never blocks on D-Bus: pairing can take ten seconds and Connect can take longer.
    // The timeouts are the GTK build's own -- 20 s for Pair and Connect, 15 s for Trusted.
    //
    // pair() STOPS DISCOVERY FIRST, best-effort and ignoring the result (gui_bt.c:670-697). An
    // adapter that is still scanning pairs unreliably, and a failure here is not a reason not to
    // try the pairing.
    void pair(const std::string &path);
    void setTrusted(const std::string &path, bool trusted);

    // connect() IS TWO CALLS, NOT ONE. It tries Device1.ConnectProfile with the A2DP Sink UUID
    // 0000110b-0000-1000-8000-00805f9b34fb first and falls back to the generic Device1.Connect
    // once if that fails (gui_bt.c:715-805). That ordering is the whole point of this program:
    // asking for the audio sink profile by name is what gets a headset onto A2DP rather than onto
    // whatever profile BlueZ would otherwise pick, and bluealsa:playback_* only appears for A2DP.
    // The fallback exists because a device with no A2DP sink -- and some that report it late --
    // answers ConnectProfile with an error and connects fine to a plain Connect.
    void connect(const std::string &path);

    // Synchronous, because both are quick and the caller acts on the answer immediately.
    void removeDevice(const std::string &path);
    // A fresh read of one device, with audioConnected filled from the transports this client is
    // tracking. False if the device could not be read.
    bool deviceState(const std::string &path, BluezDeviceProps *out);

    // The hint block compose_hint_message() appended to every failure (gui_bt.c:603). Kept verbatim
    // in substance: these three lines are what a user who cannot pair actually needs, and they were
    // written from support questions rather than invented.
    static std::string hintMessage(const std::string &prefix, const std::string &detail);

private:
    struct Impl;
    Impl *mImpl = nullptr;
};

} // namespace jackbridge

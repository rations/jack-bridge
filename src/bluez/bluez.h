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
    bool connected = false;
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
    bool open();
    void close();

    bool isOpen() const;
    // True when there is an adapter to talk to. The Bluetooth page's controls are gated on this.
    bool adapterReady() const;

    // The descriptors libdbus wants waited on, and the callback to run when one is readable. The
    // set CHANGES AT RUNTIME -- libdbus adds a second watch during authentication -- so the app
    // re-reads this whenever onWatchesChanged fires rather than registering once.
    std::function<void()> onWatchesChanged;
    std::vector<int> watchDescriptors() const;
    // Called when any watched descriptor is readable. Dispatches until DBUS_DISPATCH_COMPLETE.
    void handleWatches();
    // libdbus's own timeouts, in milliseconds, soonest first. Empty when it wants none.
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
    // The pre-populated list, from ObjectManager.GetManagedObjects. Called once at open().
    std::vector<BluezDeviceProps> knownDevices();

    // All asynchronous, all reporting through onOperation. The GTK build made these async precisely
    // so the UI never blocks on D-Bus: pairing can take ten seconds and Connect can take longer.
    void pair(const std::string &path);
    void setTrusted(const std::string &path, bool trusted);
    void connect(const std::string &path);

    // Synchronous, because both are quick and the caller acts on the answer immediately.
    void removeDevice(const std::string &path);
    bool deviceState(const std::string &path, bool *paired, bool *trusted, bool *connected);

    // The hint block compose_hint_message() appended to every failure (gui_bt.c:603). Kept verbatim
    // in substance: these three lines are what a user who cannot pair actually needs, and they were
    // written from support questions rather than invented.
    static std::string hintMessage(const std::string &prefix, const std::string &detail);

private:
    struct Impl;
    Impl *mImpl = nullptr;
};

} // namespace jackbridge

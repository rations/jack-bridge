// See bluez.h. The BlueZ client proper: what mxeq asks of org.bluez, over libdbus-1.
//
// This replaces src/gui_bt.c, which was GDBus. Every call below was read out of that file rather
// than remembered, and every signature was checked against
// org.freedesktop.DBus.Introspectable.Introspect on the running 5.82 daemon and against
// refs/bluez/bluez-5.82/doc/org.bluez.*.rst before it was written.
//
// THE THREAD HOP IS GONE AND THAT IS SAFE BY CONSTRUCTION. gui_bt.c pushed every UI update through
// g_idle_add because a GDBus signal could arrive on a worker thread. libdbus here is driven from
// the X11Window's select() loop through dbus_connection_set_watch_functions, so every callback in
// this file already runs on the main thread and the __gui_bt_*_idle trampolines are gone with it.
// That is safe BECAUSE of the loop integration, not by luck -- bus.h sets out why the
// one-descriptor shortcut would not have been.

#include "bluez.h"

#include "agent.h"
#include "bus.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

namespace jackbridge
{

namespace
{

const char *const kRoot = "/";
const char *const kObjectManager = "org.freedesktop.DBus.ObjectManager";
const char *const kProperties = "org.freedesktop.DBus.Properties";
const char *const kAdapter1 = "org.bluez.Adapter1";
const char *const kDevice1 = "org.bluez.Device1";
// One per configured A2DP stream -- see BluezDeviceProps::audioConnected for what its existence
// means and where in BlueZ that is decided.
const char *const kMediaTransport1 = "org.bluez.MediaTransport1";

// The A2DP Sink profile. A headset is an audio sink, and asking for this UUID by name is what puts
// it on A2DP rather than on HFP -- which matters because bluealsa only offers playback_1/2 for an
// A2DP transport. gui_bt.c:794 carries the same constant.
const char *const kA2dpSinkUuid = "0000110b-0000-1000-8000-00805f9b34fb";

// The timeouts the GDBus build used, kept because each was chosen against a real failure: a
// pairing that takes ten seconds is normal, a property read that takes two is already wrong.
const int kTimeoutQuick = 2000;
const int kTimeoutDiscovery = 5000;
const int kTimeoutTrust = 15000;
const int kTimeoutPair = 20000;
const int kTimeoutConnect = 20000;

// The adapter an object path belongs to: /org/bluez/hci0/dev_AA_.. -> /org/bluez/hci0. gui_bt.c
// truncates at "/dev_" in three places and falls back to a hard-coded /org/bluez/hci0; here the
// fallback is the adapter we actually found, which is better on a machine with two of them.
std::string adapterOf(const std::string &devicePath, const std::string &fallback)
{
    const size_t p = devicePath.find("/dev_");
    if (p == std::string::npos)
        return fallback;
    return devicePath.substr(0, p);
}

} // namespace

//------------------------------------------------------------------------
struct Bluez::Impl {
    Bus bus;
    Agent agent;

    std::string adapter; // empty when there is none
    bool powered = false;
    bool discoverable = false;
    bool discovering = false;

    Bluez *owner = nullptr;

    // Walk a{oa{sa{sv}}} from GetManagedObjects. `fn` is given each object path and its interface
    // dictionary iterator positioned at the start.
    void forEachManagedObject(DBusMessage *reply,
                              const std::function<void(const std::string &path,
                                                       DBusMessageIter *ifaces)> &fn);

    // Read Paired/Trusted/Connected/Alias/Name out of an a{sv} the iterator is positioned on.
    static void readDeviceProps(DBusMessageIter *dict, BluezDeviceProps *out);

    bool findAdapter();
    void refreshAdapterState();
    bool getBool(const std::string &path, const char *iface, const char *name, bool *out);
    bool setBool(const std::string &path, const char *iface, const char *name, bool value,
                 std::string *error);

    bool handleSignal(DBusMessage *m);
    void reportDevice(const std::string &path, const BluezDeviceProps &p);

    // Re-read a device in full and report it. What a PropertiesChanged on Device1 does, and what a
    // transport appearing or disappearing does too, since that changes the device's row without
    // any Device1 property changing.
    void refreshDevice(const std::string &path);

    // TRANSPORT PATH -> DEVICE PATH, for every MediaTransport1 BlueZ holds. Keyed on the transport
    // because InterfacesRemoved names only the path that went, and by then the transport's Device
    // property cannot be read.
    std::map<std::string, std::string> transports;
    bool hasTransport(const std::string &devicePath) const;
    // The device a transport belongs to: its Device property (org.bluez.MediaTransport.rst), and
    // failing that the dev_XX_XX_XX_XX_XX_XX component of its own path, which the same document
    // gives as the object path's shape.
    static std::string transportDevice(const std::string &transportPath, DBusMessageIter *props);
};

//------------------------------------------------------------------------
Bluez::Bluez() : mImpl(new Impl)
{
    mImpl->owner = this;
}

Bluez::~Bluez()
{
    close();
    delete mImpl;
}

//------------------------------------------------------------------------
void Bluez::Impl::forEachManagedObject(
    DBusMessage *reply,
    const std::function<void(const std::string &, DBusMessageIter *)> &fn)
{
    DBusMessageIter it;
    if (!dbus_message_iter_init(reply, &it))
        return;
    if (dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_ARRAY)
        return;

    DBusMessageIter outer;
    dbus_message_iter_recurse(&it, &outer);
    while (dbus_message_iter_get_arg_type(&outer) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&outer, &entry);

        std::string path;
        if (readString(&entry, &path)) {
            dbus_message_iter_next(&entry);
            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_ARRAY) {
                DBusMessageIter ifaces;
                dbus_message_iter_recurse(&entry, &ifaces);
                fn(path, &ifaces);
            }
        }
        dbus_message_iter_next(&outer);
    }
}

void Bluez::Impl::readDeviceProps(DBusMessageIter *dict, BluezDeviceProps *out)
{
    // ALIAS WINS OVER NAME. BlueZ's Alias is the name the user (or BlueZ) settled on and Name is
    // whatever the device advertises; a device that renames itself mid-session should not rename
    // its row. gui_bt.c got this by accident, from whichever key the dictionary yielded first;
    // here it is explicit.
    std::string name;
    bool haveAlias = false;

    while (dbus_message_iter_get_arg_type(dict) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter kv;
        dbus_message_iter_recurse(dict, &kv);

        std::string key;
        if (readString(&kv, &key)) {
            dbus_message_iter_next(&kv);
            if (key == "Paired")
                readVariantBool(&kv, &out->paired);
            else if (key == "Trusted")
                readVariantBool(&kv, &out->trusted);
            else if (key == "Connected")
                readVariantBool(&kv, &out->connected);
            else if (key == "UUIDs") {
                std::vector<std::string> uuids;
                if (readVariantStringArray(&kv, &uuids)) {
                    out->audioSink = std::any_of(uuids.begin(), uuids.end(), [](std::string u) {
                        // BlueZ writes these lower-case; compared case-blind anyway, because a
                        // UUID's case carries no meaning and nothing promises it stays that way.
                        std::transform(u.begin(), u.end(), u.begin(), [](char c) {
                            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
                        });
                        return u == kA2dpSinkUuid;
                    });
                }
            }
            else if (key == "Alias") {
                std::string v;
                if (readVariantString(&kv, &v) && !v.empty()) {
                    name = v;
                    haveAlias = true;
                }
            } else if (key == "Name" && !haveAlias) {
                std::string v;
                if (readVariantString(&kv, &v))
                    name = v;
            }
        }
        dbus_message_iter_next(dict);
    }

    if (!name.empty())
        out->name = name;
}

//------------------------------------------------------------------------
bool Bluez::Impl::findAdapter()
{
    // NO HARD-CODED hci0. A machine can have a second adapter, or none, or its only one on a USB
    // dongle that was not plugged in when the program started.
    DBusMessage *call = Bus::newCall(kRoot, kObjectManager, "GetManagedObjects");
    std::string error;
    Msg reply = bus.callSync(call, kTimeoutDiscovery, &error);
    if (!reply) {
        fprintf(stderr, "jack-bridge: GetManagedObjects failed: %s\n", error.c_str());
        adapter.clear();
        return false;
    }

    std::string found;
    forEachManagedObject(reply.get(), [&](const std::string &path, DBusMessageIter *ifaces) {
        if (!found.empty())
            return;
        while (dbus_message_iter_get_arg_type(ifaces) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter kv;
            dbus_message_iter_recurse(ifaces, &kv);
            std::string iface;
            if (readString(&kv, &iface) && iface == kAdapter1) {
                found = path;
                return;
            }
            dbus_message_iter_next(ifaces);
        }
    });

    adapter = found;
    if (adapter.empty())
        fprintf(stderr, "jack-bridge: no org.bluez.Adapter1 found\n");
    return !adapter.empty();
}

bool Bluez::Impl::getBool(const std::string &path, const char *iface, const char *name, bool *out)
{
    DBusMessage *call = Bus::newCall(path.c_str(), kProperties, "Get");
    if (!call)
        return false;
    dbus_message_append_args(call, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &name,
                             DBUS_TYPE_INVALID);
    std::string error;
    Msg reply = bus.callSync(call, kTimeoutQuick, &error);
    if (!reply)
        return false;

    DBusMessageIter it;
    if (!dbus_message_iter_init(reply.get(), &it))
        return false;
    return readVariantBool(&it, out);
}

bool Bluez::Impl::setBool(const std::string &path, const char *iface, const char *name, bool value,
                          std::string *error)
{
    DBusMessage *call = Bus::newCall(path.c_str(), kProperties, "Set");
    if (!call) {
        if (error)
            *error = "out of memory";
        return false;
    }
    if (!appendPropertySet(call, iface, name, value)) {
        dbus_message_unref(call);
        if (error)
            *error = "could not build the property value";
        return false;
    }
    Msg reply = bus.callSync(call, kTimeoutQuick, error);
    return static_cast<bool>(reply);
}

void Bluez::Impl::refreshAdapterState()
{
    if (adapter.empty()) {
        powered = discoverable = discovering = false;
        return;
    }
    // One GetAll rather than three Gets: refresh_adapter_state() in the GDBus build did the same,
    // and the three properties are read together on every adapter signal.
    DBusMessage *call = Bus::newCall(adapter.c_str(), kProperties, "GetAll");
    if (!call)
        return;
    const char *iface = kAdapter1;
    dbus_message_append_args(call, DBUS_TYPE_STRING, &iface, DBUS_TYPE_INVALID);
    std::string error;
    Msg reply = bus.callSync(call, kTimeoutQuick, &error);
    if (!reply)
        return;

    DBusMessageIter it;
    if (!dbus_message_iter_init(reply.get(), &it))
        return;
    if (dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_ARRAY)
        return;

    DBusMessageIter dict;
    dbus_message_iter_recurse(&it, &dict);
    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter kv;
        dbus_message_iter_recurse(&dict, &kv);
        std::string key;
        if (readString(&kv, &key)) {
            dbus_message_iter_next(&kv);
            if (key == "Powered")
                readVariantBool(&kv, &powered);
            else if (key == "Discoverable")
                readVariantBool(&kv, &discoverable);
            else if (key == "Discovering")
                readVariantBool(&kv, &discovering);
        }
        dbus_message_iter_next(&dict);
    }
}

void Bluez::Impl::reportDevice(const std::string &path, const BluezDeviceProps &p)
{
    if (owner->onDevice) {
        BluezDeviceProps copy = p;
        copy.path = path;
        copy.audioConnected = hasTransport(path);
        // A name is never empty by the time the panel sees one: an unnamed device still has to be
        // clickable, and its address is what the user can match against the thing in their hand.
        if (copy.name.empty()) {
            const size_t slash = path.rfind('/');
            copy.name = slash == std::string::npos ? path : path.substr(slash + 1);
        }
        owner->onDevice(copy);
    }
}

void Bluez::Impl::refreshDevice(const std::string &path)
{
    DBusMessage *call = Bus::newCall(path.c_str(), kProperties, "GetAll");
    if (!call)
        return;
    const char *dev = kDevice1;
    dbus_message_append_args(call, DBUS_TYPE_STRING, &dev, DBUS_TYPE_INVALID);
    std::string error;
    Msg reply = bus.callSync(call, kTimeoutQuick, &error);
    if (!reply)
        return; // the device went away between the signal and the read
    DBusMessageIter rit;
    if (!dbus_message_iter_init(reply.get(), &rit) ||
        dbus_message_iter_get_arg_type(&rit) != DBUS_TYPE_ARRAY)
        return;
    DBusMessageIter props;
    dbus_message_iter_recurse(&rit, &props);
    BluezDeviceProps full;
    readDeviceProps(&props, &full);
    reportDevice(path, full);
}

bool Bluez::Impl::hasTransport(const std::string &devicePath) const
{
    for (const auto &t : transports) {
        if (t.second == devicePath)
            return true;
    }
    return false;
}

std::string Bluez::Impl::transportDevice(const std::string &transportPath, DBusMessageIter *props)
{
    if (props) {
        while (dbus_message_iter_get_arg_type(props) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter kv;
            dbus_message_iter_recurse(props, &kv);
            std::string key;
            if (readString(&kv, &key) && key == "Device") {
                dbus_message_iter_next(&kv);
                std::string dev;
                if (readVariantString(&kv, &dev) && !dev.empty())
                    return dev;
            }
            dbus_message_iter_next(props);
        }
    }
    const size_t d = transportPath.find("/dev_");
    if (d == std::string::npos)
        return std::string();
    const size_t end = transportPath.find('/', d + 1);
    return transportPath.substr(0, end);
}

//------------------------------------------------------------------------
bool Bluez::Impl::handleSignal(DBusMessage *m)
{
    if (dbus_message_get_type(m) != DBUS_MESSAGE_TYPE_SIGNAL)
        return false;

    const char *iface = dbus_message_get_interface(m);
    const char *member = dbus_message_get_member(m);
    if (!iface || !member)
        return false;

    if (strcmp(iface, kObjectManager) == 0 && strcmp(member, "InterfacesAdded") == 0) {
        // (o a{sa{sv}})
        DBusMessageIter it;
        if (!dbus_message_iter_init(m, &it))
            return false;
        std::string path;
        if (!readString(&it, &path))
            return false;
        dbus_message_iter_next(&it);
        if (dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_ARRAY)
            return false;

        DBusMessageIter ifaces;
        dbus_message_iter_recurse(&it, &ifaces);
        while (dbus_message_iter_get_arg_type(&ifaces) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter kv;
            dbus_message_iter_recurse(&ifaces, &kv);
            std::string name;
            if (readString(&kv, &name)) {
                dbus_message_iter_next(&kv);
                if (name == kDevice1 && dbus_message_iter_get_arg_type(&kv) == DBUS_TYPE_ARRAY) {
                    // FILTERED ON Device1, which the GDBus build did not do. gui_bt.c's handler is
                    // deliberately "extremely conservative": it parses nothing and adds a row for
                    // EVERY object BlueZ announces, so an adapter, a battery provider or a media
                    // transport each become a row whose only label is the tail of its object path
                    // and whose Device1 GetAll then fails. Reading the interface name costs one
                    // comparison and is what keeps those out of the list.
                    DBusMessageIter props;
                    dbus_message_iter_recurse(&kv, &props);
                    BluezDeviceProps p;
                    readDeviceProps(&props, &p);
                    reportDevice(path, p);
                } else if (name == kMediaTransport1) {
                    // A2DP came up for a device. Nothing on Device1 changes when it does, so
                    // without this the row would never learn its audio is connected.
                    DBusMessageIter props;
                    DBusMessageIter *pp = nullptr;
                    if (dbus_message_iter_get_arg_type(&kv) == DBUS_TYPE_ARRAY) {
                        dbus_message_iter_recurse(&kv, &props);
                        pp = &props;
                    }
                    const std::string dev = transportDevice(path, pp);
                    if (!dev.empty()) {
                        transports[path] = dev;
                        refreshDevice(dev);
                    }
                } else if (name == kAdapter1) {
                    // An adapter appeared -- a USB dongle plugged in, or bluetoothd starting after
                    // this program did. The page comes to life without a restart.
                    if (adapter.empty()) {
                        adapter = path;
                        refreshAdapterState();
                        if (owner->onAdapterChanged)
                            owner->onAdapterChanged();
                    }
                }
            }
            dbus_message_iter_next(&ifaces);
        }
        return false; // a filter that consumed signals would starve any other handler
    }

    if (strcmp(iface, kObjectManager) == 0 && strcmp(member, "InterfacesRemoved") == 0) {
        // (o as)
        DBusMessageIter it;
        if (!dbus_message_iter_init(m, &it))
            return false;
        std::string path;
        if (!readString(&it, &path))
            return false;

        // A transport going is A2DP going down. It is not a device, so it must not reach
        // onDeviceRemoved -- that would be harmless today only because no row has its path.
        const auto t = transports.find(path);
        if (t != transports.end()) {
            const std::string dev = t->second;
            transports.erase(t);
            refreshDevice(dev);
            return false;
        }

        if (path == adapter) {
            adapter.clear();
            refreshAdapterState();
            if (owner->onAdapterChanged)
                owner->onAdapterChanged();
        }
        if (owner->onDeviceRemoved)
            owner->onDeviceRemoved(path);
        return false;
    }

    if (strcmp(iface, kProperties) == 0 && strcmp(member, "PropertiesChanged") == 0) {
        // (s a{sv} as). The match rules filter on arg0, but a filter function sees every message
        // that reaches the connection, so the interface is checked here too.
        DBusMessageIter it;
        if (!dbus_message_iter_init(m, &it))
            return false;
        std::string changed;
        if (!readString(&it, &changed))
            return false;

        const char *path = dbus_message_get_path(m);
        if (!path)
            return false;

        if (changed == kAdapter1) {
            if (adapter.empty() || adapter == path) {
                if (adapter.empty())
                    adapter = path;
                refreshAdapterState();
                if (owner->onAdapterChanged)
                    owner->onAdapterChanged();
            }
            return false;
        }

        if (changed == kDevice1) {
            // The signal carries only the properties that CHANGED, so the row is refreshed from a
            // full GetAll rather than patched from the delta -- which is what gui_bt.c's
            // update_device_row_state() does, and for the same reason: a row patched from deltas
            // is only as correct as the first signal it ever saw.
            refreshDevice(path);
            return false;
        }
    }

    return false;
}

//------------------------------------------------------------------------
bool Bluez::open()
{
    // BEFORE bus.open(), NOT AFTER. dbus_connection_set_watch_functions calls the add function
    // once for every watch that already exists, which on a fresh connection is the socket itself --
    // so a handler installed afterwards never hears about the first descriptor, and the only
    // reason that is survivable is that the caller also calls back for the set explicitly once.
    // Relying on that is relying on the caller's ordering; this does not.
    mImpl->bus.onWatchesChanged = [this] {
        if (onWatchesChanged)
            onWatchesChanged();
    };
    mImpl->bus.onMessage = [this](DBusMessage *m) { return mImpl->handleSignal(m); };

    if (!mImpl->bus.open())
        return false;

    // THE arg0 ON THE TWO PropertiesChanged RULES IS LOAD-BEARING. Without it every property
    // change on the whole system bus -- systemd units, NetworkManager, logind -- is delivered to
    // this process and filtered in userspace. GDBus built these rules from its subscribe()
    // arguments; here they are strings, so getting them right is this program's job.
    mImpl->bus.addMatch("type='signal',sender='org.bluez',"
                        "interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'");
    mImpl->bus.addMatch("type='signal',sender='org.bluez',"
                        "interface='org.freedesktop.DBus.ObjectManager',"
                        "member='InterfacesRemoved'");
    mImpl->bus.addMatch("type='signal',sender='org.bluez',"
                        "interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',"
                        "arg0='org.bluez.Device1'");
    mImpl->bus.addMatch("type='signal',sender='org.bluez',"
                        "interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',"
                        "arg0='org.bluez.Adapter1'");

    const bool haveAdapter = mImpl->findAdapter();
    mImpl->refreshAdapterState();

    // THE AGENT IS REGISTERED EVEN WITH NO ADAPTER. bt_agent.c registers it from gui_bt_init()
    // before anything has looked for an adapter, and an agent registered early is an agent that is
    // already in place when a dongle is plugged in.
    mImpl->agent.onPrompt = [this](const std::string &prompt) {
        if (onAgentPrompt)
            onAgentPrompt(prompt);
    };
    mImpl->agent.install(mImpl->bus);

    return haveAdapter;
}

void Bluez::close()
{
    if (!mImpl->bus.isOpen())
        return;
    // The agent first: it wants to send UnregisterAgent, which needs the connection still open.
    mImpl->agent.remove();
    mImpl->bus.close();
    mImpl->adapter.clear();
    mImpl->powered = mImpl->discoverable = mImpl->discovering = false;
}

bool Bluez::isOpen() const
{
    return mImpl->bus.isOpen();
}

bool Bluez::adapterReady() const
{
    return mImpl->bus.isOpen() && !mImpl->adapter.empty();
}

//------------------------------------------------------------------------
std::vector<Bluez::Watch> Bluez::watchDescriptors() const
{
    std::vector<Watch> out;
    for (const Bus::WatchFd &w : mImpl->bus.watches())
        out.push_back({w.fd, w.write});
    return out;
}

void Bluez::handleWatch(int fd, bool writable)
{
    mImpl->bus.handleFd(fd, writable);
}

void Bluez::handleTimeouts()
{
    mImpl->bus.handleTimeouts();
}

//------------------------------------------------------------------------
bool Bluez::powered() const
{
    return mImpl->powered;
}

bool Bluez::discoverable() const
{
    return mImpl->discoverable;
}

bool Bluez::discovering() const
{
    return mImpl->discovering;
}

void Bluez::setPowered(bool on)
{
    if (!adapterReady())
        return;
    std::string error;
    if (!mImpl->setBool(mImpl->adapter, kAdapter1, "Powered", on, &error)) {
        if (onOperation)
            onOperation(false, hintMessage("Could not power the adapter", error));
        return;
    }
    mImpl->refreshAdapterState();
    if (onAdapterChanged)
        onAdapterChanged();
}

void Bluez::setDiscoverable(bool on)
{
    if (!adapterReady()) {
        if (onOperation)
            onOperation(false, hintMessage("No Bluetooth adapter", "nothing to make discoverable"));
        return;
    }

    // DISCOVERABLE AND PAIRABLE MOVE TOGETHER, and gui_bt.c's comment says why: Discoverable
    // controls whether other devices can FIND this one in a scan, and Pairable controls whether
    // they can bond with it at all. Leaving Pairable on while Discoverable is off means anyone who
    // already knows the address can still pair. The toggle says "Discoverable" and has to mean it.
    std::string error;
    bool ok = mImpl->setBool(mImpl->adapter, kAdapter1, "Discoverable", on, &error);
    if (ok)
        ok = mImpl->setBool(mImpl->adapter, kAdapter1, "Pairable", on, &error);

    mImpl->refreshAdapterState();
    if (!ok && onOperation)
        onOperation(false, hintMessage("Could not change Discoverable", error));
    if (onAdapterChanged)
        onAdapterChanged();
}

void Bluez::startDiscovery()
{
    if (!adapterReady()) {
        if (onOperation)
            onOperation(false, hintMessage("No Bluetooth adapter", "cannot scan"));
        return;
    }

    // Discovery needs Powered=true, and a user who presses Scan means "scan", not "tell me the
    // adapter is off". ensure_adapter_powered() did this in the GDBus build.
    if (!mImpl->powered) {
        std::string error;
        if (!mImpl->setBool(mImpl->adapter, kAdapter1, "Powered", true, &error)) {
            if (onOperation)
                onOperation(false, hintMessage("Could not power the adapter", error));
            return;
        }
        mImpl->refreshAdapterState();
    }

    // RULE 1: IF IT IS ALREADY DISCOVERING, STOP FIRST. BlueZ does not re-emit InterfacesAdded for
    // devices it has already cached, so a StartDiscovery on an adapter that is already discovering
    // is a no-op that produces no signals at all -- which the user experiences as "Scan does
    // nothing until I press Stop and then Scan". Best-effort; an error here is not a reason not to
    // try the start. gui_bt.c:462-465.
    bool already = false;
    if (mImpl->getBool(mImpl->adapter, kAdapter1, "Discovering", &already) && already) {
        DBusMessage *stop = Bus::newCall(mImpl->adapter.c_str(), kAdapter1, "StopDiscovery");
        std::string ignored;
        mImpl->bus.callSync(stop, kTimeoutDiscovery, &ignored);
    }

    // RULE 2: SetDiscoveryFilter IS NOT CALLED. gui_bt.c:457 records that building the container
    // variant it needs tripped GVariant refcount assertions on some GLib versions, and that plain
    // StartDiscovery finds classic audio devices perfectly well. The GLib reason is gone with
    // GLib, but the second half still holds and an unfiltered scan is what this program wants.

    DBusMessage *call = Bus::newCall(mImpl->adapter.c_str(), kAdapter1, "StartDiscovery");
    std::string error;
    Msg reply = mImpl->bus.callSync(call, kTimeoutDiscovery, &error);
    if (!reply) {
        if (onOperation)
            onOperation(false, hintMessage("Scan failed", error));
        return;
    }

    // And the other half of rule 1: surface what BlueZ already knows, because those devices will
    // never arrive as signals.
    for (const BluezDeviceProps &p : knownDevices())
        mImpl->reportDevice(p.path, p);

    mImpl->refreshAdapterState();
    if (onAdapterChanged)
        onAdapterChanged();
}

void Bluez::stopDiscovery()
{
    if (!adapterReady())
        return;
    DBusMessage *call = Bus::newCall(mImpl->adapter.c_str(), kAdapter1, "StopDiscovery");
    std::string error;
    Msg reply = mImpl->bus.callSync(call, kTimeoutDiscovery, &error);
    if (!reply && onOperation)
        onOperation(false, hintMessage("Stop failed", error));

    mImpl->refreshAdapterState();
    if (onAdapterChanged)
        onAdapterChanged();
}

//------------------------------------------------------------------------
std::vector<BluezDeviceProps> Bluez::knownDevices()
{
    std::vector<BluezDeviceProps> out;
    if (!mImpl->bus.isOpen())
        return out;

    DBusMessage *call = Bus::newCall(kRoot, kObjectManager, "GetManagedObjects");
    std::string error;
    Msg reply = mImpl->bus.callSync(call, kTimeoutDiscovery, &error);
    if (!reply) {
        fprintf(stderr, "jack-bridge: GetManagedObjects failed: %s\n", error.c_str());
        return out;
    }

    // TWO PASSES OVER ONE REPLY. The dictionary is in no promised order, and a speaker already
    // playing when this program starts has its transport somewhere in it -- possibly after the
    // device. Collecting every transport first is what lets the device pass say, correctly, that
    // its audio is connected. This is also the one authoritative moment for the transport table,
    // so it is rebuilt here rather than patched.
    mImpl->transports.clear();
    mImpl->forEachManagedObject(reply.get(), [&](const std::string &path,
                                                 DBusMessageIter *ifaces) {
        while (dbus_message_iter_get_arg_type(ifaces) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter kv;
            dbus_message_iter_recurse(ifaces, &kv);
            std::string name;
            if (readString(&kv, &name) && name == kMediaTransport1) {
                dbus_message_iter_next(&kv);
                DBusMessageIter props;
                DBusMessageIter *pp = nullptr;
                if (dbus_message_iter_get_arg_type(&kv) == DBUS_TYPE_ARRAY) {
                    dbus_message_iter_recurse(&kv, &props);
                    pp = &props;
                }
                const std::string dev = Impl::transportDevice(path, pp);
                if (!dev.empty())
                    mImpl->transports[path] = dev;
            }
            dbus_message_iter_next(ifaces);
        }
    });

    mImpl->forEachManagedObject(reply.get(), [&](const std::string &path,
                                                 DBusMessageIter *ifaces) {
        while (dbus_message_iter_get_arg_type(ifaces) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter kv;
            dbus_message_iter_recurse(ifaces, &kv);
            std::string name;
            if (readString(&kv, &name)) {
                dbus_message_iter_next(&kv);
                if (name == kDevice1 && dbus_message_iter_get_arg_type(&kv) == DBUS_TYPE_ARRAY) {
                    DBusMessageIter props;
                    dbus_message_iter_recurse(&kv, &props);
                    BluezDeviceProps p;
                    p.path = path;
                    Impl::readDeviceProps(&props, &p);
                    p.audioConnected = mImpl->hasTransport(path);
                    if (p.name.empty()) {
                        const size_t slash = path.rfind('/');
                        p.name = slash == std::string::npos ? path : path.substr(slash + 1);
                    }
                    out.push_back(std::move(p));
                }
            }
            dbus_message_iter_next(ifaces);
        }
    });

    return out;
}

//------------------------------------------------------------------------
void Bluez::pair(const std::string &path)
{
    if (!mImpl->bus.isOpen() || path.empty())
        return;

    // STOP DISCOVERY FIRST, best-effort. An adapter that is still scanning pairs unreliably, and
    // gui_bt.c:670-697 does the same, ignoring the result.
    {
        const std::string ad = adapterOf(path, mImpl->adapter);
        if (!ad.empty()) {
            DBusMessage *stop = Bus::newCall(ad.c_str(), kAdapter1, "StopDiscovery");
            std::string ignored;
            mImpl->bus.callSync(stop, kTimeoutQuick, &ignored);
            mImpl->refreshAdapterState();
        }
    }

    DBusMessage *call = Bus::newCall(path.c_str(), kDevice1, "Pair");
    if (!mImpl->bus.callAsync(call, kTimeoutPair,
                              [this](bool ok, const std::string &error, DBusMessage *) {
                                  if (onOperation)
                                      onOperation(ok, ok ? std::string()
                                                         : hintMessage("Pair failed", error));
                              })) {
        if (onOperation)
            onOperation(false, hintMessage("Pair failed", "could not send the request"));
    }
    if (onAdapterChanged)
        onAdapterChanged();
}

void Bluez::setTrusted(const std::string &path, bool trusted)
{
    if (!mImpl->bus.isOpen() || path.empty())
        return;

    DBusMessage *call = Bus::newCall(path.c_str(), kProperties, "Set");
    if (!call || !appendPropertySet(call, kDevice1, "Trusted", trusted)) {
        if (call)
            dbus_message_unref(call);
        if (onOperation)
            onOperation(false, hintMessage("Trust failed", "could not build the request"));
        return;
    }

    if (!mImpl->bus.callAsync(call, kTimeoutTrust,
                              [this](bool ok, const std::string &error, DBusMessage *) {
                                  if (onOperation)
                                      onOperation(ok, ok ? std::string()
                                                         : hintMessage("Trust failed", error));
                              })) {
        if (onOperation)
            onOperation(false, hintMessage("Trust failed", "could not send the request"));
    }
}

void Bluez::connect(const std::string &path)
{
    if (!mImpl->bus.isOpen() || path.empty())
        return;

    // ConnectProfile(A2DP Sink) FIRST, generic Connect as the fallback -- see bluez.h. The
    // fallback is attempted exactly once, which is what `tried_profile` tracked in the GDBus
    // build; a second failure is reported rather than retried.
    DBusMessage *call = Bus::newCall(path.c_str(), kDevice1, "ConnectProfile");
    if (!call) {
        if (onOperation)
            onOperation(false, hintMessage("Connect failed", "out of memory"));
        return;
    }
    const char *uuid = kA2dpSinkUuid;
    dbus_message_append_args(call, DBUS_TYPE_STRING, &uuid, DBUS_TYPE_INVALID);

    const std::string devicePath = path;
    auto onProfileDone = [this, devicePath](bool ok, const std::string &error, DBusMessage *) {
        if (ok) {
            if (onOperation)
                onOperation(true, std::string());
            return;
        }
        // NOT RETRIED WHEN A RETRY CANNOT HELP. A page timeout means the device did not answer
        // at all -- off, out of range, or held by a phone -- and the generic Connect pages the same
        // silent device again, doubling the wait before the same error. An unpowered adapter is
        // the same story. Both strings are src/error.h's.
        if (error == "br-connection-page-timeout" || error == "br-connection-adapter-not-powered") {
            if (onOperation)
                onOperation(false, hintMessage("Connect failed", error));
            return;
        }
        DBusMessage *plain = Bus::newCall(devicePath.c_str(), kDevice1, "Connect");
        if (!mImpl->bus.callAsync(
                plain, kTimeoutConnect,
                [this](bool ok2, const std::string &error2, DBusMessage *) {
                    // THE REPLY IS THE VERDICT, because BlueZ already made it the right one.
                    // device_profile_connected() in src/device.c resets a Connect() error to
                    // success when any service ended up connected, and connect_profiles() answers
                    // success, not AlreadyConnected, for a device that is already up. So an error
                    // here means nothing connected. Second-guessing it with Device1.Connected --
                    // which an earlier revision did -- reported SUCCESS for a speaker whose only
                    // link was the one left idling after pairing, i.e. the exact opposite.
                    if (onOperation)
                        onOperation(ok2, ok2 ? std::string()
                                             : hintMessage("Connect failed", error2));
                })) {
            if (onOperation)
                onOperation(false, hintMessage("Connect failed", "could not send the request"));
        }
    };

    if (!mImpl->bus.callAsync(call, kTimeoutConnect, onProfileDone)) {
        if (onOperation)
            onOperation(false, hintMessage("Connect failed", "could not send the request"));
    }
}

void Bluez::removeDevice(const std::string &path)
{
    if (!mImpl->bus.isOpen() || path.empty())
        return;

    const std::string ad = adapterOf(path, mImpl->adapter);
    if (ad.empty()) {
        if (onOperation)
            onOperation(false, hintMessage("Remove failed", "no adapter for that device"));
        return;
    }

    DBusMessage *call = Bus::newCall(ad.c_str(), kAdapter1, "RemoveDevice");
    if (!call)
        return;
    const char *dev = path.c_str();
    dbus_message_append_args(call, DBUS_TYPE_OBJECT_PATH, &dev, DBUS_TYPE_INVALID);
    std::string error;
    Msg reply = mImpl->bus.callSync(call, kTimeoutTrust, &error);
    if (!reply && onOperation)
        onOperation(false, hintMessage("Remove failed", error));
}

bool Bluez::deviceState(const std::string &path, BluezDeviceProps *out)
{
    *out = BluezDeviceProps();
    if (!mImpl->bus.isOpen() || path.empty())
        return false;

    DBusMessage *call = Bus::newCall(path.c_str(), kProperties, "GetAll");
    if (!call)
        return false;
    const char *iface = kDevice1;
    dbus_message_append_args(call, DBUS_TYPE_STRING, &iface, DBUS_TYPE_INVALID);
    std::string error;
    Msg reply = mImpl->bus.callSync(call, kTimeoutQuick, &error);
    if (!reply)
        return false;

    DBusMessageIter it;
    if (!dbus_message_iter_init(reply.get(), &it))
        return false;
    if (dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_ARRAY)
        return false;

    DBusMessageIter dict;
    dbus_message_iter_recurse(&it, &dict);
    BluezDeviceProps p;
    p.path = path;
    Impl::readDeviceProps(&dict, &p);
    p.audioConnected = mImpl->hasTransport(path);
    *out = p;
    return true;
}

//------------------------------------------------------------------------
// The hint block compose_hint_message() appended to every failure (gui_bt.c:603). Kept because
// these three lines were written from support questions rather than invented, and they are what a
// user who cannot pair actually needs to check.
std::string Bluez::hintMessage(const std::string &prefix, const std::string &detail)
{
    // A REASON BLUEZ NAMED GETS ADVICE ABOUT THAT REASON. The strings are the br-connection-*
    // reasons from src/error.h (btd_error_bredr_conn_from_errno), which is what BlueZ puts in the
    // message of a failed Connect/ConnectProfile. Only reasons with one clear remedy are listed.
    //
    // Everything else keeps the GTK build's catch-all, which leads with group membership and the
    // polkit rule. That is the right first check for an ACCESS failure and the wrong one for a
    // device that simply did not answer -- it sent the user checking their groups over a speaker
    // that was switched off.
    struct Hint {
        const char *reason;
        const char *advice;
    };
    static const Hint kHints[] = {
        {"br-connection-page-timeout",
         "The device did not answer. Check it is switched on, in range, and not connected to "
         "another device such as a phone."},
        {"br-connection-adapter-not-powered", "The Bluetooth adapter is switched off."},
        {"br-connection-profile-unavailable",
         "No audio profile in common. Check that bluealsad is running."},
        {"br-connection-key-missing",
         "The pairing keys no longer match. Remove the device and pair it again."},
        {"br-connection-refused", "The device refused. Remove it and pair it again."},
    };

    std::string m = prefix;
    if (!prefix.empty() && !detail.empty())
        m += ": ";
    m += detail;
    for (const Hint &h : kHints) {
        if (detail == h.reason)
            return m + "  " + h.advice;
    }
    // EVERY CHECK THE GTK BUILD LISTED, IN FEWER WORDS. The full sentence measured 1258 units
    // behind the longest reason BlueZ can give, in a strip that holds 973, so it was cut off
    // mid-path -- and nothing measured it until uirender's corpus carried it. The polkit rule is
    // named by file rather than by its /etc/polkit-1/rules.d/ path: the name is what a search
    // finds, and it is the part that was being cut.
    m += "  Check: membership of the 'audio' (and 'bluetooth') group, "
         "90-jack-bridge-bluetooth.rules, adapter on, device in range.";
    return m;
}

} // namespace jackbridge

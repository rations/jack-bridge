// The libdbus-1 connection, and the ONLY file in this project that includes <dbus/dbus.h>.
//
// This replaces GDBus. Everything above it -- bluez.cpp, agent.cpp -- talks to a D-Bus type system
// through the small vocabulary below rather than through libdbus directly, for the same reason
// gfx/ never sees an X type: one place to get the marshalling discipline right, and one place to
// look when it is wrong.
//
//------------------------------------------------------------------------------------------------
// THE LOOP INTEGRATION IS THE WHOLE POINT OF THIS FILE, so it is spelled out.
//
// libdbus does not own a main loop. It hands the application a set of DBusWatch objects (file
// descriptors it wants waited on) and DBusTimeout objects (deadlines it wants woken at), through
// dbus_connection_set_watch_functions and set_timeout_functions. BOTH SETS CHANGE AT RUNTIME: a
// watch is added during authentication, watches toggle enabled and disabled as the outgoing queue
// fills and drains, and a timeout appears for every call that has a deadline. So this class
// publishes the current descriptors and fires onWatchesChanged whenever they move; the application
// re-reads them and re-registers with X11Window::addFd rather than registering once.
//
// THE SHORTCUT IS WRONG AND IT IS WORTH SAYING WHY, because it is what most examples show:
//
//     int fd; dbus_connection_get_unix_fd(conn, &fd);            // one descriptor
//     ... select on it ...
//     dbus_connection_read_write_dispatch(conn, 0);
//
// That is *almost* right, and it fails in two ways that both look like something else. It sees
// only the first descriptor, so the watch libdbus adds during authentication is never waited on;
// and it has no notion of a timeout, so a call that times out with no further traffic on the
// socket never completes -- the reply callback simply never fires and the UI waits for ever on a
// pairing that already failed. Neither shows up in a quick test on an idle bus.
//
// WRITE WATCHES. libdbus asks for a write watch when its outgoing queue cannot drain in one go.
// X11Window::addFd takes a `wantWrite` flag for exactly this, and its header records the rule that
// makes it safe: a write watch is ready nearly always, so it would spin the loop -- except that
// libdbus disables it the moment the queue empties, which fires the toggle below and takes the
// descriptor back out of the set.
//
//------------------------------------------------------------------------------------------------
// MARSHALLING DISCIPLINE. Every read checks dbus_message_iter_get_arg_type BEFORE get_basic.
// libdbus's get_basic on a mismatched type does not fail, it reinterprets: reading a DBUS_TYPE_
// STRING out of what is really a boolean hands back a pointer built from a 0 or a 1 and the next
// strlen() walks off into nothing. The system bus carries traffic from things this program did not
// write, so "the reply will have the shape the documentation says" is not a safety argument. The
// GDBus code this replaces checked g_variant_is_of_type before every read for the same reason, and
// that discipline is carried across rather than trusted to the daemon.

#pragma once

#include <dbus/dbus.h>

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

// A DBusMessage with a destructor. Every message in this project is owned by one of these, so a
// path that returns early cannot leak one -- which, with a dozen early returns per call site, is
// otherwise a matter of remembering.
class Msg
{
public:
    Msg() = default;
    explicit Msg(DBusMessage *m) : mMsg(m)
    {
    }
    ~Msg()
    {
        reset();
    }

    Msg(const Msg &) = delete;
    Msg &operator=(const Msg &) = delete;

    Msg(Msg &&o) noexcept : mMsg(o.mMsg)
    {
        o.mMsg = nullptr;
    }
    Msg &operator=(Msg &&o) noexcept
    {
        if (this != &o) {
            reset();
            mMsg = o.mMsg;
            o.mMsg = nullptr;
        }
        return *this;
    }

    void reset(DBusMessage *m = nullptr)
    {
        if (mMsg)
            dbus_message_unref(mMsg);
        mMsg = m;
    }
    DBusMessage *get() const
    {
        return mMsg;
    }
    explicit operator bool() const
    {
        return mMsg != nullptr;
    }

    // Takes a reference, for the cases where libdbus hands out a borrowed message (a filter's
    // argument, a pending call's reply) and it has to outlive the call.
    static Msg ref(DBusMessage *m)
    {
        if (m)
            dbus_message_ref(m);
        return Msg(m);
    }

private:
    DBusMessage *mMsg = nullptr;
};

//--- reading ------------------------------------------------------------------------------------
// Each of these checks the type it is about to read and returns false rather than reinterpreting.
bool readString(DBusMessageIter *it, std::string *out);     // s or o
bool readBool(DBusMessageIter *it, bool *out);              // b
// Steps INTO a variant and reads the basic value inside it. `Get` returns `v` and the properties
// dictionaries are a{sv}, so almost every value this program reads arrives boxed.
bool readVariantString(DBusMessageIter *it, std::string *out);
bool readVariantBool(DBusMessageIter *it, bool *out);

//--- writing ------------------------------------------------------------------------------------
// Properties.Set takes (ssv) and the variant has to be built by hand: there is no format-string
// equivalent of g_variant_new("(ssv)", ...) here.
bool appendPropertySet(DBusMessage *m, const char *iface, const char *name, bool value);

class Bus
{
public:
    Bus() = default;
    ~Bus();

    Bus(const Bus &) = delete;
    Bus &operator=(const Bus &) = delete;

    // PRIVATE, not shared. dbus_bus_get() hands out a shared connection whose dispatch belongs to
    // whoever else in the process is using it; dbus_bus_get_private() gives a connection this
    // object may close, which is what makes close() mean anything.
    bool open();
    // Cancels every pending call, removes the filter and the match rules, closes and unrefs.
    void close();
    bool isOpen() const
    {
        return mConn != nullptr;
    }

    DBusConnection *conn() const
    {
        return mConn;
    }

    //--- the loop -------------------------------------------------------
    // Fired whenever the watch set changed, from inside libdbus. The application re-reads
    // watches() and re-registers.
    std::function<void()> onWatchesChanged;

    struct WatchFd {
        int fd;
        bool write;
    };
    std::vector<WatchFd> watches() const;

    // One descriptor became ready. Handles every enabled watch on that descriptor, then dispatches
    // until DBUS_DISPATCH_COMPLETE -- because one read can carry several messages and libdbus
    // hands them over one dispatch at a time.
    void handleFd(int fd, bool writable);
    // Fire any libdbus timeout that is due. Called on a fixed tick.
    void handleTimeouts();

    //--- messaging ------------------------------------------------------
    // Build a method call to org.bluez. The caller appends its arguments and hands it to one of
    // the two below, WHICH TAKE OWNERSHIP -- including on every failure path, so a call site that
    // gives up halfway has nothing to unref.
    static DBusMessage *newCall(const char *path, const char *iface, const char *member);

    // A blocking call. `timeoutMs` is milliseconds; -1 is libdbus's own default (25 s). On failure
    // the returned Msg is empty and `error` carries the D-Bus error message, which is shown to the
    // user because BlueZ's own text ("Device is not paired", "Authentication Failed") is more use
    // than anything this program could write in its place.
    Msg callSync(DBusMessage *call, int timeoutMs, std::string *error);

    // A non-blocking call. `cb` runs on the main thread when the reply arrives, or with ok=false
    // and the error text when it fails. It does NOT run if the Bus is closed first -- close()
    // cancels every pending call, which is what the GDBus build's g_shutting_down flag was for.
    using ReplyCb = std::function<void(bool ok, const std::string &error, DBusMessage *reply)>;
    bool callAsync(DBusMessage *call, int timeoutMs, ReplyCb cb);

    //--- signals --------------------------------------------------------
    // `rule` is a match rule string. Errors are reported and swallowed: a match rule that the bus
    // refuses means some updates stop arriving, not that the program cannot run.
    void addMatch(const std::string &rule);
    // One filter for every signal. It returns true to say the message was consumed.
    std::function<bool(DBusMessage *)> onMessage;

private:
    // The four libdbus callbacks. Static because libdbus takes plain function pointers.
    static dbus_bool_t addWatch(DBusWatch *w, void *data);
    static void removeWatch(DBusWatch *w, void *data);
    static void toggledWatch(DBusWatch *w, void *data);
    static dbus_bool_t addTimeout(DBusTimeout *t, void *data);
    static void removeTimeout(DBusTimeout *t, void *data);
    static void toggledTimeout(DBusTimeout *t, void *data);
    static DBusHandlerResult filter(DBusConnection *c, DBusMessage *m, void *data);
    static void pendingNotify(DBusPendingCall *pending, void *data);
    static void pendingFree(void *data);

    void dispatchAll();

    DBusConnection *mConn = nullptr;

    std::vector<DBusWatch *> mWatches;

    struct TimeoutEntry {
        DBusTimeout *t;
        long dueNs; // absolute CLOCK_MONOTONIC
    };
    std::vector<TimeoutEntry> mTimeouts;

    std::vector<std::string> mMatches;

    // Every pending call, so close() can cancel them. A notify firing into a destroyed panel is
    // the crash the GDBus build's g_shutting_down guard was added for, and a cancelled call's
    // notify does not fire at all -- which is a stronger guarantee than a flag checked inside it.
    std::vector<DBusPendingCall *> mPending;

    bool mFilterAdded = false;

    // Guards dispatchAll() against re-entering itself. A dispatched signal reaches the panel, and
    // the panel answers questions about a device with a synchronous call -- which dispatches again.
    // Without this, one InterfacesAdded during a busy scan recurses as deep as the queue is long.
    bool mDispatching = false;
};

} // namespace jackbridge

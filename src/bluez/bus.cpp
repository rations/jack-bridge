// See bus.h.

#include "bus.h"

#include <time.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace jackbridge
{

namespace
{

const char *const kBluez = "org.bluez";

long nowNs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

// The context a pending call carries. Freed through the DBusFreeFunction libdbus calls when the
// pending call is unreffed -- including when it is cancelled -- so there is one owner and no path
// where the callback's captures outlive it.
struct PendingCtx {
    Bus *bus;
    Bus::ReplyCb cb;
};

} // namespace

//------------------------------------------------------------------------
bool readString(DBusMessageIter *it, std::string *out)
{
    const int t = dbus_message_iter_get_arg_type(it);
    if (t != DBUS_TYPE_STRING && t != DBUS_TYPE_OBJECT_PATH && t != DBUS_TYPE_SIGNATURE)
        return false;
    const char *s = nullptr;
    dbus_message_iter_get_basic(it, &s);
    if (!s)
        return false;
    *out = s;
    return true;
}

bool readBool(DBusMessageIter *it, bool *out)
{
    if (dbus_message_iter_get_arg_type(it) != DBUS_TYPE_BOOLEAN)
        return false;
    dbus_bool_t b = FALSE;
    dbus_message_iter_get_basic(it, &b);
    *out = b != FALSE;
    return true;
}

bool readVariantString(DBusMessageIter *it, std::string *out)
{
    if (dbus_message_iter_get_arg_type(it) != DBUS_TYPE_VARIANT)
        return false;
    DBusMessageIter v;
    dbus_message_iter_recurse(it, &v);
    return readString(&v, out);
}

bool readVariantBool(DBusMessageIter *it, bool *out)
{
    if (dbus_message_iter_get_arg_type(it) != DBUS_TYPE_VARIANT)
        return false;
    DBusMessageIter v;
    dbus_message_iter_recurse(it, &v);
    return readBool(&v, out);
}

//------------------------------------------------------------------------
bool appendPropertySet(DBusMessage *m, const char *iface, const char *name, bool value)
{
    DBusMessageIter it;
    dbus_message_iter_init_append(m, &it);
    if (!dbus_message_iter_append_basic(&it, DBUS_TYPE_STRING, &iface))
        return false;
    if (!dbus_message_iter_append_basic(&it, DBUS_TYPE_STRING, &name))
        return false;

    // The variant has to be opened, filled and closed by hand. Note the signature string is
    // "b" -- the CONTENTS of the variant, not the variant itself. Passing "v" here produces a
    // variant containing a variant, which BlueZ rejects with "invalid signature"; the GDBus code
    // this replaces carries a comment about exactly that double-boxing on some GLib versions.
    DBusMessageIter var;
    if (!dbus_message_iter_open_container(&it, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING,
                                          &var))
        return false;
    const dbus_bool_t v = value ? TRUE : FALSE;
    if (!dbus_message_iter_append_basic(&var, DBUS_TYPE_BOOLEAN, &v)) {
        dbus_message_iter_abandon_container(&it, &var);
        return false;
    }
    return dbus_message_iter_close_container(&it, &var);
}

//------------------------------------------------------------------------
Bus::~Bus()
{
    close();
}

bool Bus::open()
{
    if (mConn)
        return true;

    DBusError err;
    dbus_error_init(&err);

    mConn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
    if (!mConn) {
        fprintf(stderr, "jack-bridge: cannot connect to the system bus: %s\n",
                dbus_error_is_set(&err) ? err.message : "unknown error");
        dbus_error_free(&err);
        return false;
    }
    dbus_error_free(&err);

    // NON-NEGOTIABLE. The default is to _exit() the process when the bus drops, so a bluetoothd
    // restart -- or a dbus-daemon restart -- would take the mixer down with it. mxeq has to stay
    // up with its Bluetooth page inert; that is half of the plan's check 9.
    dbus_connection_set_exit_on_disconnect(mConn, FALSE);

    if (!dbus_connection_set_watch_functions(mConn, &Bus::addWatch, &Bus::removeWatch,
                                             &Bus::toggledWatch, this, nullptr)) {
        fprintf(stderr, "jack-bridge: cannot install the D-Bus watch functions\n");
        close();
        return false;
    }
    if (!dbus_connection_set_timeout_functions(mConn, &Bus::addTimeout, &Bus::removeTimeout,
                                               &Bus::toggledTimeout, this, nullptr)) {
        fprintf(stderr, "jack-bridge: cannot install the D-Bus timeout functions\n");
        close();
        return false;
    }

    if (!dbus_connection_add_filter(mConn, &Bus::filter, this, nullptr)) {
        fprintf(stderr, "jack-bridge: cannot install the D-Bus message filter\n");
        close();
        return false;
    }
    mFilterAdded = true;
    return true;
}

void Bus::close()
{
    if (!mConn)
        return;

    // CANCEL EVERY PENDING CALL FIRST. A cancelled call's notify does not fire, so nothing can
    // call back into a half-destroyed client while the rest of this runs. The GDBus build set a
    // g_shutting_down flag and checked it inside each callback instead; cancelling is the same
    // intent with no window between the flag and the check.
    for (DBusPendingCall *p : mPending) {
        dbus_pending_call_cancel(p);
        dbus_pending_call_unref(p);
    }
    mPending.clear();

    for (const std::string &rule : mMatches)
        dbus_bus_remove_match(mConn, rule.c_str(), nullptr);
    mMatches.clear();

    if (mFilterAdded) {
        dbus_connection_remove_filter(mConn, &Bus::filter, this);
        mFilterAdded = false;
    }

    // Detach libdbus from the loop before closing, so nothing tries to add a watch during
    // teardown and leave a descriptor registered on a connection that is gone.
    dbus_connection_set_watch_functions(mConn, nullptr, nullptr, nullptr, nullptr, nullptr);
    dbus_connection_set_timeout_functions(mConn, nullptr, nullptr, nullptr, nullptr, nullptr);
    mWatches.clear();
    mTimeouts.clear();

    // close() then unref(), in that order and both of them: a PRIVATE connection is not closed by
    // dropping the last reference, and one that is only unreffed leaks its socket.
    dbus_connection_close(mConn);
    dbus_connection_unref(mConn);
    mConn = nullptr;

    if (onWatchesChanged)
        onWatchesChanged();
}

//------------------------------------------------------------------------
std::vector<Bus::WatchFd> Bus::watches() const
{
    std::vector<WatchFd> out;
    for (DBusWatch *w : mWatches) {
        if (!dbus_watch_get_enabled(w))
            continue;
        const unsigned flags = dbus_watch_get_flags(w);
        const int fd = dbus_watch_get_unix_fd(w);
        if (fd < 0)
            continue;
        if (flags & DBUS_WATCH_READABLE)
            out.push_back({fd, false});
        if (flags & DBUS_WATCH_WRITABLE)
            out.push_back({fd, true});
    }
    return out;
}

void Bus::handleFd(int fd, bool writable)
{
    if (!mConn)
        return;

    // A COPY, because dbus_watch_handle can add or remove watches from inside libdbus.
    const std::vector<DBusWatch *> watches = mWatches;
    for (DBusWatch *w : watches) {
        if (!dbus_watch_get_enabled(w) || dbus_watch_get_unix_fd(w) != fd)
            continue;
        // Still registered? A previous handle() in this same pass may have removed it.
        if (std::find(mWatches.begin(), mWatches.end(), w) == mWatches.end())
            continue;
        const unsigned flags = dbus_watch_get_flags(w);
        const unsigned want = writable ? DBUS_WATCH_WRITABLE : DBUS_WATCH_READABLE;
        if (!(flags & want))
            continue;
        dbus_watch_handle(w, want);
    }

    dispatchAll();
}

void Bus::dispatchAll()
{
    if (!mConn)
        return;
    // ONE READ CAN CARRY SEVERAL MESSAGES, and libdbus hands them over one dispatch at a time. A
    // single dispatch per wake-up therefore leaves messages sitting in the queue until the next
    // one arrives -- which on an idle bus can be a long time, and looks exactly like a signal
    // that was never sent.
    while (dbus_connection_dispatch(mConn) == DBUS_DISPATCH_DATA_REMAINS) {
    }
}

void Bus::handleTimeouts()
{
    if (!mConn)
        return;
    const long now = nowNs();

    // A copy again: handling a timeout can remove it, and can add others.
    const std::vector<TimeoutEntry> due = mTimeouts;
    for (const TimeoutEntry &e : due) {
        if (e.dueNs > now)
            continue;
        if (std::find_if(mTimeouts.begin(), mTimeouts.end(), [&](const TimeoutEntry &c) {
                return c.t == e.t;
            }) == mTimeouts.end())
            continue;
        if (!dbus_timeout_get_enabled(e.t))
            continue;
        // Re-arm BEFORE handling: dbus_timeout_handle can complete a call whose callback starts
        // another one, and an entry re-armed afterwards would be re-armed on top of whatever that
        // left behind.
        for (TimeoutEntry &cur : mTimeouts) {
            if (cur.t == e.t) {
                cur.dueNs = now + static_cast<long>(dbus_timeout_get_interval(e.t)) * 1000000L;
                break;
            }
        }
        dbus_timeout_handle(e.t);
    }

    dispatchAll();
}

//------------------------------------------------------------------------
dbus_bool_t Bus::addWatch(DBusWatch *w, void *data)
{
    Bus *self = static_cast<Bus *>(data);
    self->mWatches.push_back(w);
    if (self->onWatchesChanged)
        self->onWatchesChanged();
    return TRUE;
}

void Bus::removeWatch(DBusWatch *w, void *data)
{
    Bus *self = static_cast<Bus *>(data);
    auto it = std::find(self->mWatches.begin(), self->mWatches.end(), w);
    if (it != self->mWatches.end())
        self->mWatches.erase(it);
    if (self->onWatchesChanged)
        self->onWatchesChanged();
}

void Bus::toggledWatch(DBusWatch *, void *data)
{
    // The watch object is unchanged; only its enabled flag moved. watches() reads that flag, so
    // the set the application gets back is already right -- it just has to be asked again. This is
    // what keeps a write watch from spinning the loop after libdbus is done with it.
    Bus *self = static_cast<Bus *>(data);
    if (self->onWatchesChanged)
        self->onWatchesChanged();
}

dbus_bool_t Bus::addTimeout(DBusTimeout *t, void *data)
{
    Bus *self = static_cast<Bus *>(data);
    self->mTimeouts.push_back(
        {t, nowNs() + static_cast<long>(dbus_timeout_get_interval(t)) * 1000000L});
    return TRUE;
}

void Bus::removeTimeout(DBusTimeout *t, void *data)
{
    Bus *self = static_cast<Bus *>(data);
    for (size_t i = 0; i < self->mTimeouts.size(); ++i) {
        if (self->mTimeouts[i].t == t) {
            self->mTimeouts.erase(self->mTimeouts.begin() + static_cast<long>(i));
            return;
        }
    }
}

void Bus::toggledTimeout(DBusTimeout *t, void *data)
{
    // Re-arm from now. A timeout that was disabled and re-enabled has not been counting down in
    // between, and treating its old deadline as live fires it immediately.
    Bus *self = static_cast<Bus *>(data);
    for (TimeoutEntry &e : self->mTimeouts) {
        if (e.t == t) {
            e.dueNs = nowNs() + static_cast<long>(dbus_timeout_get_interval(t)) * 1000000L;
            return;
        }
    }
}

//------------------------------------------------------------------------
DBusHandlerResult Bus::filter(DBusConnection *, DBusMessage *m, void *data)
{
    Bus *self = static_cast<Bus *>(data);
    if (self->onMessage && self->onMessage(m))
        return DBUS_HANDLER_RESULT_HANDLED;
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

//------------------------------------------------------------------------
DBusMessage *Bus::newCall(const char *path, const char *iface, const char *member)
{
    return dbus_message_new_method_call(kBluez, path, iface, member);
}

Msg Bus::callSync(DBusMessage *callMsg, int timeoutMs, std::string *error)
{
    // Owned from here on, on every path out. A caller that ran out of memory building its
    // arguments passes the half-built message in and this frees it.
    Msg call(callMsg);
    if (!call) {
        if (error)
            *error = "out of memory building the call";
        return Msg();
    }
    if (!mConn) {
        if (error)
            *error = "not connected to the system bus";
        return Msg();
    }

    DBusError err;
    dbus_error_init(&err);
    DBusMessage *reply =
        dbus_connection_send_with_reply_and_block(mConn, call.get(), timeoutMs, &err);
    if (!reply) {
        if (error)
            *error = dbus_error_is_set(&err) ? err.message : "no reply";
        dbus_error_free(&err);
        return Msg();
    }
    dbus_error_free(&err);
    return Msg(reply);
}

bool Bus::callAsync(DBusMessage *callMsg, int timeoutMs, ReplyCb cb)
{
    Msg call(callMsg);
    if (!call || !mConn)
        return false;

    DBusPendingCall *pending = nullptr;
    if (!dbus_connection_send_with_reply(mConn, call.get(), &pending, timeoutMs) || !pending)
        return false;

    PendingCtx *ctx = new PendingCtx{this, std::move(cb)};
    if (!dbus_pending_call_set_notify(pending, &Bus::pendingNotify, ctx, &Bus::pendingFree)) {
        delete ctx;
        dbus_pending_call_cancel(pending);
        dbus_pending_call_unref(pending);
        return false;
    }

    // One reference is held here so close() can cancel it. The notify drops it again.
    dbus_pending_call_ref(pending);
    mPending.push_back(pending);

    // The reference send_with_reply() gave us. The one just taken is the tracking reference.
    dbus_pending_call_unref(pending);
    return true;
}

void Bus::pendingNotify(DBusPendingCall *pending, void *data)
{
    PendingCtx *ctx = static_cast<PendingCtx *>(data);
    Bus *self = ctx->bus;

    // Drop the tracking reference: this call is finished either way, and leaving it in the list
    // means close() would cancel a completed call and unref it a second time.
    auto it = std::find(self->mPending.begin(), self->mPending.end(), pending);
    if (it != self->mPending.end()) {
        self->mPending.erase(it);
        dbus_pending_call_unref(pending);
    }

    Msg reply(dbus_pending_call_steal_reply(pending));
    if (!reply) {
        if (ctx->cb)
            ctx->cb(false, "no reply", nullptr);
        return;
    }

    if (dbus_message_get_type(reply.get()) == DBUS_MESSAGE_TYPE_ERROR) {
        // BlueZ's own error text, verbatim. "org.bluez.Error.AuthenticationFailed: Authentication
        // Failed" tells a user more about a refused pairing than anything this program could write
        // in its place, which is why the GDBus build passed GError::message through too.
        std::string text = dbus_message_get_error_name(reply.get())
                               ? dbus_message_get_error_name(reply.get())
                               : "org.freedesktop.DBus.Error.Failed";
        DBusMessageIter it2;
        if (dbus_message_iter_init(reply.get(), &it2)) {
            std::string detail;
            if (readString(&it2, &detail) && !detail.empty())
                text = detail;
        }
        if (ctx->cb)
            ctx->cb(false, text, nullptr);
        return;
    }

    if (ctx->cb)
        ctx->cb(true, std::string(), reply.get());
}

void Bus::pendingFree(void *data)
{
    delete static_cast<PendingCtx *>(data);
}

//------------------------------------------------------------------------
void Bus::addMatch(const std::string &rule)
{
    if (!mConn)
        return;
    DBusError err;
    dbus_error_init(&err);
    dbus_bus_add_match(mConn, rule.c_str(), &err);
    if (dbus_error_is_set(&err)) {
        // Not fatal: a refused match rule means some updates stop arriving, not that the program
        // cannot run. It is worth a line on stderr because "the list never updates" is otherwise a
        // silent symptom with no cause attached.
        fprintf(stderr, "jack-bridge: could not add the D-Bus match rule '%s': %s\n", rule.c_str(),
                err.message);
        dbus_error_free(&err);
        return;
    }
    dbus_error_free(&err);
    mMatches.push_back(rule);
}

} // namespace jackbridge

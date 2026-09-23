// See agent.h.

#include "agent.h"

#include "bus.h"

#include <cstdio>
#include <cstring>

namespace jackbridge
{

const char *const Agent::kPath = "/org/bluez/JackBridgeAgent";

// NOT A FREE CHOICE. The capability tells BlueZ what this agent can do, and BlueZ picks which of
// the nine Agent1 methods to call from it: "KeyboardDisplay" is the one that gets
// RequestConfirmation and RequestAuthorization rather than RequestPinCode alone. bt_agent.c
// registered with this string, so it stays -- changing it changes which pairings work.
const char *const Agent::kCapability = "KeyboardDisplay";

namespace
{

const char *const kAgentIface = "org.bluez.Agent1";
const char *const kManagerPath = "/org/bluez";
const char *const kManagerIface = "org.bluez.AgentManager1";

// The introspection XML BlueZ reads to learn what we answer. RequestAuthorization is present here
// and absent from the GDBus original; agent.h records why that omission was a bug.
const char *const kIntrospectXml =
    DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
    "<node>"
    "  <interface name='org.freedesktop.DBus.Introspectable'>"
    "    <method name='Introspect'>"
    "      <arg type='s' name='xml' direction='out'/>"
    "    </method>"
    "  </interface>"
    "  <interface name='org.bluez.Agent1'>"
    "    <method name='Release'/>"
    "    <method name='RequestPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='in'/>"
    "    </method>"
    "    <method name='RequestPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "      <arg type='q' name='entered' direction='in'/>"
    "    </method>"
    "    <method name='RequestConfirmation'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "    </method>"
    "    <method name='RequestAuthorization'>"
    "      <arg type='o' name='device' direction='in'/>"
    "    </method>"
    "    <method name='AuthorizeService'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='uuid' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

// The last path component of /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF, as a MAC, for a prompt a human
// can match against the thing in their hand. Falls back to the whole path rather than to nothing.
std::string deviceLabel(const std::string &path)
{
    const size_t slash = path.rfind('/');
    if (slash == std::string::npos)
        return path;
    std::string tail = path.substr(slash + 1);
    if (tail.compare(0, 4, "dev_") != 0)
        return tail;
    tail = tail.substr(4);
    for (char &c : tail) {
        if (c == '_')
            c = ':';
    }
    return tail;
}

void sendReply(DBusConnection *conn, DBusMessage *call, DBusMessage *reply)
{
    if (!reply) {
        // Out of memory building a reply. Saying nothing would leave BlueZ waiting out its own
        // timeout on the pairing; an error at least ends it now.
        reply = dbus_message_new_error(call, DBUS_ERROR_NO_MEMORY, "out of memory");
        if (!reply)
            return;
    }
    dbus_connection_send(conn, reply, nullptr);
    dbus_message_unref(reply);
}

} // namespace

//------------------------------------------------------------------------
Agent::~Agent()
{
    remove();
}

//------------------------------------------------------------------------
namespace
{

// The one message_function for the exported object. It dispatches on interface and member, which
// is what the GDBus build got from a parsed introspection node and a vtable.
DBusHandlerResult agentMessage(DBusConnection *conn, DBusMessage *msg, void *data)
{
    Agent *self = static_cast<Agent *>(data);

    const char *iface = dbus_message_get_interface(msg);
    const char *member = dbus_message_get_member(msg);
    if (!iface || !member)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (strcmp(iface, DBUS_INTERFACE_INTROSPECTABLE) == 0 && strcmp(member, "Introspect") == 0) {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            const char *xml = kIntrospectXml;
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
        }
        sendReply(conn, msg, reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(iface, kAgentIface) != 0)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    // Every Agent1 method except Release and Cancel takes the device path first.
    std::string device;
    DBusMessageIter it;
    const bool haveArgs = dbus_message_iter_init(msg, &it);
    if (haveArgs)
        readString(&it, &device);

    if (strcmp(member, "Release") == 0 || strcmp(member, "Cancel") == 0) {
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "RequestPinCode") == 0) {
        // The legacy default the GDBus build returned. It works for the headsets that ask.
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            const char *pin = "0000";
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &pin, DBUS_TYPE_INVALID);
        }
        sendReply(conn, msg, reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "RequestPasskey") == 0) {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            const dbus_uint32_t passkey = 0;
            dbus_message_append_args(reply, DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);
        }
        sendReply(conn, msg, reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "DisplayPinCode") == 0) {
        std::string pin;
        if (haveArgs) {
            dbus_message_iter_next(&it);
            readString(&it, &pin);
        }
        if (self->onPrompt && !pin.empty())
            self->onPrompt("Enter " + pin + " on " + deviceLabel(device));
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "DisplayPasskey") == 0) {
        dbus_uint32_t passkey = 0;
        if (haveArgs) {
            dbus_message_iter_next(&it);
            if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_UINT32)
                dbus_message_iter_get_basic(&it, &passkey);
        }
        if (self->onPrompt) {
            // Zero-padded to six digits, which doc/org.bluez.Agent.rst asks for explicitly: the
            // passkey is always a six-digit number and 001234 shown as 1234 is a passkey the user
            // types wrongly.
            char buf[64];
            snprintf(buf, sizeof(buf), "Passkey %06u on %s", static_cast<unsigned>(passkey),
                     deviceLabel(device).c_str());
            self->onPrompt(buf);
        }
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "RequestConfirmation") == 0) {
        dbus_uint32_t passkey = 0;
        if (haveArgs) {
            dbus_message_iter_next(&it);
            if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_UINT32)
                dbus_message_iter_get_basic(&it, &passkey);
        }
        if (self->onPrompt) {
            char buf[96];
            snprintf(buf, sizeof(buf), "Confirm passkey %06u on %s -- accepted",
                     static_cast<unsigned>(passkey), deviceLabel(device).c_str());
            self->onPrompt(buf);
        }
        // AUTO-ACCEPT, unchanged. An empty reply is what confirms it.
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "RequestAuthorization") == 0) {
        // THE METHOD THE GDBus BUILD DID NOT IMPLEMENT. BlueZ calls this on a KeyboardDisplay
        // agent for an incoming just-works pairing -- exactly what the Discoverable toggle invites
        // -- and an unimplemented method fails that pairing with UnknownMethod.
        if (self->onPrompt)
            self->onPrompt("Pairing request from " + deviceLabel(device) + " -- accepted");
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "AuthorizeService") == 0) {
        sendReply(conn, msg, dbus_message_new_method_return(msg));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    sendReply(conn, msg,
              dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD,
                                     "this agent does not implement that method"));
    return DBUS_HANDLER_RESULT_HANDLED;
}

const DBusObjectPathVTable kVTable = {nullptr, &agentMessage, nullptr, nullptr, nullptr, nullptr};

} // namespace

//------------------------------------------------------------------------
bool Agent::install(Bus &bus)
{
    if (!bus.isOpen())
        return false;
    mBus = &bus;

    if (!dbus_connection_register_object_path(bus.conn(), kPath, &kVTable, this)) {
        fprintf(stderr, "jack-bridge: cannot export the pairing agent at %s\n", kPath);
        return false;
    }
    mExported = true;

    // UNREGISTER FIRST, IGNORING THE RESULT. A previous run that was killed rather than closed
    // leaves an agent entry behind in bluetoothd, and RegisterAgent then fails with "Already
    // Exists" -- which the user sees as pairing that silently stops working until they restart
    // bluetoothd. This is bt_agent.c:185's comment, carried over.
    {
        DBusMessage *call = dbus_message_new_method_call("org.bluez", kManagerPath, kManagerIface,
                                                         "UnregisterAgent");
        if (call) {
            const char *path = kPath;
            dbus_message_append_args(call, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID);
            std::string ignored;
            bus.callSync(call, 2000, &ignored);
        }
    }

    {
        DBusMessage *call = dbus_message_new_method_call("org.bluez", kManagerPath, kManagerIface,
                                                         "RegisterAgent");
        if (!call) {
            remove();
            return false;
        }
        const char *path = kPath;
        const char *cap = kCapability;
        dbus_message_append_args(call, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_STRING, &cap,
                                 DBUS_TYPE_INVALID);
        std::string error;
        Msg reply = bus.callSync(call, 5000, &error);
        if (!reply) {
            fprintf(stderr, "jack-bridge: RegisterAgent failed: %s\n", error.c_str());
            remove();
            return false;
        }
    }
    mRegistered = true;

    // Becoming the DEFAULT agent is what makes BlueZ route an incoming pairing here rather than to
    // whatever else on the system has an agent. Failing it is not fatal: pairings this program
    // starts itself still come to us.
    {
        DBusMessage *call = dbus_message_new_method_call("org.bluez", kManagerPath, kManagerIface,
                                                         "RequestDefaultAgent");
        if (call) {
            const char *path = kPath;
            dbus_message_append_args(call, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID);
            std::string error;
            Msg reply = bus.callSync(call, 5000, &error);
            if (!reply)
                fprintf(stderr,
                        "jack-bridge: could not become the default pairing agent (%s); pairings "
                        "started from this window will still work\n",
                        error.c_str());
        }
    }

    return true;
}

void Agent::remove()
{
    if (!mBus || !mBus->isOpen()) {
        mBus = nullptr;
        mExported = false;
        mRegistered = false;
        return;
    }

    if (mRegistered) {
        // TELL BLUEZ BEFORE DROPPING THE OBJECT. Without this, bluetoothd only notices the agent
        // is gone when the whole D-Bus connection closes, which leaves a window in which a fast
        // restart of this program gets "Already Exists" from RegisterAgent.
        DBusMessage *call = dbus_message_new_method_call("org.bluez", kManagerPath, kManagerIface,
                                                         "UnregisterAgent");
        if (call) {
            const char *path = kPath;
            dbus_message_append_args(call, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID);
            std::string ignored;
            mBus->callSync(call, 2000, &ignored);
        }
        mRegistered = false;
    }

    if (mExported) {
        dbus_connection_unregister_object_path(mBus->conn(), kPath);
        mExported = false;
    }
    mBus = nullptr;
}

} // namespace jackbridge

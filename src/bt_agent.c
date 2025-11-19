/*
 * src/bt_agent.c
 *
 * Minimal BlueZ Agent implementation using GDBus.
 * - Exports /org/bluez/JackBridgeAgent implementing org.bluez.Agent1 methods.
 * - Registers itself with org.bluez.AgentManager1.RegisterAgent and
 *   org.bluez.AgentManager1.RequestDefault (RegisterDefaultAgent variant differs
 *   across BlueZ versions; we call RegisterAgent then RequestDefault if available).
 *
 * This implementation performs only logging and returns default values.
 * Integrate with GUI callbacks to prompt user for PIN/passkey where necessary.
 *
 * Build:
 *   gcc -Wall -Wextra `pkg-config --cflags glib-2.0 gio-2.0` src/bt_agent.c -o bin/bt_agent `pkg-config --libs glib-2.0 gio-2.0`
 */
 
#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#define AGENT_PATH "/org/bluez/JackBridgeAgent"
static guint registration_id = 0;
static GDBusConnection *agent_conn = NULL;

/* Introspection data for org.bluez.Agent1 */
static const gchar agent_introspection_xml[] =
  "<node>"
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
  "    <method name='AuthorizeService'>"
  "      <arg type='o' name='device' direction='in'/>"
  "      <arg type='s' name='uuid' direction='in'/>"
  "    </method>"
  "    <method name='Cancel'/>"
  "  </interface>"
  "</node>";

/* Generic method handler: perform simple, non-interactive behavior and log.
 * Replace calls to GUI prompt functions as needed.
 */
static void agent_method_call(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *method_name,
                              GVariant *parameters,
                              GDBusMethodInvocation *invocation,
                              gpointer user_data) {
    (void)connection; (void)object_path; (void)interface_name; (void)user_data;

    if (g_strcmp0(method_name, "Release") == 0) {
        g_info("bt_agent: Release called by %s", sender ? sender : "(unknown)");
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "RequestPinCode") == 0) {
        const gchar *device;
        g_variant_get(parameters, "(o)", &device);
        g_info("bt_agent: RequestPinCode for device %s (returning default '0000')", device);
        /* Non-interactive default: return legacy '0000' which works for many headsets */
        GVariant *ret = g_variant_new("(s)", "0000");
        g_dbus_method_invocation_return_value(invocation, ret);
        return;
    }

    if (g_strcmp0(method_name, "DisplayPinCode") == 0) {
        const gchar *device;
        const gchar *pincode;
        g_variant_get(parameters, "(os)", &device, &pincode);
        g_info("bt_agent: DisplayPinCode for %s pin=%s", device, pincode);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "RequestPasskey") == 0) {
        const gchar *device;
        g_variant_get(parameters, "(o)", &device);
        g_info("bt_agent: RequestPasskey for %s", device);
        /* return 0 as default passkey */
        GVariant *ret = g_variant_new("(u)", (guint)0);
        g_dbus_method_invocation_return_value(invocation, ret);
        return;
    }

    if (g_strcmp0(method_name, "DisplayPasskey") == 0) {
        const gchar *device;
        guint passkey;
        guint entered;
        g_variant_get(parameters, "(ouq)", &device, &passkey, &entered);
        g_info("bt_agent: DisplayPasskey device=%s passkey=%u entered=%u", device, passkey, entered);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "RequestConfirmation") == 0) {
        const gchar *device;
        guint passkey;
        g_variant_get(parameters, "(ou)", &device, &passkey);
        g_info("bt_agent: RequestConfirmation device=%s passkey=%u (auto-accept)", device, passkey);
        /* auto-accept */
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "AuthorizeService") == 0) {
        const gchar *device;
        const gchar *uuid;
        g_variant_get(parameters, "(os)", &device, &uuid);
        g_info("bt_agent: AuthorizeService device=%s uuid=%s (auto-allow)", device, uuid);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "Cancel") == 0) {
        g_info("bt_agent: Cancel called");
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    /* Unknown method */
    g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown method %s", method_name);
}

/* Export the agent object on the given connection */
static gboolean export_agent_object(GDBusConnection *conn, GError **error) {
    GDBusNodeInfo *introspection = NULL;
    GError *err = NULL;

    introspection = g_dbus_node_info_new_for_xml(agent_introspection_xml, &err);
    if (!introspection) {
        if (error) *error = err;
        else g_error_free(err);
        return FALSE;
    }

    /* Register object */
    registration_id = g_dbus_connection_register_object(conn,
                                                        AGENT_PATH,
                                                        introspection->interfaces[0],
                                                        &(const GDBusInterfaceVTable){
                                                            .method_call = agent_method_call,
                                                            .get_property = NULL,
                                                            .set_property = NULL
                                                        },
                                                        NULL, /* user_data */
                                                        NULL, /* user_data_free_func */
                                                        &err);
    g_dbus_node_info_unref(introspection);
    if (registration_id == 0) {
        if (error) *error = err;
        else if (err) g_error_free(err);
        return FALSE;
    }
    g_info("bt_agent: exported agent object at %s (id=%u)", AGENT_PATH, registration_id);
    return TRUE;
}

/* Call org.bluez.AgentManager1.RegisterAgent and RequestDefault if available */
static gboolean register_agent_with_bluez(GDBusConnection *conn, GError **error) {
    GError *err = NULL;
    /* Call RegisterAgent(path, capability) */
    GVariant *res = g_dbus_connection_call_sync(conn,
                                                "org.bluez",
                                                "/org/bluez",
                                                "org.bluez.AgentManager1",
                                                "RegisterAgent",
                                                g_variant_new("(os)", AGENT_PATH, "KeyboardDisplay"),
                                                NULL,
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &err);
    if (!res) {
        if (error) *error = err;
        else g_error_free(err);
        return FALSE;
    }
    g_variant_unref(res);
    /* Try RequestDefault or RegisterDefault depending on BlueZ version */
    err = NULL;
    res = g_dbus_connection_call_sync(conn,
                                      "org.bluez",
                                      "/org/bluez",
                                      "org.bluez.AgentManager1",
                                      "RequestDefaultAgent",
                                      g_variant_new("(o)", AGENT_PATH),
                                      NULL,
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      &err);
    if (!res) {
        /* RequestDefaultAgent may not be available; try RegisterDefault (older) */
        if (err) {
            g_info("bt_agent: RequestDefaultAgent failed (will try RegisterDefault): %s", err->message);
            g_error_free(err);
            err = NULL;
        }
        res = g_dbus_connection_call_sync(conn,
                                          "org.bluez",
                                          "/org/bluez",
                                          "org.bluez.AgentManager1",
                                          "RegisterDefault",
                                          g_variant_new("(o)", AGENT_PATH),
                                          NULL,
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &err);
        if (!res) {
            if (error) *error = err;
            else if (err) g_error_free(err);
            /* Not fatal; agent is registered but setting default failed */
            return TRUE;
        }
        g_variant_unref(res);
    } else {
        g_variant_unref(res);
    }
    return TRUE;
}

/* Public: register the agent on system bus */
gboolean bt_agent_register(GDBusConnection *conn, GError **error) {
    g_return_val_if_fail(conn != NULL, FALSE);
    agent_conn = conn;
    GError *err = NULL;
    if (!export_agent_object(conn, &err)) {
        if (error) *error = err;
        else g_error_free(err);
        return FALSE;
    }
    if (!register_agent_with_bluez(conn, &err)) {
        /* non-fatal: cleanup exported object */
        g_dbus_connection_unregister_object(conn, registration_id);
        registration_id = 0;
        if (error) *error = err;
        else g_error_free(err);
        return FALSE;
    }
    g_info("bt_agent: successfully registered with BlueZ");
    return TRUE;
}

/* Public: unregister agent */
void bt_agent_unregister(GDBusConnection *conn) {
    (void)conn;
    if (agent_conn && registration_id != 0) {
        g_dbus_connection_unregister_object(agent_conn, registration_id);
        registration_id = 0;
        g_info("bt_agent: unregistered agent object");
    }
    agent_conn = NULL;
}
/*
 * src/gui_bt.c
 *
 * GUI Bluetooth integration helpers.
 *
 * This file implements Adapter1 / Device1 D-Bus calls used by the GUI:
 *  - Start/Stop discovery on a given adapter
 *  - Pair, Connect and RemoveDevice for a device
 *  - Mark device "Trusted" via Properties.Set on org.bluez.Device1
 *
 * Notes:
 *  - device_path_or_mac accepts either a BlueZ object path (e.g. /org/bluez/hci0/dev_XX_XX_...)
 *    or a colon-separated MAC string (AA:BB:CC:DD:EE:FF). When a MAC is provided we
 *    convert it to the object path assuming hci0 adapter.
 *  - These helpers are synchronous and return 0 on success, -1 on failure.
 *  - Integrate with your GUI main loop and present user-visible dialogs on errors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Forward declarations for agent registration (from bt_agent.c) */
gboolean bt_agent_register(GDBusConnection *conn, GError **error);
void bt_agent_unregister(GDBusConnection *conn);
/* Forward declaration to silence implicit warning in gui_bt_shutdown() */
int gui_bt_unregister_discovery_listeners(void);

/* Cached system bus connection used by GUI helpers (non-owning) */
static GDBusConnection *gui_system_bus = NULL;
static gboolean agent_registered = FALSE;

/* Bound GTK model for device list (explicit binding from GUI) */
static GtkTreeView *s_tree = NULL;
static GtkListStore *s_store = NULL;

/* Idempotent listener registration guard */
static gboolean s_listeners_registered = FALSE;

/* Helper: convert MAC "AA:BB:CC:DD:EE:FF" to BlueZ object path "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF" */
static gchar *mac_to_object_path(const char *mac) {
    if (!mac) return NULL;
    gchar *dup = g_ascii_strup(mac, -1); /* uppercase for consistency */
    for (gchar *p = dup; *p; ++p) if (*p == ':') *p = '_';
    gchar *path = g_strdup_printf("/org/bluez/hci0/dev_%s", dup);
    g_free(dup);
    return path;
}

/* Helper: ensure we have a system bus connection */
static gboolean ensure_system_bus(GError **error) {
    if (gui_system_bus) return TRUE;
    gui_system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, error);
    if (!gui_system_bus) return FALSE;
    return TRUE;
}

/* Initialize GUI bluetooth panel. Call from main GUI init.
   This will obtain a system bus connection and register the BlueZ agent.
   Return 0 on success. */
int gui_bt_init(void) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_init: failed to get system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }

    if (!bt_agent_register(gui_system_bus, &err)) {
        fprintf(stderr, "gui_bt_init: bt_agent_register failed: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        agent_registered = FALSE;
    } else {
        agent_registered = TRUE;
    }
    return 0;
}

/* Shutdown / cleanup GUI bluetooth panel. */
void gui_bt_shutdown(void) {
    /* Unregister listeners first (idempotent) */
    gui_bt_unregister_discovery_listeners();

    /* Drop strong refs to bound store/tree */
    if (s_store) { g_object_unref(s_store); s_store = NULL; }
    if (s_tree)  { g_object_unref(s_tree);  s_tree  = NULL; }

    if (agent_registered && gui_system_bus) {
        bt_agent_unregister(gui_system_bus);
        agent_registered = FALSE;
    }
    if (gui_system_bus) {
        g_object_unref(gui_system_bus);
        gui_system_bus = NULL;
    }
}

/* Start discovery on adapter (adapter_path or default hci0 if NULL) - async, non-blocking */
int gui_bt_start_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_start_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    const char *adapter = adapter_path ? adapter_path : "/org/bluez/hci0";
    g_dbus_connection_call(gui_system_bus,
                           "org.bluez",
                           adapter,
                           "org.bluez.Adapter1",
                           "StartDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    return 0;
}

/* Stop discovery on adapter (adapter_path or default hci0 if NULL) - async, non-blocking */
int gui_bt_stop_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_stop_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    const char *adapter = adapter_path ? adapter_path : "/org/bluez/hci0";
    g_dbus_connection_call(gui_system_bus,
                           "org.bluez",
                           adapter,
                           "org.bluez.Adapter1",
                           "StopDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    return 0;
}

/* Pair with device. device_path_or_mac can be object path or MAC. */
int gui_bt_pair_device(const char *device_path_or_mac) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_pair_device: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) {
        device_path = g_strdup(device_path_or_mac);
    } else {
        device_path = mac_to_object_path(device_path_or_mac);
    }
    if (!device_path) return -1;

    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               device_path,
                                               "org.bluez.Device1",
                                               "Pair",
                                               NULL,
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "gui_bt_pair_device: Pair failed for %s: %s\n", device_path, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(device_path);
        return -1;
    }
    g_variant_unref(res);
    g_free(device_path);
    return 0;
}

/* Connect device (Device1.Connect). */
int gui_bt_connect_device(const char *device_path_or_mac) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_connect_device: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) {
        device_path = g_strdup(device_path_or_mac);
    } else {
        device_path = mac_to_object_path(device_path_or_mac);
    }
    if (!device_path) return -1;

    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               device_path,
                                               "org.bluez.Device1",
                                               "Connect",
                                               NULL,
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "gui_bt_connect_device: Connect failed for %s: %s\n", device_path, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(device_path);
        return -1;
    }
    g_variant_unref(res);
    g_free(device_path);
    return 0;
}

/* Set device Trusted property via org.freedesktop.DBus.Properties.Set on org.bluez.Device1 */
int gui_bt_trust_device(const char *device_path_or_mac, int trusted) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_trust_device: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) {
        device_path = g_strdup(device_path_or_mac);
    } else {
        device_path = mac_to_object_path(device_path_or_mac);
    }
    if (!device_path) return -1;

    GVariant *value = g_variant_new_variant(g_variant_new_boolean(trusted ? TRUE : FALSE));
    GVariant *params = g_variant_new("(ssv)", "org.bluez.Device1", "Trusted", value);

    /* Properties.Set must be invoked on the owner of the object (org.bluez).
       Call the org.freedesktop.DBus.Properties Set method on the device object
       with destination "org.bluez". */
    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               device_path,
                                               "org.freedesktop.DBus.Properties",
                                               "Set",
                                               params,
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "gui_bt_trust_device: Properties.Set failed for %s: %s\n", device_path, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(device_path);
        return -1;
    }
    g_variant_unref(res);
    g_free(device_path);
    return 0;
}

/* Remove/unpair device: call Adapter1.RemoveDevice(adapter, device) */
int bluez_remove_device(const char *device_path_or_mac) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "bluez_remove_device: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) {
        device_path = g_strdup(device_path_or_mac);
    } else {
        device_path = mac_to_object_path(device_path_or_mac);
    }
    if (!device_path) return -1;

    /* Derive adapter path by truncating at "/dev_" */
    gchar *devpos = strstr(device_path, "/dev_");
    gchar *adapter_path = NULL;
    if (devpos) {
        size_t len = (size_t)(devpos - device_path);
        adapter_path = g_strndup(device_path, len);
    } else {
        adapter_path = g_strdup("/org/bluez/hci0");
    }

    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               adapter_path,
                                               "org.bluez.Adapter1",
                                               "RemoveDevice",
                                               g_variant_new("(o)", device_path),
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "bluez_remove_device: RemoveDevice failed for %s on %s: %s\n", device_path, adapter_path, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(adapter_path);
        g_free(device_path);
        return -1;
    }
    g_variant_unref(res);
    g_free(adapter_path);
    g_free(device_path);
    return 0;
}

/* Toggle routing for a device (A2DP sink/source or SCO). This should update
   /etc/jack-bridge/bluetooth.conf or call an IPC endpoint on the autobridge.
   'route' is a small string like "a2dp_sink", "a2dp_source", "sco".
*/
int gui_bt_set_route(const char *mac, const char *route, int enabled) {
    if (!mac || !route) return -1;
    fprintf(stderr, "gui_bt_set_route: mac=%s route=%s enabled=%d (placeholder - implement persistence or IPC)\n",
            mac, route, enabled);
    return 0;
}
/* GUI-side helpers to let the GUI expose a GtkTreeView's store for gui_bt to update.
   These functions are minimal and intended for use by the GUI (mxeq.c). They
   will be called from the main thread. */

int gui_bt_attach_device_store_widget(GtkWidget *treeview) {
    if (!GTK_IS_TREE_VIEW(treeview)) return -1;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    if (!model) return -1;
    g_object_set_data(G_OBJECT(treeview), "gui_bt_attached", GINT_TO_POINTER(1));
    return 0;
}

/* New explicit binder used by GUI to set the device list store. Keeps strong refs. */
int gui_bt_set_device_store_widget(GtkWidget *treeview, GtkListStore *store) {
    if (!GTK_IS_TREE_VIEW(treeview) || !GTK_IS_LIST_STORE(store)) return -1;

    if (s_store) { g_object_unref(s_store); s_store = NULL; }
    if (s_tree)  { g_object_unref(s_tree);  s_tree  = NULL; }

    s_tree  = GTK_TREE_VIEW(g_object_ref(treeview));
    s_store = GTK_LIST_STORE(g_object_ref(store));
    return 0;
}

/* Append or update a device row (display, object_path) on any attached tree.
   This looks for a tree widget that exposes "device_store" data (set by mxeq.c).
   Returns 0 on success, -1 on failure.
*/
int gui_bt_add_device(const char *display, const char *object_path) {
    if (!display || !object_path) return -1;
    if (!s_store) return -1;

    /* Update if exists */
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(s_store), &iter);
    while (valid) {
        gchar *obj = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(s_store), &iter, 1, &obj, -1);
        if (obj && g_strcmp0(obj, object_path) == 0) {
            gtk_list_store_set(s_store, &iter, 0, display, -1);
            g_free(obj);
            return 0;
        }
        if (obj) g_free(obj);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(s_store), &iter);
    }

    /* Append new */
    GtkTreeIter newiter;
    gtk_list_store_append(s_store, &newiter);
    gtk_list_store_set(s_store, &newiter, 0, display, 1, object_path, -1);
    return 0;
}

/* Remove device row(s) matching object_path */
int gui_bt_remove_device_by_object(const char *object_path) {
    if (!object_path) return -1;
    if (!s_store) return -1;

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(s_store), &iter);
    while (valid) {
        gchar *obj = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(s_store), &iter, 1, &obj, -1);
        if (obj && g_strcmp0(obj, object_path) == 0) {
            gtk_list_store_remove(s_store, &iter);
            if (obj) g_free(obj);
            return 0;
        }
        if (obj) g_free(obj);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(s_store), &iter);
    }
    return -1;
}

/* GUI-side discovery callbacks: subscribe to org.bluez ObjectManager signals and forward
   discovered devices into the GUI list via gui_bt_add_device / gui_bt_remove_device.
   This code runs in the main thread context when possible; signal handlers schedule
   g_idle_add to ensure GTK updates happen on the main loop.
*/

#include <gio/gio.h>

/* Forward declarations of GUI helpers in this file */
int gui_bt_add_device(const char *display, const char *object_path);
int gui_bt_remove_device_by_object(const char *object_path);

static guint bluez_interfaces_added_sub = 0;
static guint bluez_interfaces_removed_sub = 0;

/* Handler invoked on the GLib main loop to add a device */
static gboolean __gui_bt_add_device_idle(gpointer data) {
    char **parts = data;
    gui_bt_add_device(parts[0], parts[1]);
    g_free(parts[0]);
    g_free(parts[1]);
    g_free(parts);
    return G_SOURCE_REMOVE;
}

/* Handler invoked on the GLib main loop to remove a device from GUI list only */
static gboolean __gui_bt_remove_device_idle(gpointer data) {
    char *obj = data;
    gui_bt_remove_device_by_object(obj);
    g_free(obj);
    return G_SOURCE_REMOVE;
}

/* D-Bus InterfacesAdded callback for BlueZ (ObjectManager) */
static void bluez_interfaces_added_for_gui(GDBusConnection *connection,
                                          const gchar *sender_name,
                                          const gchar *object_path,
                                          const gchar *interface_name,
                                          const gchar *signal_name,
                                          GVariant *parameters,
                                          gpointer user_data)
{
    (void)connection; (void)sender_name; (void)object_path; (void)interface_name; (void)user_data; (void)signal_name;

    /* Unpack (o a{sa{sv}}) and look for org.bluez.Device1 */
    gchar *path = NULL;
    GVariantIter *interfaces = NULL;

    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(oa{sa{sv}})")))
        return;

    g_variant_get(parameters, "(oa{sa{sv}})", &path, &interfaces);
    if (!path) return;

    gchar *iface = NULL;
    GVariant *props = NULL;
    while (g_variant_iter_next(interfaces, "{sa{sv}}", &iface, &props)) {
        if (g_strcmp0(iface, "org.bluez.Device1") == 0) {
            /* try to read Name or Alias property from props */
            GVariantIter *piter = NULL;
            gchar *pkey = NULL;
            GVariant *pval = NULL;
            char *display = NULL;
            g_variant_get(props, "a{sv}", &piter);
            while (g_variant_iter_next(piter, "{sv}", &pkey, &pval)) {
                if (g_strcmp0(pkey, "Name") == 0 && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                    display = g_strdup(g_variant_get_string(pval, NULL));
                } else if (g_strcmp0(pkey, "Alias") == 0 && !display && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                    display = g_strdup(g_variant_get_string(pval, NULL));
                }
                if (pval) g_variant_unref(pval);
                g_free(pkey);
            }
            if (piter) g_variant_iter_free(piter);
            if (!display) {
                const gchar *last = strrchr(path, '/');
                display = last ? g_strdup(last + 1) : g_strdup(path);
            }

            /* Schedule GUI update on main loop */
            char **pair = g_new0(char*, 3);
            pair[0] = display;
            pair[1] = g_strdup(path);
            g_idle_add(__gui_bt_add_device_idle, pair);
        }
        if (props) g_variant_unref(props);
        g_free(iface);
    }

    if (interfaces) g_variant_iter_free(interfaces);
    g_free(path);
}

/* D-Bus InterfacesRemoved callback for BlueZ (ObjectManager) */
static void bluez_interfaces_removed_for_gui(GDBusConnection *connection,
                                            const gchar *sender_name,
                                            const gchar *object_path,
                                            const gchar *interface_name,
                                            const gchar *signal_name,
                                            GVariant *parameters,
                                            gpointer user_data)
{
    (void)connection; (void)sender_name; (void)object_path; (void)interface_name; (void)user_data; (void)signal_name;

    /* Prefer (oas) signature */
    gchar *path = NULL;
    GVariantIter *ifaces = NULL;

    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(oas)"))) {
        g_variant_get(parameters, "(oas)", &path, &ifaces);
        if (ifaces) g_variant_iter_free(ifaces);
    } else if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(o@as)"))) {
        g_variant_get(parameters, "(o@as)", &path, NULL);
    } else if (g_variant_n_children(parameters) > 0) {
        GVariant *child = g_variant_get_child_value(parameters, 0);
        if (child && g_variant_is_of_type(child, G_VARIANT_TYPE_STRING)) {
            path = g_strdup(g_variant_get_string(child, NULL));
        }
        if (child) g_variant_unref(child);
    }

    if (path) {
        g_idle_add(__gui_bt_remove_device_idle, g_strdup(path));
        g_free(path);
    }
}

/* Populate existing BlueZ devices (Device1) into the bound store */
void gui_bt_populate_existing_devices(void) {
    if (!s_store) return;

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        if (err) g_error_free(err);
        return;
    }

    GVariant *managed = g_dbus_connection_call_sync(gui_system_bus,
                                                    "org.bluez",
                                                    "/",
                                                    "org.freedesktop.DBus.ObjectManager",
                                                    "GetManagedObjects",
                                                    NULL,
                                                    G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
                                                    G_DBUS_CALL_FLAGS_NONE,
                                                    -1,
                                                    NULL,
                                                    &err);
    if (!managed) {
        if (err) {
            g_warning("GetManagedObjects failed: %s", err->message);
            g_error_free(err);
        }
        return;
    }

    GVariantIter *outer = NULL;
    g_variant_get(managed, "(a{oa{sa{sv}}})", &outer);
    gchar *path = NULL;
    GVariantIter *iface_map = NULL;

    while (g_variant_iter_next(outer, "{oa{sa{sv}}}", &path, &iface_map)) {
        gchar *iface_name = NULL;
        GVariantIter *props = NULL;
        while (g_variant_iter_next(iface_map, "{sa{sv}}", &iface_name, &props)) {
            if (g_strcmp0(iface_name, "org.bluez.Device1") == 0) {
                gchar *pkey = NULL;
                GVariant *pval = NULL;
                char *display = NULL;
                while (g_variant_iter_next(props, "{sv}", &pkey, &pval)) {
                    if (!display && g_strcmp0(pkey, "Name") == 0 && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                        display = g_strdup(g_variant_get_string(pval, NULL));
                    } else if (!display && g_strcmp0(pkey, "Alias") == 0 && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                        display = g_strdup(g_variant_get_string(pval, NULL));
                    }
                    if (pval) g_variant_unref(pval);
                    g_free(pkey);
                }
                if (!display) {
                    const gchar *last = strrchr(path, '/');
                    display = last ? g_strdup(last + 1) : g_strdup(path);
                }
                gui_bt_add_device(display, path);
                g_free(display);
            }
            if (props) g_variant_iter_free(props);
            g_free(iface_name);
        }
        if (iface_map) g_variant_iter_free(iface_map);
        g_free(path);
    }
    g_variant_iter_free(outer);
    g_variant_unref(managed);
}

/* Start listening for BlueZ ObjectManager signals to populate GUI list (idempotent) */
int gui_bt_register_discovery_listeners(void) {
    if (s_listeners_registered) return 0;

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        if (err) g_error_free(err);
        return -1;
    }

    bluez_interfaces_added_sub = g_dbus_connection_signal_subscribe(gui_system_bus,
        "org.bluez",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        bluez_interfaces_added_for_gui,
        NULL,
        NULL);

    bluez_interfaces_removed_sub = g_dbus_connection_signal_subscribe(gui_system_bus,
        "org.bluez",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesRemoved",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        bluez_interfaces_removed_for_gui,
        NULL,
        NULL);

    s_listeners_registered = TRUE;
    return 0;
}

/* Unregister listeners */
int gui_bt_unregister_discovery_listeners(void) {
    if (bluez_interfaces_added_sub) {
        g_dbus_connection_signal_unsubscribe(gui_system_bus, bluez_interfaces_added_sub);
        bluez_interfaces_added_sub = 0;
    }
    if (bluez_interfaces_removed_sub) {
        g_dbus_connection_signal_unsubscribe(gui_system_bus, bluez_interfaces_removed_sub);
        bluez_interfaces_removed_sub = 0;
    }
    s_listeners_registered = FALSE;
    return 0;
}
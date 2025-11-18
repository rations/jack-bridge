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

/* Forward declarations for agent registration (from bt_agent.c) */
gboolean bt_agent_register(GDBusConnection *conn, GError **error);
void bt_agent_unregister(GDBusConnection *conn);

/* Cached system bus connection used by GUI helpers (non-owning) */
static GDBusConnection *gui_system_bus = NULL;
static gboolean agent_registered = FALSE;

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
    if (agent_registered && gui_system_bus) {
        bt_agent_unregister(gui_system_bus);
        agent_registered = FALSE;
    }
    if (gui_system_bus) {
        g_object_unref(gui_system_bus);
        gui_system_bus = NULL;
    }
}

/* Start discovery on adapter (adapter_path or default hci0 if NULL) */
int gui_bt_start_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_start_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    const char *adapter = adapter_path ? adapter_path : "/org/bluez/hci0";
    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               adapter,
                                               "org.bluez.Adapter1",
                                               "StartDiscovery",
                                               NULL,
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "gui_bt_start_discovery: StartDiscovery failed: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    g_variant_unref(res);
    return 0;
}

/* Stop discovery on adapter (adapter_path or default hci0 if NULL) */
int gui_bt_stop_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_stop_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    const char *adapter = adapter_path ? adapter_path : "/org/bluez/hci0";
    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                                               "org.bluez",
                                               adapter,
                                               "org.bluez.Adapter1",
                                               "StopDiscovery",
                                               NULL,
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               &err);
    if (!res) {
        fprintf(stderr, "gui_bt_stop_discovery: StopDiscovery failed: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    g_variant_unref(res);
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
int gui_bt_remove_device(const char *device_path_or_mac) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_remove_device: no system bus: %s\n", err ? err->message : "(unknown)");
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
        fprintf(stderr, "gui_bt_remove_device: RemoveDevice failed for %s on %s: %s\n", device_path, adapter_path, err ? err->message : "(unknown)");
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
    /* ensure model has two columns (display, object) */
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    if (!model) return -1;
    /* mark attached so other helpers can find it via widget data if needed */
    g_object_set_data(G_OBJECT(treeview), "gui_bt_attached", GINT_TO_POINTER(1));
    return 0;
}

/* Append or update a device row (display, object_path) on any attached tree.
   This looks for a tree widget that exposes "device_store" data (set by mxeq.c).
   Returns 0 on success, -1 on failure.
*/
int gui_bt_add_device(const char *display, const char *object_path) {
    if (!display || !object_path) return -1;
    /* Iterate toplevel windows and their children to find a tree with device_store */
    GList *toplevels = gtk_window_list_toplevels();
    for (GList *l = toplevels; l; l = l->next) {
        GtkWindow *w = GTK_WINDOW(l->data);
        if (!GTK_IS_WINDOW(w)) continue;
        /* recursively search widget tree - simple BFS */
        GQueue q;
        g_queue_init(&q);
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(w));
        if (child) g_queue_push_tail(&q, child);
        while (!g_queue_is_empty(&q)) {
            GtkWidget *wgt = g_queue_pop_head(&q);
            if (GTK_IS_TREE_VIEW(wgt)) {
                gpointer store_ptr = g_object_get_data(G_OBJECT(wgt), "device_store");
                if (store_ptr) {
                    GtkListStore *store = GTK_LIST_STORE(store_ptr);
                    GtkTreeIter iter;
                    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
                    while (valid) {
                        gchar *obj = NULL;
                        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &obj, -1);
                        if (obj && g_strcmp0(obj, object_path) == 0) {
                            /* update display */
                            gtk_list_store_set(store, &iter, 0, display, -1);
                            g_free(obj);
                            g_list_free(toplevels);
                            return 0;
                        }
                        if (obj) g_free(obj);
                        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
                    }
                    /* not found -> append */
                    GtkTreeIter newiter;
                    gtk_list_store_append(store, &newiter);
                    gtk_list_store_set(store, &newiter, 0, display, 1, object_path, -1);
                    g_list_free(toplevels);
                    return 0;
                }
            }
            if (GTK_IS_CONTAINER(wgt)) {
                GList *children = gtk_container_get_children(GTK_CONTAINER(wgt));
                for (GList *c = children; c; c = c->next) g_queue_push_tail(&q, c->data);
                if (children) g_list_free(children);
            }
        }
        g_queue_clear(&q);
    }
    if (toplevels) g_list_free(toplevels);
    return -1;
}

/* Remove device row(s) matching object_path */
int gui_bt_remove_device_by_object(const char *object_path) {
    if (!object_path) return -1;
    GList *toplevels = gtk_window_list_toplevels();
    for (GList *l = toplevels; l; l = l->next) {
        GtkWindow *w = GTK_WINDOW(l->data);
        if (!GTK_IS_WINDOW(w)) continue;
        GQueue q;
        g_queue_init(&q);
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(w));
        if (child) g_queue_push_tail(&q, child);
        while (!g_queue_is_empty(&q)) {
            GtkWidget *wgt = g_queue_pop_head(&q);
            if (GTK_IS_TREE_VIEW(wgt)) {
                gpointer store_ptr = g_object_get_data(G_OBJECT(wgt), "device_store");
                if (store_ptr) {
                    GtkListStore *store = GTK_LIST_STORE(store_ptr);
                    GtkTreeIter iter;
                    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
                    while (valid) {
                        gchar *obj = NULL;
                        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &obj, -1);
                        if (obj && g_strcmp0(obj, object_path) == 0) {
                            gtk_list_store_remove(store, &iter);
                            if (obj) g_free(obj);
                            g_list_free(toplevels);
                            return 0;
                        }
                        if (obj) g_free(obj);
                        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
                    }
                }
            }
            if (GTK_IS_CONTAINER(wgt)) {
                GList *children = gtk_container_get_children(GTK_CONTAINER(wgt));
                for (GList *c = children; c; c = c->next) g_queue_push_tail(&q, c->data);
                if (children) g_list_free(children);
            }
        }
        g_queue_clear(&q);
    }
    if (toplevels) g_list_free(toplevels);
    return -1;
}
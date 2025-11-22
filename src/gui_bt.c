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

/* Ensure any device names/aliases we put into GTK are valid UTF-8.
 * Non-UTF8 strings coming from BlueZ (e.g., BLE advertisements with raw bytes)
 * can trigger GTK assertions/crashes. This helper guarantees a valid string. */
static gchar* safe_utf8(const char *s) {
    if (!s) return NULL;
    if (g_utf8_validate(s, -1, NULL)) return g_strdup(s);
#if GLIB_CHECK_VERSION(2,52,0)
    /* Use -1 to indicate null-terminated string; g_utf8_make_valid takes (const gchar*, gssize). */
    return g_utf8_make_valid(s, -1);
#else
    /* Fallback for older GLib: replace non-ASCII bytes with '?' */
    GString *out = g_string_new(NULL);
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        if (*p < 0x80) g_string_append_c(out, (char)*p);
        else g_string_append_c(out, '?');
    }
    return g_string_free(out, FALSE);
#endif
}

/* Strip any previously appended state markers from a device label, e.g.
 * "My Device [Paired] [Trusted] [Connected]" -> "My Device".
 * Returns a newly-allocated string the caller must free. */
static char* strip_state_markers(const char *s) {
    if (!s) return NULL;
    char *out = g_strdup(s);
    const char *toks[] = { " [Paired]", " [Trusted]", " [Connected]" };
    for (int i = 0; i < 3; i++) {
        const char *tok = toks[i];
        size_t toklen = strlen(tok);
        char *p;
        while ((p = g_strstr_len(out, -1, tok)) != NULL) {
            /* shift the tail left over the token (including trailing NUL) */
            memmove(p, p + toklen, strlen(p + toklen) + 1);
        }
    }
    return out;
}
 
/* Async operation callback for GUI (success flag, error message or NULL, user data) */
typedef void (*GuiBtOpCb)(gboolean success, const char *message, gpointer user_data);

/* Forward declarations for agent registration (from bt_agent.c) */
gboolean bt_agent_register(GDBusConnection *conn, GError **error);
void bt_agent_unregister(GDBusConnection *conn);
/* Forward declaration to silence implicit warning in gui_bt_shutdown() */
int gui_bt_unregister_discovery_listeners(void);

/* Cached system bus connection used by GUI helpers (non-owning) */
static GDBusConnection *gui_system_bus = NULL;
static gboolean agent_registered = FALSE;
/* Added: flag to prevent races during shutdown for outstanding async ops */
static gboolean g_shutting_down = FALSE;

/* Bound GTK model for device list (explicit binding from GUI) */
static GtkTreeView *s_tree = NULL;
static GtkListStore *s_store = NULL;

/* Idempotent listener registration guard */
static gboolean s_listeners_registered = FALSE;

/* Forward declarations for internal helpers defined later in this file */
static gchar *get_default_adapter_path(void);
static void update_device_row_state(const char *object_path);
static gboolean __gui_bt_update_device_idle(gpointer data);
static gboolean __gui_bt_refresh_selection_idle(gpointer data);
/* Forward decl so Start/Stop discovery can refresh Scan/Stop sensitivity immediately */
static void refresh_adapter_state(void);

/* Helper: convert MAC "AA:BB:CC:DD:EE:FF" to BlueZ object path using the default adapter */
static gchar *mac_to_object_path(const char *mac) {
    if (!mac) return NULL;
    gchar *dup = g_ascii_strup(mac, -1); /* uppercase for consistency */
    for (gchar *p = dup; *p; ++p) if (*p == ':') *p = '_';
    gchar *adapter = get_default_adapter_path();
    if (!adapter) {
        g_free(dup);
        return NULL;
    }
    gchar *path = g_strdup_printf("%s/dev_%s", adapter, dup);
    g_free(adapter);
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
/* Resolve default adapter path dynamically via ObjectManager (no hard-coded hci0) */
static gchar *get_default_adapter_path(void) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        if (err) { g_error_free(err); }
        return NULL;
    }
    g_message("DBG: get_default_adapter_path: calling GetManagedObjects with type '(a{oa{sa{sv}}})'");
    GVariant *managed = g_dbus_connection_call_sync(
        gui_system_bus,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &err
    );
    if (!managed) {
        if (err) {
            g_warning("get_default_adapter_path: GetManagedObjects failed: %s", err->message);
            g_error_free(err);
        }
        return NULL;
    }

    GVariantIter *outer = NULL;
    g_variant_get(managed, "(a{oa{sa{sv}}})", &outer);
    gchar *path = NULL;
    GVariant *ifaces = NULL;
    gchar *adapter_path = NULL;
 
    while (g_variant_iter_next(outer, "{o@a{sa{sv}}}", &path, &ifaces)) {
        gchar *iface_name = NULL;
        GVariant *props = NULL;
        gboolean is_adapter = FALSE;
        
        GVariantIter iiter;
        g_variant_iter_init(&iiter, ifaces);

        while (g_variant_iter_next(&iiter, "{s@a{sv}}", &iface_name, &props)) {
            if (g_strcmp0(iface_name, "org.bluez.Adapter1") == 0) {
                is_adapter = TRUE;
            }
            if (props) g_variant_unref(props);
            g_free(iface_name);
            if (is_adapter) break;
        }

        if (is_adapter) {
            adapter_path = g_strdup(path);
            if (ifaces) g_variant_unref(ifaces);
            g_free(path);
            break;
        }
        if (ifaces) g_variant_unref(ifaces);
        g_free(path);
    }
    if (outer) g_variant_iter_free(outer);
    g_variant_unref(managed);

    if (!adapter_path) {
        g_warning("get_default_adapter_path: no org.bluez.Adapter1 found");
    }
    return adapter_path;
}

/* Ensure adapter Powered=true; best-effort synchronous call for robust UX */
static int ensure_adapter_powered(const char *adapter_path) {
    if (!adapter_path) return -1;
    GError *err = NULL;

    /* Get current Powered state */
    g_message("DBG: ensure_adapter_powered Get Powered on %s", adapter_path);
    GVariant *res = g_dbus_connection_call_sync(
        gui_system_bus,
        "org.bluez",
        adapter_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.Adapter1", "Powered"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &err
    );
    if (!res) {
        if (err) {
            g_warning("ensure_adapter_powered(Get): %s", err->message);
            g_error_free(err);
        }
        return -1;
    }
    GVariant *v = NULL;
    g_variant_get(res, "(v)", &v);
    gboolean powered = FALSE;
    if (v && g_variant_is_of_type(v, G_VARIANT_TYPE_BOOLEAN))
        powered = g_variant_get_boolean(v);
    if (v) g_variant_unref(v);
    g_variant_unref(res);

    if (powered) return 0;

    /* Set Powered=true
     * Use a plain boolean variant for the 'v' slot to avoid double-boxing on
     * some GLib/BlueZ combinations that can cause "invalid signature". */
    GVariant *params = g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered", g_variant_new_boolean(TRUE));
    g_message("DBG: ensure_adapter_powered Set Powered=true on %s (params type=%s)", adapter_path, g_variant_get_type_string(params));
    res = g_dbus_connection_call_sync(
        gui_system_bus,
        "org.bluez",
        adapter_path,
        "org.freedesktop.DBus.Properties",
        "Set",
        params,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &err
    );
    if (!res) {
        if (err) {
            g_warning("ensure_adapter_powered(Set): %s", err->message);
            g_error_free(err);
        }
        return -1;
    }
    g_variant_unref(res);
    return 0;
}

/* Apply a discovery filter focused on classic audio devices (BR/EDR)
 *
 * Disabled: constructing and passing container variants here has caused
 * GVariant type/refcount corruption on some GLib versions (see g_variant_
 * assertions and aborts). StartDiscovery works without an explicit filter,
 * so skip SetDiscoveryFilter to avoid runtime crashes.
 */
static int set_discovery_filter_bredr(const char *adapter_path) {
    (void)adapter_path;
    return 0;
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
    /* Mark shutting down first so async callbacks can bail out without touching gui_system_bus */
    g_shutting_down = TRUE;

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



/* Start discovery on adapter (adapter_path or default adapter if NULL) - synchronous to surface errors */
int gui_bt_start_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_start_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }

    gchar *adapter = adapter_path ? g_strdup(adapter_path) : get_default_adapter_path();
    if (!adapter) {
        fprintf(stderr, "gui_bt_start_discovery: no BlueZ adapter found\n");
        return -1;
    }

    /* Ensure adapter is powered; discovery requires Powered=true */
    if (ensure_adapter_powered(adapter) != 0) {
        fprintf(stderr, "gui_bt_start_discovery: failed to power adapter %s\n", adapter);
        g_free(adapter);
        return -1;
    }

    /* Prefer BR/EDR for classic audio devices; best-effort */
    if (set_discovery_filter_bredr(adapter) != 0) {
        fprintf(stderr, "gui_bt_start_discovery: SetDiscoveryFilter failed on %s (continuing)\n", adapter);
    }

    /* Synchronous StartDiscovery so GUI can notify user on failure (polkit denial, etc.) */
    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                           "org.bluez",
                           adapter,
                           "org.bluez.Adapter1",
                           "StartDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           5000,
                           NULL,
                           &err);
    if (!res) {
        fprintf(stderr, "gui_bt_start_discovery: StartDiscovery failed on %s: %s\n", adapter, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(adapter);
        return -1;
    }
    g_variant_unref(res);

    /* Update scan/stop button sensitivity from current Adapter state */
    refresh_adapter_state();

    g_free(adapter);
    return 0;
}

/* Stop discovery on adapter (adapter_path or default adapter if NULL) - synchronous */
int gui_bt_stop_discovery(const char *adapter_path) {
    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        fprintf(stderr, "gui_bt_stop_discovery: no system bus: %s\n", err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        return -1;
    }
    gchar *adapter = adapter_path ? g_strdup(adapter_path) : get_default_adapter_path();
    if (!adapter) {
        fprintf(stderr, "gui_bt_stop_discovery: no BlueZ adapter found\n");
        return -1;
    }

    GVariant *res = g_dbus_connection_call_sync(gui_system_bus,
                           "org.bluez",
                           adapter,
                           "org.bluez.Adapter1",
                           "StopDiscovery",
                           NULL,
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           5000,
                           NULL,
                           &err);
    if (!res) {
        fprintf(stderr, "gui_bt_stop_discovery: StopDiscovery failed on %s: %s\n", adapter, err ? err->message : "(unknown)");
        if (err) g_error_free(err);
        g_free(adapter);
        return -1;
    }
    g_variant_unref(res);

    /* Update scan/stop button sensitivity from current Adapter state */
    refresh_adapter_state();

    g_free(adapter);
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
                                               20000,
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
                                               15000,
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

    /* Build params: pass boolean directly (GLib will box where required) to avoid
     * invalid-signature on some systems. */
    GVariant *params = g_variant_new("(ssv)", "org.bluez.Device1", "Trusted", g_variant_new_boolean(trusted ? TRUE : FALSE));
    /* Debug: print param type & contents for diagnosis */
    {
        gchar *dump = g_variant_print(params, TRUE);
        g_message("DBG: gui_bt_trust_device: params type=%s value=%s", g_variant_get_type_string(params), dump);
        g_free(dump);
    }
 
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
                                               15000,
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

/* ---------- Async Bluetooth Ops (Pair/Connect/Trust) ---------- */

typedef struct {
    char *device_path;
    GuiBtOpCb cb;
    gpointer ud;
    int tried_profile; /* 0 = none, 1 = tried ConnectProfile, 2 = tried fallback Connect */
} BtOpCtx;

static gboolean __invoke_cb_idle(gpointer data) {
    /* data layout: gboolean success; char* message; GuiBtOpCb cb; gpointer ud */
    gpointer *arr = data;
    gboolean success = GPOINTER_TO_INT(arr[0]);
    char *msg = arr[1];
    GuiBtOpCb cb = (GuiBtOpCb)arr[2];
    gpointer ud = arr[3];
    if (cb) cb(success, msg, ud);
    if (msg) g_free(msg);
    g_free(arr);
    return G_SOURCE_REMOVE;
}

static void invoke_cb_main(GuiBtOpCb cb, gboolean success, const char *message, gpointer ud) {
    gpointer *arr = g_new0(gpointer, 4);
    arr[0] = GINT_TO_POINTER(success ? 1 : 0);
    arr[1] = message ? g_strdup(message) : NULL;
    arr[2] = (gpointer)cb;
    arr[3] = ud;
    g_idle_add(__invoke_cb_idle, arr);
}

static void bt_op_ctx_free(BtOpCtx *ctx) {
    if (!ctx) return;
    if (ctx->device_path) g_free(ctx->device_path);
    g_free(ctx);
}

static gchar *compose_hint_message(const char *prefix, const char *detail) {
    return g_strdup_printf(
        "%s%s%s\n\nHints:\n"
        "• Ensure your user is in 'audio' (and 'bluetooth' if present) groups\n"
        "• Verify polkit rule exists: /etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules\n"
        "• Make sure adapter is Powered and device is in range\n",
        prefix ? prefix : "",
        (prefix && detail) ? ": " : "",
        detail ? detail : ""
    );
}

/* ---- Pair Async ---- */
static void pair_call_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    BtOpCtx *ctx = (BtOpCtx*)user_data;

    /* If shutting down, avoid touching the system bus or invoking callbacks */
    if (g_shutting_down) {
        bt_op_ctx_free(ctx);
        return;
    }

    GError *err = NULL;
    GDBusConnection *conn = G_DBUS_CONNECTION(source_object);
    GVariant *r = g_dbus_connection_call_finish(conn, res, &err);
    if (!r) {
        gchar *msg = compose_hint_message("Pair failed", err ? err->message : "unknown error");
        if (err) g_error_free(err);
        invoke_cb_main(ctx->cb, FALSE, msg, ctx->ud);
        g_free(msg);
        bt_op_ctx_free(ctx);
        return;
    }
    g_variant_unref(r);
    /* Refresh row state and selection after success */
    update_device_row_state(ctx->device_path);
    g_idle_add(__gui_bt_refresh_selection_idle, NULL);
    invoke_cb_main(ctx->cb, TRUE, NULL, ctx->ud);
    bt_op_ctx_free(ctx);
}

int gui_bt_pair_device_async(const char *device_path_or_mac, GuiBtOpCb cb, gpointer ud) {
    if (g_shutting_down) {
        if (cb) invoke_cb_main(cb, FALSE, "Shutting down", ud);
        return -1;
    }

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        gchar *msg = compose_hint_message("System bus error", err ? err->message : "unknown");
        if (err) g_error_free(err);
        invoke_cb_main(cb, FALSE, msg, ud);
        g_free(msg);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) device_path = g_strdup(device_path_or_mac);
    else device_path = mac_to_object_path(device_path_or_mac);
    if (!device_path) {
        invoke_cb_main(cb, FALSE, "Invalid device path/MAC", ud);
        return -1;
    }

    /* Best-effort: stop discovery before pairing */
    {
        gchar *devpos = strstr(device_path, "/dev_");
        gchar *adapter_path2 = NULL;
        if (devpos) {
            size_t len = (size_t)(devpos - device_path);
            adapter_path2 = g_strndup(device_path, len);
        } else {
            adapter_path2 = get_default_adapter_path();
        }
        if (adapter_path2) {
            GError *err2 = NULL;
            GVariant *r2 = g_dbus_connection_call_sync(
                gui_system_bus,
                "org.bluez",
                adapter_path2,
                "org.bluez.Adapter1",
                "StopDiscovery",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                2000,
                NULL,
                &err2
            );
            if (!r2) { if (err2) g_error_free(err2); }
            else g_variant_unref(r2);
            g_free(adapter_path2);
        }
    }

    BtOpCtx *ctx = g_new0(BtOpCtx, 1);
    ctx->device_path = device_path;
    ctx->cb = cb; ctx->ud = ud;

    g_dbus_connection_call(gui_system_bus,
        "org.bluez",
        ctx->device_path,
        "org.bluez.Device1",
        "Pair",
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        20000,
        NULL,
        pair_call_done,
        ctx);
    return 0;
}

/* ---- Connect Async ---- */
static void connect_call_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    BtOpCtx *ctx = (BtOpCtx*)user_data;

    if (g_shutting_down) {
        bt_op_ctx_free(ctx);
        return;
    }

    GError *err = NULL;
    GDBusConnection *conn = G_DBUS_CONNECTION(source_object);
    GVariant *r = g_dbus_connection_call_finish(conn, res, &err);
    if (!r) {
        /* If we attempted ConnectProfile first, fall back to generic Connect once */
        if (ctx->tried_profile == 1) {
            /* clear error and try Connect (fallback) */
            if (err) { g_error_free(err); err = NULL; }
            ctx->tried_profile = 2;
            g_dbus_connection_call(gui_system_bus,
                "org.bluez",
                ctx->device_path,
                "org.bluez.Device1",
                "Connect",
                NULL,
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                20000,
                NULL,
                connect_call_done,
                ctx);
            return;
        }

        gchar *msg = compose_hint_message("Connect failed", err ? err->message : "unknown error");
        if (err) g_error_free(err);
        invoke_cb_main(ctx->cb, FALSE, msg, ctx->ud);
        g_free(msg);
        bt_op_ctx_free(ctx);
        return;
    }
    g_variant_unref(r);
    /* Refresh row state and selection after success */
    update_device_row_state(ctx->device_path);
    g_idle_add(__gui_bt_refresh_selection_idle, NULL);
    invoke_cb_main(ctx->cb, TRUE, NULL, ctx->ud);
    bt_op_ctx_free(ctx);
}

int gui_bt_connect_device_async(const char *device_path_or_mac, GuiBtOpCb cb, gpointer ud) {
    if (g_shutting_down) {
        if (cb) invoke_cb_main(cb, FALSE, "Shutting down", ud);
        return -1;
    }

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        gchar *msg = compose_hint_message("System bus error", err ? err->message : "unknown");
        if (err) g_error_free(err);
        invoke_cb_main(cb, FALSE, msg, ud);
        g_free(msg);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) device_path = g_strdup(device_path_or_mac);
    else device_path = mac_to_object_path(device_path_or_mac);
    if (!device_path) {
        invoke_cb_main(cb, FALSE, "Invalid device path/MAC", ud);
        return -1;
    }
    BtOpCtx *ctx = g_new0(BtOpCtx, 1);
    ctx->device_path = device_path;
    ctx->cb = cb; ctx->ud = ud;
    ctx->tried_profile = 1; /* will attempt ConnectProfile first */

    /* Try ConnectProfile for A2DP Sink (headsets) first; fallback to Connect in callback */
    const char *a2dp_sink_uuid = "0000110b-0000-1000-8000-00805f9b34fb";
    GVariant *params = g_variant_new("(s)", a2dp_sink_uuid);

    g_dbus_connection_call(gui_system_bus,
        "org.bluez",
        ctx->device_path,
        "org.bluez.Device1",
        "ConnectProfile",
        params,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        20000,
        NULL,
        connect_call_done,
        ctx);

    return 0;
}

/* ---- Trust Async (set Trusted=true) ---- */
static void trust_call_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    BtOpCtx *ctx = (BtOpCtx*)user_data;

    if (g_shutting_down) {
        bt_op_ctx_free(ctx);
        return;
    }

    GError *err = NULL;
    GDBusConnection *conn = G_DBUS_CONNECTION(source_object);
    GVariant *r = g_dbus_connection_call_finish(conn, res, &err);
    if (!r) {
        gchar *msg = compose_hint_message("Trust failed", err ? err->message : "unknown error");
        if (err) g_error_free(err);
        invoke_cb_main(ctx->cb, FALSE, msg, ctx->ud);
        g_free(msg);
        bt_op_ctx_free(ctx);
        return;
    }
    g_variant_unref(r);
    /* Refresh row state and selection after success */
    update_device_row_state(ctx->device_path);
    g_idle_add(__gui_bt_refresh_selection_idle, NULL);
    invoke_cb_main(ctx->cb, TRUE, NULL, ctx->ud);
    bt_op_ctx_free(ctx);
}

int gui_bt_trust_device_async(const char *device_path_or_mac, gboolean trusted, GuiBtOpCb cb, gpointer ud) {
    if (g_shutting_down) {
        if (cb) invoke_cb_main(cb, FALSE, "Shutting down", ud);
        return -1;
    }

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        gchar *msg = compose_hint_message("System bus error", err ? err->message : "unknown");
        if (err) g_error_free(err);
        invoke_cb_main(cb, FALSE, msg, ud);
        g_free(msg);
        return -1;
    }
    gchar *device_path = NULL;
    if (device_path_or_mac && strchr(device_path_or_mac, '/')) device_path = g_strdup(device_path_or_mac);
    else device_path = mac_to_object_path(device_path_or_mac);
    if (!device_path) {
        invoke_cb_main(cb, FALSE, "Invalid device path/MAC", ud);
        return -1;
    }

    BtOpCtx *ctx = g_new0(BtOpCtx, 1);
    ctx->device_path = device_path;
    ctx->cb = cb; ctx->ud = ud;

    /* Build params: pass boolean directly as above */
    GVariant *params = g_variant_new("(ssv)", "org.bluez.Device1", "Trusted", g_variant_new_boolean(trusted ? TRUE : FALSE));
    {
        gchar *dump = g_variant_print(params, TRUE);
        g_message("DBG: gui_bt_trust_device_async: params type=%s value=%s", g_variant_get_type_string(params), dump);
        g_free(dump);
    }
 
    g_dbus_connection_call(gui_system_bus,
        "org.bluez",
        ctx->device_path,
        "org.freedesktop.DBus.Properties",
        "Set",
        params,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        15000,
        NULL,
        trust_call_done,
        ctx);
    return 0;
}

/* ---------- Existing sync helper (used by selection gating) ---------- */
/* Query Device1 booleans for gating UI buttons; returns 0 on success, -1 on error */
int gui_bt_get_device_state(const char *object_path, gboolean *paired, gboolean *trusted, gboolean *connected) {
    GError *err = NULL;
    if (!object_path) return -1;
    if (!ensure_system_bus(&err)) {
        if (err) g_error_free(err);
        return -1;
    }
    GVariant *res = g_dbus_connection_call_sync(
        gui_system_bus,
        "org.bluez",
        object_path,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.bluez.Device1"),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        2000,
        NULL,
        &err
    );
    if (!res) {
        if (err) { g_debug("gui_bt_get_device_state(GetAll): %s", err->message); g_error_free(err); }
        return -1;
    }
    gboolean p = FALSE, t = FALSE, c = FALSE;
    GVariantIter *iter = NULL;
    g_variant_get(res, "(a{sv})", &iter);
    gchar *key = NULL;
    GVariant *val = NULL;
    while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
        if (g_strcmp0(key, "Paired") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            p = g_variant_get_boolean(val);
        else if (g_strcmp0(key, "Trusted") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            t = g_variant_get_boolean(val);
        else if (g_strcmp0(key, "Connected") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            c = g_variant_get_boolean(val);
        g_free(key);
        g_variant_unref(val);
    }
    if (iter) g_variant_iter_free(iter);
    g_variant_unref(res);
    if (paired)   *paired = p;
    if (trusted)  *trusted = t;
    if (connected)*connected = c;
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

/* Toggle routing for a device (A2DP sink/source or SCO).
   Note: autobridge has been removed. Routing is handled via JACK by
   /usr/local/lib/jack-bridge/jack-route-select and preferences persist in
   /etc/jack-bridge/devices.conf (e.g., PREFERRED_OUTPUT, BLUETOOTH_DEVICE).
   'route' tokens are presently unused here; kept as a placeholder.
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
static guint bluez_device_props_changed_sub = 0;
static guint bluez_adapter_props_changed_sub = 0;

/* Scan/Stop buttons bound from GUI for Adapter state updates */
static GtkWidget *s_scan_btn = NULL;
static GtkWidget *s_stop_btn = NULL;
static gboolean s_adapter_discovering = FALSE;
static gboolean s_adapter_powered = FALSE;

/* Handler invoked on the GLib main loop to add a device */
static gboolean __gui_bt_add_device_idle(gpointer data) {
    char **parts = data;
    gui_bt_add_device(parts[0], parts[1]);
    /* After adding, refresh row label with Paired/Trusted/Connected state */
    /* parts[1] is object_path */
    update_device_row_state(parts[1]);
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

/* Device1 PropertiesChanged handler (runs in D-Bus thread; schedule UI update on main loop) */
static void bluez_device_properties_changed_cb(GDBusConnection *c, const gchar *sender, const gchar *object_path,
                                               const gchar *interface_name, const gchar *signal_name,
                                               GVariant *parameters, gpointer user_data) {
    (void)c; (void)sender; (void)interface_name; (void)signal_name; (void)parameters; (void)user_data;
    if (object_path) {
        /* Update the device row label (Paired/Trusted/Connected) */
        g_idle_add(__gui_bt_update_device_idle, g_strdup(object_path));
        /* Also refresh any selection-driven buttons in the GUI (e.g. Pair/Connect gating) */
        g_idle_add(__gui_bt_refresh_selection_idle, NULL);
    }
}

/* Idle updater to refresh a device row display based on latest properties */
static gboolean __gui_bt_update_device_idle(gpointer data) {
    char *obj = data;
    update_device_row_state(obj);
    g_free(obj);
    return G_SOURCE_REMOVE;
}

/* Idle updater to refresh Scan/Stop button sensitivity from Adapter state */
static gboolean __gui_bt_update_scan_buttons_idle(gpointer data) {
    (void)data;
    if (!s_scan_btn || !s_stop_btn) return G_SOURCE_REMOVE;
    if (s_adapter_discovering) {
        gtk_widget_set_sensitive(s_scan_btn, FALSE);
        gtk_widget_set_sensitive(s_stop_btn, TRUE);
    } else {
        gtk_widget_set_sensitive(s_scan_btn, TRUE);
        gtk_widget_set_sensitive(s_stop_btn, FALSE);
    }
    return G_SOURCE_REMOVE;
}

/* Idle updater to refresh selection-driven buttons (emits 'changed' on selection) */
static gboolean __gui_bt_refresh_selection_idle(gpointer data) {
    (void)data;
    if (!s_tree) return G_SOURCE_REMOVE;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(s_tree));
    if (!sel) return G_SOURCE_REMOVE;
    /* Emit the "changed" signal to trigger selection handlers in the GUI */
    g_signal_emit_by_name(sel, "changed");
    return G_SOURCE_REMOVE;
}

/* refresh_selection_buttons removed — no longer used. The GUI now triggers
   selection-driven updates via the idle callback __gui_bt_refresh_selection_idle
   directly where needed. */

/* Query Adapter1 state (Powered, Discovering) and update globals */
static void refresh_adapter_state(void) {
    GError *err = NULL;
    gchar *adapter = get_default_adapter_path();
    if (!adapter) return;

    g_message("DBG: refresh_adapter_state GetAll on %s", adapter);
    GVariant *res = g_dbus_connection_call_sync(
        gui_system_bus,
        "org.bluez",
        adapter,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.bluez.Adapter1"),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        2000,
        NULL,
        &err
    );
    if (!res) {
        if (err) g_error_free(err);
        g_free(adapter);
        return;
    }

    gboolean powered = FALSE, discovering = FALSE;
    GVariantIter *iter = NULL;
    g_variant_get(res, "(a{sv})", &iter);
    gchar *key = NULL;
    GVariant *val = NULL;
    while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
        if (g_strcmp0(key, "Powered") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            powered = g_variant_get_boolean(val);
        else if (g_strcmp0(key, "Discovering") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            discovering = g_variant_get_boolean(val);
        g_free(key);
        g_variant_unref(val);
    }
    if (iter) g_variant_iter_free(iter);
    g_variant_unref(res);
    g_free(adapter);
 
    g_message("DBG: refresh_adapter_state Powered=%d Discovering=%d", powered ? 1 : 0, discovering ? 1 : 0);
    s_adapter_powered = powered;
    s_adapter_discovering = discovering;
    g_idle_add(__gui_bt_update_scan_buttons_idle, NULL);
}

/* Duplicate Device1 PropertiesChanged handler removed (handled by the primary
   bluez_device_properties_changed_cb defined earlier). */

/* Adapter1 PropertiesChanged handler: refresh scan/stop buttons */
static void bluez_adapter_properties_changed_cb(GDBusConnection *c, const gchar *sender, const gchar *object_path,
                                                const gchar *interface_name, const gchar *signal_name,
                                                GVariant *parameters, gpointer user_data) {
    (void)c; (void)sender; (void)object_path; (void)interface_name; (void)signal_name; (void)parameters; (void)user_data;
    refresh_adapter_state();
}

/* Bind Scan/Stop buttons so gui_bt can toggle sensitivity when Adapter Discovering changes */
int gui_bt_bind_scan_buttons(GtkWidget *scan_btn, GtkWidget *stop_btn) {
    s_scan_btn = scan_btn;
    s_stop_btn = stop_btn;
    /* Initialize from current adapter state */
    refresh_adapter_state();
    return 0;
}

/* Query Device1 properties and update the GUI label for the given object_path.
   Appends state markers [Paired], [Trusted], [Connected]. Preserves a leading
   "★ " prefix (used to tag Known devices populated at startup). */
/* ---- Asynchronous properties fetch + UI updater to avoid blocking GTK main loop ----
 *
 * We fetch Device1 GetAll via g_dbus_connection_call (async) and then schedule a
 * main-loop idle callback to update the GtkListStore. This prevents bursts of
 * PropertiesChanged signals from freezing or crashing the GUI.
 */

/* Data passed from async result into the idle updater */
typedef struct {
    char *object_path;
    gboolean paired;
    gboolean trusted;
    gboolean connected;
    char *name_or_alias; /* UTF-8 safe, newly allocated or NULL */
} DevProps;

/* Idle callback that applies parsed device properties into the device list */
static gboolean __gui_bt_apply_device_props_idle(gpointer ud) {
    DevProps *p = ud;
    if (!p) return G_SOURCE_REMOVE;
    if (!s_store) {
        if (p->object_path) g_free(p->object_path);
        if (p->name_or_alias) g_free(p->name_or_alias);
        g_free(p);
        return G_SOURCE_REMOVE;
    }

    /* Find current row and update label (preserve star prefix) */
    GtkTreeIter row;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(s_store), &row);
    while (valid) {
        gchar *obj = NULL;
        gchar *current = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(s_store), &row, 0, &current, 1, &obj, -1);
        gboolean match = (obj && g_strcmp0(obj, p->object_path) == 0);
        if (match) {
            const gchar *prefix = (current && g_str_has_prefix(current, "★ ")) ? "★ " : "";
            const gchar *raw_base = p->name_or_alias ? p->name_or_alias
                                  : (current ? (g_str_has_prefix(current, "★ ") ? current + 2 : current)
                                             : p->object_path);
            char *base = strip_state_markers(raw_base);
            GString *label = g_string_new(NULL);
            g_string_append_printf(label, "%s%s", prefix, base ? base : "");
            if (p->paired)   g_string_append(label, " [Paired]");
            if (p->trusted)  g_string_append(label, " [Trusted]");
            if (p->connected)g_string_append(label, " [Connected]");
            gtk_list_store_set(s_store, &row, 0, label->str, -1);
            g_string_free(label, TRUE);
            if (current) g_free(current);
            if (obj) g_free(obj);
            if (base) g_free(base);
            break;
        }
        if (current) g_free(current);
        if (obj) g_free(obj);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(s_store), &row);
    }

    if (p->object_path) g_free(p->object_path);
    if (p->name_or_alias) g_free(p->name_or_alias);
    g_free(p);
    return G_SOURCE_REMOVE;
}

/* Async result handler: parse properties and schedule UI update on main loop */
static void get_device_props_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    /* GAsyncReadyCallback signature uses GObject* as first param; convert to GDBusConnection below */
    char *obj = user_data; /* ownership transferred here */
    if (!obj) return;

    if (g_shutting_down) {
        g_free(obj);
        return;
    }

    GError *err = NULL;
    GDBusConnection *conn = G_DBUS_CONNECTION(source_object);
    GVariant *r = g_dbus_connection_call_finish(conn, res, &err);
    if (!r) {
        if (err) {
            g_debug("get_device_props_done: GetAll failed for %s: %s", obj, err->message);
            g_error_free(err);
        }
        g_free(obj);
        return;
    }

    DevProps *p = g_new0(DevProps, 1);
    p->object_path = obj;
    p->paired = FALSE;
    p->trusted = FALSE;
    p->connected = FALSE;
    p->name_or_alias = NULL;

    GVariantIter *iter = NULL;
    g_variant_get(r, "(a{sv})", &iter);
    gchar *key = NULL;
    GVariant *val = NULL;
    while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
        if (g_strcmp0(key, "Paired") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            p->paired = g_variant_get_boolean(val);
        else if (g_strcmp0(key, "Trusted") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            p->trusted = g_variant_get_boolean(val);
        else if (g_strcmp0(key, "Connected") == 0 && g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
            p->connected = g_variant_get_boolean(val);
        else if (!p->name_or_alias && (g_strcmp0(key, "Alias") == 0 || g_strcmp0(key, "Name") == 0) &&
                 g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
            p->name_or_alias = safe_utf8(g_variant_get_string(val, NULL));
        }
        if (val) g_variant_unref(val);
        g_free(key);
    }
    if (iter) g_variant_iter_free(iter);
    g_variant_unref(r);

    /* Schedule main-loop UI update */
    g_idle_add(__gui_bt_apply_device_props_idle, p);
}

/* Asynchronous update starter: initiates GetAll and returns immediately */
void update_device_row_state(const char *object_path) {
    if (!object_path || !s_store) return;

    GError *err = NULL;
    if (!ensure_system_bus(&err)) {
        if (err) g_error_free(err);
        return;
    }

    /* Duplicate path for ownership transfer into async callback */
    char *dup = g_strdup(object_path);
    g_dbus_connection_call(gui_system_bus,
                           "org.bluez",
                           dup,
                           "org.freedesktop.DBus.Properties",
                           "GetAll",
                           g_variant_new("(s)", "org.bluez.Device1"),
                           G_VARIANT_TYPE("(a{sv})"),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, /* timeout: let GLib manage */
                           NULL,
                           get_device_props_done,
                           dup);
}

/* D-Bus InterfacesAdded callback for BlueZ (ObjectManager)
 *
 * Keep this handler extremely conservative to avoid any GVariant parsing bugs on
 * older GLib builds. We do NOT parse Device1 properties here. We only extract the
 * object path and schedule a UI add with a placeholder label derived from the path.
 * The detailed label (Name/Alias + flags) will be filled by update_device_row_state()
 * asynchronously on the main loop.
 */
static void bluez_interfaces_added_for_gui(GDBusConnection *connection,
                                          const gchar *sender_name,
                                          const gchar *object_path,
                                          const gchar *interface_name,
                                          const gchar *signal_name,
                                          GVariant *parameters,
                                          gpointer user_data)
{
    (void)connection; (void)sender_name; (void)object_path; (void)interface_name; (void)user_data; (void)signal_name;

    /* Expect (o a{sa{sv}}); borrow the path pointer from parameters */
    const gchar *path = NULL;
    GVariantIter *interfaces = NULL;

    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(oa{sa{sv}})")))
        return;

    g_variant_get(parameters, "(oa{sa{sv}})", &path, &interfaces);
    if (interfaces) g_variant_iter_free(interfaces);
    if (!path) return;

    /* Derive a safe display label from the object path; row will be updated later */
    const gchar *last = strrchr(path, '/');
    char *display = last ? safe_utf8(last + 1) : safe_utf8(path);

    char **pair = g_new0(char*, 3);
    pair[0] = display;
    pair[1] = g_strdup(path);
    g_idle_add(__gui_bt_add_device_idle, pair);
    /* Do NOT free 'path'; it is borrowed from parameters */
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
    const gchar *path = NULL;     /* borrowed if coming from "(oas)" */
    char *owned_path = NULL;      /* duplicated string we can pass to idle handler */
    GVariantIter *ifaces = NULL;

    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(oas)"))) {
        g_variant_get(parameters, "(oas)", &path, &ifaces);
        if (path) owned_path = g_strdup(path);
        if (ifaces) g_variant_iter_free(ifaces);
    } else if (g_variant_n_children(parameters) > 0) {
        /* Fallback: support (o@as) and other variant shapes by extracting first child.
         * Avoid using '(@...)' style GVariant type strings which may be unsupported
         * on older GLib versions (they can trigger g_variant_type_info_check failures). */
        GVariant *child = g_variant_get_child_value(parameters, 0);
        if (child && g_variant_is_of_type(child, G_VARIANT_TYPE_STRING)) {
            owned_path = g_strdup(g_variant_get_string(child, NULL));
        }
        if (child) g_variant_unref(child);
    }

    if (owned_path) {
        g_idle_add(__gui_bt_remove_device_idle, owned_path);
        /* owned_path will be freed in __gui_bt_remove_device_idle */
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
    GVariant *ifaces = NULL;

    while (g_variant_iter_next(outer, "{o@a{sa{sv}}}", &path, &ifaces)) {
        gchar *iface_name = NULL;
        GVariant *props = NULL;
        
        GVariantIter iiter;
        g_variant_iter_init(&iiter, ifaces);

        while (g_variant_iter_next(&iiter, "{s@a{sv}}", &iface_name, &props)) {
            if (g_strcmp0(iface_name, "org.bluez.Device1") == 0) {
                /* Extract iterator from the a{sv} variant to traverse properties */
                GVariantIter *piter = NULL;
                g_variant_get(props, "a{sv}", &piter);
                gchar *pkey = NULL;
                GVariant *pval = NULL;
                char *display = NULL;
                while (g_variant_iter_next(piter, "{sv}", &pkey, &pval)) {
                    if (!display && g_strcmp0(pkey, "Name") == 0 && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                        display = safe_utf8(g_variant_get_string(pval, NULL));
                    } else if (!display && g_strcmp0(pkey, "Alias") == 0 && pval && g_variant_is_of_type(pval, G_VARIANT_TYPE_STRING)) {
                        display = safe_utf8(g_variant_get_string(pval, NULL));
                    }
                    if (pval) g_variant_unref(pval);
                    g_free(pkey);
                }
                if (piter) g_variant_iter_free(piter);
                if (!display) {
                    const gchar *last = strrchr(path, '/');
                    display = last ? safe_utf8(last + 1) : safe_utf8(path);
                }
                /* Tag pre-existing devices as Known with a star prefix */
                gchar *disp2 = g_strdup_printf("★ %s", display);
                gui_bt_add_device(disp2, path);
                g_free(disp2);
                g_free(display);
            }
            if (props) g_variant_unref(props);
            g_free(iface_name);
        }
        if (ifaces) g_variant_unref(ifaces);
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

    /* Subscribe to Device1 PropertiesChanged to track Paired/Trusted/Connected */
    bluez_device_props_changed_sub = g_dbus_connection_signal_subscribe(gui_system_bus,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        NULL,
        "org.bluez.Device1",
        G_DBUS_SIGNAL_FLAGS_NONE,
        bluez_device_properties_changed_cb,
        NULL,
        NULL);

    /* Subscribe to Adapter1 PropertiesChanged to drive Scan/Stop sensitivity */
    bluez_adapter_props_changed_sub = g_dbus_connection_signal_subscribe(gui_system_bus,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        NULL,
        "org.bluez.Adapter1",
        G_DBUS_SIGNAL_FLAGS_NONE,
        bluez_adapter_properties_changed_cb,
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
    if (bluez_device_props_changed_sub) {
        g_dbus_connection_signal_unsubscribe(gui_system_bus, bluez_device_props_changed_sub);
        bluez_device_props_changed_sub = 0;
    }
    if (bluez_adapter_props_changed_sub) {
        g_dbus_connection_signal_unsubscribe(gui_system_bus, bluez_adapter_props_changed_sub);
        bluez_adapter_props_changed_sub = 0;
    }
    s_listeners_registered = FALSE;
    return 0;
}
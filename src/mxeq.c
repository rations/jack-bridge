#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h> // For g_mkdir_with_parents and file operations
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Forward declaration for Devices panel (Playback switching) */
static void create_devices_panel(GtkWidget *main_box);

/* Forward declarations needed by earlier callers */
static int write_string_atomic(const char *path, const char *content);
static gboolean bluealsa_ports_exist(void);

typedef struct {
    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem;
    GtkWidget *scale;
    GtkWidget *mute_check;
    const char *channel_name;
    gboolean is_capture;  /* TRUE if this is a capture control, FALSE for playback */
} MixerChannel;

typedef struct {
    snd_mixer_t *mixer;
    MixerChannel *channels;
    int num_channels;
    int current_card;  /* Track which card we're showing (0=internal, 1=USB, etc.) */
    GtkWidget *mixer_box;  /* Reference to container for dynamic rebuild */
} MixerData;

typedef struct {
    snd_ctl_t *ctl;
    snd_ctl_elem_value_t *val;
    GtkWidget *scale;
    const char *band_name;
} EQBand;

typedef struct {
    snd_ctl_t *ctl;
    EQBand *bands;
    int num_bands;
} EQData;

/* UI globals used to keep window/expander references for compacting behavior.
   These are used by the expander 'notify::expanded' handler to shrink the
   main window back to a compact size when all expanders are collapsed. */
static GtkWidget *g_main_window = NULL;
static GtkWidget *g_eq_expander = NULL;
static GtkWidget *g_bt_expander = NULL;
static GtkWidget *g_dev_expander = NULL;
/* Expose Bluetooth device tree to Devices (Playback) panel for MAC selection */
static GtkWidget *g_bt_tree = NULL;
/* Phase 3: Global references to Devices panel radio buttons for state synchronization */
static GtkWidget *g_rb_internal = NULL;
static GtkWidget *g_rb_usb = NULL;
static GtkWidget *g_rb_hdmi = NULL;
static GtkWidget *g_rb_bt = NULL;
/* Global mixer data for dynamic card switching */
static MixerData *g_mixer_data = NULL;
/* Forward declaration used by Devices (Playback) panel to derive MAC from BlueZ path */
static char *mac_from_bluez_object(const char *s);
/* Forward declaration for mixer rebuild */
static void rebuild_mixer_for_card(int card_num);

/* When both expanders are collapsed we want the main window to shrink back to a
   compact height so there is no wasted blank space. This handler watches the
   'expanded' property on both expanders and toggles the window size accordingly.
   It prefers to let the window manager size the window naturally when possible,
   but uses a small fallback compact height to ensure tight layout on minimal screens.
*/
static void on_any_expander_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    /* Silence unused parameter warnings */
    (void)object;
    (void)pspec;
    (void)user_data;
    if (!g_main_window || !g_eq_expander || !g_bt_expander || !g_dev_expander) return;

    gboolean eq_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_eq_expander));
    gboolean bt_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_bt_expander));
    gboolean dev_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_dev_expander));

    if (!eq_exp && !bt_exp && !dev_exp) {
        /* All collapsed - shrink to compact height */
        gtk_window_resize(GTK_WINDOW(g_main_window), 600, 260);
    } else if ((eq_exp || bt_exp) && !dev_exp) {
        /* EQ or Bluetooth expanded (large panels) */
        gtk_window_resize(GTK_WINDOW(g_main_window), 600, 480);
    } else if (dev_exp && !eq_exp && !bt_exp) {
        /* Only Devices expanded (small panel - just radio buttons) */
        gtk_window_resize(GTK_WINDOW(g_main_window), 600, 310);
    } else {
        /* Multiple panels expanded */
        gtk_window_resize(GTK_WINDOW(g_main_window), 600, 480);
    }
}

/* Bluetooth wrapper helpers used by the UI.
   These call into src/gui_bt.c helpers and present GTK error dialogs on failure.
   Keep minimal and safe: if the gui_bt_* implementation is not linked or fails,
   the wrappers return non-zero and the UI shows an error to the user. */
static void show_bt_error_dialog(GtkWindow *parent, const char *msg) {
    GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

/* Safe helper: return a GtkWindow* for any widget's toplevel, or NULL */
static GtkWindow *get_parent_window_from_widget(GtkWidget *w) {
    if (!w) return NULL;
    GtkWidget *top = gtk_widget_get_toplevel(w);
    if (top && GTK_IS_WINDOW(top)) {
        return GTK_WINDOW(top);
    }
    return NULL;
}

/* Forward declarations of GUI BT helpers (defined in src/gui_bt.c) */
extern int gui_bt_start_discovery(const char *adapter_path);
extern int gui_bt_stop_discovery(const char *adapter_path);
extern int gui_bt_pair_device(const char *device_path_or_mac);
extern int gui_bt_trust_device(const char *device_path_or_mac, int trusted);
extern int gui_bt_connect_device(const char *device_path_or_mac);
/* Async variants (provide callbacks to surface errors to GUI without blocking) */
extern int gui_bt_pair_device_async(const char *device_path_or_mac, void (*cb)(gboolean, const char *, gpointer), gpointer ud);
extern int gui_bt_trust_device_async(const char *device_path_or_mac, gboolean trusted, void (*cb)(gboolean, const char *, gpointer), gpointer ud);
extern int gui_bt_connect_device_async(const char *device_path_or_mac, void (*cb)(gboolean, const char *, gpointer), gpointer ud);
/* Renamed D-Bus removal helper to avoid collision with UI removal */
extern int bluez_remove_device(const char *device_path_or_mac);
/* Explicit binding for the Bluetooth device list store */
extern int gui_bt_set_device_store_widget(GtkWidget *treeview, GtkListStore *store);
extern int gui_bt_register_discovery_listeners(void);
extern void gui_bt_populate_existing_devices(void);
extern int gui_bt_bind_scan_buttons(GtkWidget *scan_btn, GtkWidget *stop_btn);
/* Query Device1 state (Paired/Trusted/Connected) for button gating */
extern int gui_bt_get_device_state(const char *object_path, gboolean *paired, gboolean *trusted, gboolean *connected);
/* Set adapter Discoverable property (0=off, 1=on) */
extern int gui_bt_set_adapter_discoverable(gboolean discoverable);
/* Query adapter Discoverable property */
extern gboolean gui_bt_get_adapter_discoverable(void);
/* Forward declaration for Bluetooth "Set as output" action */
static void on_bt_set_output_clicked(GtkButton *b, gpointer user_data);

/* Safe wrappers return 0 on success, -1 on failure and show a GTK dialog when appropriate */
static int bt_wrapper_start_discovery(GtkWindow *parent) {
    if (gui_bt_start_discovery(NULL) != 0) {
        show_bt_error_dialog(parent, "Failed to start Bluetooth discovery");
        return -1;
    }
    return 0;
}
static int bt_wrapper_stop_discovery(GtkWindow *parent) {
    if (gui_bt_stop_discovery(NULL) != 0) {
        show_bt_error_dialog(parent, "Failed to stop Bluetooth discovery");
        return -1;
    }
    return 0;
}
static int bt_wrapper_remove(GtkWindow *parent, const char *objpath) {
    if (bluez_remove_device(objpath) != 0) {
        show_bt_error_dialog(parent, "RemoveDevice failed");
        return -1;
    }
    return 0;
}

/* Async operation callbacks to surface errors in GTK */
static void bt_pair_op_cb(gboolean success, const char *message, gpointer user_data) {
    GtkWindow *parent = GTK_WINDOW(user_data);
    if (!success && message) {
        show_bt_error_dialog(parent, message);
    }
}
static void bt_connect_op_cb(gboolean success, const char *message, gpointer user_data) {
    GtkWindow *parent = GTK_WINDOW(user_data);
    if (!success && message) {
        show_bt_error_dialog(parent, message);
    }
}
static void bt_trust_op_cb(gboolean success, const char *message, gpointer user_data) {
    GtkWindow *parent = GTK_WINDOW(user_data);
    if (!success && message) {
        show_bt_error_dialog(parent, message);
    }
}

/* Bluetooth helper callbacks at file scope (valid C, referenced by GCallback in main UI) */
static gpointer tree_get_selected_obj(GtkTreeView *tv) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gchar *obj = NULL;
        gtk_tree_model_get(model, &iter, 1, &obj, -1);
        return obj; /* caller must g_free */
    }
    return NULL;
}

static void on_scan_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    /* Toggle buttons only on success (async D-Bus call already initiated inside wrapper) */
    GtkWidget *stop_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "peer_stop_btn"));
    if (bt_wrapper_start_discovery(parent) == 0) {
        if (stop_btn) gtk_widget_set_sensitive(stop_btn, TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(b), FALSE);
    }
}

static void on_stop_scan_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    /* Toggle buttons only on success */
    GtkWidget *scan_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "peer_scan_btn"));
    if (bt_wrapper_stop_discovery(parent) == 0) {
        if (scan_btn) gtk_widget_set_sensitive(scan_btn, TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(b), FALSE);
    }
}

static void on_pair_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(b), "device_tree"));
    gchar *obj = tree_get_selected_obj(tv);
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    if (!obj) { show_bt_error_dialog(parent, "No device selected"); return; }
    /* Async pair with error surfacing */
    gui_bt_pair_device_async(obj, bt_pair_op_cb, parent);
    g_free(obj);
}

static void on_trust_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(b), "device_tree"));
    gchar *obj = tree_get_selected_obj(tv);
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    if (!obj) { show_bt_error_dialog(parent, "No device selected"); return; }

    /* Recheck current device state to avoid racing transient states */
    gboolean paired = FALSE, trusted = FALSE, connected = FALSE;
    if (gui_bt_get_device_state(obj, &paired, &trusted, &connected) == 0) {
        if (!paired) {
            show_bt_error_dialog(parent, "Device is not paired. Pair the device before marking it as trusted.");
            g_free(obj);
            return;
        }
        if (trusted) {
            show_bt_error_dialog(parent, "Device is already trusted.");
            g_free(obj);
            return;
        }
    }

    /* Async trust=true with error surfacing */
    gui_bt_trust_device_async(obj, TRUE, bt_trust_op_cb, parent);
    g_free(obj);
}

static void on_connect_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(b), "device_tree"));
    gchar *obj = tree_get_selected_obj(tv);
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    if (!obj) { show_bt_error_dialog(parent, "No device selected"); return; }

    /* Recheck current device state to ensure Connect preconditions are met */
    gboolean paired = FALSE, trusted = FALSE, connected = FALSE;
    if (gui_bt_get_device_state(obj, &paired, &trusted, &connected) == 0) {
        if (!paired) {
            show_bt_error_dialog(parent, "Device is not paired. Pair the device before connecting.");
            g_free(obj);
            return;
        }
    }

    /* Async connect with error surfacing */
    gui_bt_connect_device_async(obj, bt_connect_op_cb, parent);
    g_free(obj);
}

static void on_remove_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(b), "device_tree"));
    gchar *obj = tree_get_selected_obj(tv);
    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(b));
    if (!obj) { show_bt_error_dialog(parent, "No device selected"); return; }
    bt_wrapper_remove(parent, obj);
    g_free(obj);
}

/* Bluetooth Discoverable toggle callback */
static void on_bt_discoverable_toggled(GtkSwitch *sw, gboolean state, gpointer user_data) {
    (void)user_data;
    /* state parameter is the NEW state (after toggle) */
    if (gui_bt_set_adapter_discoverable(state) != 0) {
        GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(sw));
        show_bt_error_dialog(parent, "Failed to change Bluetooth discovery state.\n\nYou may need to be in the 'bluetooth' group.");
        /* Revert switch state on failure */
        g_signal_handlers_block_by_func(sw, G_CALLBACK(on_bt_discoverable_toggled), NULL);
        gtk_switch_set_active(sw, !state);
        g_signal_handlers_unblock_by_func(sw, G_CALLBACK(on_bt_discoverable_toggled), NULL);
    }
}
// Callback to save preset to ~/.local/share/mxeq/presets.csv
static void save_preset(GtkWidget *button, gpointer user_data) {
    EQData *eq_data = (EQData *)user_data;
    GtkWidget *entry = g_object_get_data(G_OBJECT(button), "preset_entry");
    GtkWidget *combo = g_object_get_data(G_OBJECT(button), "preset_combo");
    GtkWidget *window = g_object_get_data(G_OBJECT(button), "window");
    const char *preset_name = gtk_entry_get_text(GTK_ENTRY(entry));

    if (!preset_name || strlen(preset_name) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Please enter a preset name.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Check for duplicate preset name
    const char *home = g_get_home_dir();
    char *preset_path = g_build_filename(home, ".local", "share", "mxeq", "presets.csv", NULL);
    FILE *fp_check = fopen(preset_path, "r");
    if (fp_check) {
        char line[256];
        while (fgets(line, sizeof(line), fp_check)) {
            char *name = strtok(line, ":");
            if (name && strcmp(name, preset_name) == 0) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                          "Preset name already exists. Choose a different name.");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                fclose(fp_check);
                g_free(preset_path);
                return;
            }
        }
        fclose(fp_check);
    }

    // Create directory if it doesn't exist
    char *dir_path = g_build_filename(home, ".local", "share", "mxeq", NULL);
    g_mkdir_with_parents(dir_path, 0755);
    g_free(dir_path);

    // Open file in append mode
    FILE *fp = fopen(preset_path, "a");
    if (!fp) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Failed to save preset: Could not open file.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(preset_path);
        return;
    }

    // Collect EQ values
    GString *values = g_string_new("");
    for (int i = 0; i < eq_data->num_bands; i++) {
        gdouble value = gtk_range_get_value(GTK_RANGE(eq_data->bands[i].scale));
        g_string_append_printf(values, "%.2f", value);
        if (i < eq_data->num_bands - 1) {
            g_string_append(values, ",");
        }
    }

    // Write preset to file
    fprintf(fp, "%s:%s\n", preset_name, values->str);
    fclose(fp);
    g_string_free(values, TRUE);

    // Add to combo box and set as active
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), preset_name);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), preset_name);

    // Show confirmation dialog
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Preset Saved!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Clear entry
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    g_free(preset_path);
}

// Callback to apply selected preset
static void apply_preset(GtkComboBox *combo, gpointer user_data) {
    EQData *eq_data = (EQData *)user_data;
    char *preset_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!preset_name || g_strcmp0(preset_name, "Choose EQ Pre-set") == 0) {
        g_free(preset_name);
        return;
    }

    // Build preset file path
    const char *home = g_get_home_dir();
    char *preset_path = g_build_filename(home, ".local", "share", "mxeq", "presets.csv", NULL);

    // Open preset file
    FILE *fp = fopen(preset_path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open presets.csv\n");
        g_free(preset_name);
        g_free(preset_path);
        return;
    }

    // Read file to find matching preset
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *name = strtok(line, ":");
        if (name && strcmp(name, preset_name) == 0) {
            char *values = strtok(NULL, "\n");
            if (values) {
                char *token = strtok(values, ",");
                int i = 0;
                while (token && i < eq_data->num_bands) {
                    gdouble value = g_ascii_strtod(token, NULL);
                    gtk_range_set_value(GTK_RANGE(eq_data->bands[i].scale), value);
                    long alsa_value = (long)(value * 100.0);
                    snd_ctl_elem_value_set_integer(eq_data->bands[i].val, 0, alsa_value);
                    snd_ctl_elem_value_set_integer(eq_data->bands[i].val, 1, alsa_value);
                    snd_ctl_elem_write(eq_data->bands[i].ctl, eq_data->bands[i].val);
                    token = strtok(NULL, ",");
                    i++;
                }
            }
            break;
        }
    }
    fclose(fp);
    g_free(preset_name);
    g_free(preset_path);
}

// Load presets into combo box at startup
static void load_presets(GtkComboBoxText *combo) {
    gtk_combo_box_text_append_text(combo, "Choose EQ Pre-set");
    const char *home = g_get_home_dir();
    char *preset_path = g_build_filename(home, ".local", "share", "mxeq", "presets.csv", NULL);
    FILE *fp = fopen(preset_path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char *name = strtok(line, ":");
            if (name) {
                gtk_combo_box_text_append_text(combo, name);
            }
        }
        fclose(fp);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0); // Default to "Choose EQ Pre-set"
    g_free(preset_path);
}

static void slider_changed(GtkRange *range, MixerChannel *channel) {
    gdouble value = gtk_range_get_value(range);
    long min, max;
    if (channel->is_capture) {
        snd_mixer_selem_get_capture_volume_range(channel->elem, &min, &max);
        long alsa_value = (long)(value * (max - min) + min);
        snd_mixer_selem_set_capture_volume_all(channel->elem, alsa_value);
    } else {
        snd_mixer_selem_get_playback_volume_range(channel->elem, &min, &max);
        long alsa_value = (long)(value * (max - min) + min);
        snd_mixer_selem_set_playback_volume_all(channel->elem, alsa_value);
    }
}

/* Mute toggle handler — for playback: 1=unmuted, 0=muted; for capture: 1=enabled, 0=disabled.
   We expose a "Mute" checkbox (or "Enable" for capture); when checked, set appropriate state. */
static void on_mute_toggled(GtkToggleButton *btn, gpointer user_data) {
    MixerChannel *ch = (MixerChannel *)user_data;
    if (!ch || !ch->elem) return;
    int checked = gtk_toggle_button_get_active(btn) ? 1 : 0;
    if (ch->is_capture) {
        /* For capture: checked=enabled (1=capture on), unchecked=disabled (0=capture off) */
        snd_mixer_selem_set_capture_switch_all(ch->elem, checked ? 1 : 0);
    } else {
        /* For playback: checked=muted (0=mute), unchecked=unmuted (1=unmute) */
        snd_mixer_selem_set_playback_switch_all(ch->elem, checked ? 0 : 1);
    }
}

static void eq_slider_changed(GtkRange *range, EQBand *band) {
    gdouble value = gtk_range_get_value(range);
    long alsa_value = (long)(value * 100.0); // Map 0-1 to 0-100
    snd_ctl_elem_value_set_integer(band->val, 0, alsa_value); // Left channel
    snd_ctl_elem_value_set_integer(band->val, 1, alsa_value); // Right channel
    snd_ctl_elem_write(band->ctl, band->val);
}

static void init_alsa_mixer(MixerData *data, int card_num) {
    /* Generic mixer initialization for specified card number.
     * Enumerates ALL simple mixer elements (no hardcoded names).
     * Works for any audio card: internal, USB, or external interfaces.
     */
    
    /* Close existing mixer if reinitializing */
    if (data->mixer) {
        snd_mixer_close(data->mixer);
        data->mixer = NULL;
    }
    if (data->channels) {
        g_free(data->channels);
        data->channels = NULL;
    }
    data->num_channels = 0;
    data->current_card = card_num;
    
    /* Build card attach string */
    gchar *card_str = g_strdup_printf("hw:%d", card_num);
    
    snd_mixer_t *m = NULL;
    if (snd_mixer_open(&m, 0) < 0) {
        fprintf(stderr, "init_alsa_mixer: failed to open mixer for card %d\n", card_num);
        g_free(card_str);
        return;
    }
    if (snd_mixer_attach(m, card_str) < 0) {
        fprintf(stderr, "init_alsa_mixer: failed to attach to %s\n", card_str);
        snd_mixer_close(m);
        g_free(card_str);
        return;
    }
    if (snd_mixer_selem_register(m, NULL, NULL) < 0) {
        fprintf(stderr, "init_alsa_mixer: failed to register simple element class for %s\n", card_str);
        snd_mixer_close(m);
        g_free(card_str);
        return;
    }
    if (snd_mixer_load(m) < 0) {
        fprintf(stderr, "init_alsa_mixer: failed to load mixer elements for %s\n", card_str);
        snd_mixer_close(m);
        g_free(card_str);
        return;
    }
    
    fprintf(stderr, "init_alsa_mixer: successfully opened card %d (%s)\n", card_num, card_str);
    g_free(card_str);
    
    /* Count simple mixer elements */
    int count = 0;
    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(m); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE) {
            count++;
        }
    }
    
    if (count == 0) {
        fprintf(stderr, "init_alsa_mixer: no simple mixer elements found on card %d\n", card_num);
        snd_mixer_close(m);
        return;
    }
    
    /* Allocate channel array */
    data->channels = g_new0(MixerChannel, count);
    data->mixer = m;
    
    /* Enumerate ALL simple mixer elements generically */
    int idx = 0;
    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(m); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE) continue;
        
        const char *name = snd_mixer_selem_get_name(elem);
        if (!name) continue;
        
        /* Detect if this is a capture-only control */
        gboolean is_capture = FALSE;
        if (snd_mixer_selem_has_capture_volume(elem) && !snd_mixer_selem_has_playback_volume(elem)) {
            is_capture = TRUE;
        }
        
        /* Auto-enable ALL capture switches on startup (generic for any interface) */
        if (is_capture && snd_mixer_selem_has_capture_switch(elem)) {
            int sw = 0;
            snd_mixer_selem_get_capture_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
            if (!sw) {
                snd_mixer_selem_set_capture_switch_all(elem, 1);
                fprintf(stderr, "Auto-enabled capture for '%s' on card %d\n", name, card_num);
            }
        }
        
        data->channels[idx].elem = elem;
        data->channels[idx].channel_name = g_strdup(name);  /* Allocate copy since elem names are transient */
        data->channels[idx].is_capture = is_capture;
        idx++;
    }
    
    data->num_channels = idx;
    fprintf(stderr, "init_alsa_mixer: found %d mixer controls on card %d\n", idx, card_num);
}

static void init_alsa_eq(EQData *data) {
    int err;
    if ((err = snd_ctl_open(&data->ctl, "equal", 0)) < 0) {
        fprintf(stderr, "Failed to open ALSA control 'equal': %s\n", snd_strerror(err));
        return;
    }

    snd_ctl_elem_list_t *list;
    snd_ctl_elem_list_alloca(&list);
    if ((err = snd_ctl_elem_list(data->ctl, list)) < 0) {
        fprintf(stderr, "Failed to get initial control list: %s\n", snd_strerror(err));
        snd_ctl_close(data->ctl);
        return;
    }

    int num_controls = snd_ctl_elem_list_get_count(list);
    if (num_controls <= 0) {
        fprintf(stderr, "No controls found for 'equal' device\n");
        snd_ctl_close(data->ctl);
        return;
    }

    if ((err = snd_ctl_elem_list_alloc_space(list, num_controls)) < 0) {
        fprintf(stderr, "Failed to allocate control list space: %s\n", snd_strerror(err));
        snd_ctl_close(data->ctl);
        return;
    }
    if ((err = snd_ctl_elem_list(data->ctl, list)) < 0) {
        fprintf(stderr, "Failed to list controls: %s\n", snd_strerror(err));
        snd_ctl_close(data->ctl);
        return;
    }

    num_controls = snd_ctl_elem_list_get_used(list);
    data->bands = g_new(EQBand, num_controls);
    data->num_bands = 0;

    for (int i = 0; i < num_controls; i++) {
        snd_ctl_elem_id_t *id;
        snd_ctl_elem_id_alloca(&id);
        snd_ctl_elem_list_get_id(list, i, id);

        const char *name = snd_ctl_elem_id_get_name(id);
        unsigned int numid = snd_ctl_elem_id_get_numid(id);
        snd_ctl_elem_iface_t iface = snd_ctl_elem_id_get_interface(id);

        if (iface != SND_CTL_ELEM_IFACE_MIXER) {
            fprintf(stderr, "Skipping non-mixer control: %s (interface=%d)\n", name, iface);
            continue;
        }

        snd_ctl_elem_value_t *val;
        if ((err = snd_ctl_elem_value_malloc(&val)) < 0) {
            fprintf(stderr, "Failed to allocate value for control '%s': %s\n", name, snd_strerror(err));
            continue;
        }
        snd_ctl_elem_value_set_id(val, id);

        if ((err = snd_ctl_elem_read(data->ctl, val)) < 0) {
            fprintf(stderr, "Failed to read control '%s' (numid %u): %s\n", name, numid, snd_strerror(err));
            snd_ctl_elem_value_free(val);
            continue;
        }

        data->bands[data->num_bands].val = val;
        data->bands[data->num_bands].band_name = g_strdup(name);
        data->bands[data->num_bands].ctl = data->ctl;
        data->num_bands++;
    }

    if (data->num_bands == 0) {
        fprintf(stderr, "No EQ bands found after enumeration\n");
    }
    snd_ctl_elem_list_free_space(list);
}

static void cleanup_alsa(MixerData *mixer_data, EQData *eq_data) {
    if (mixer_data->mixer) {
        snd_mixer_close(mixer_data->mixer);
    }
    if (mixer_data->channels) {
        /* Free dynamically allocated channel names */
        for (int i = 0; i < mixer_data->num_channels; i++) {
            if (mixer_data->channels[i].channel_name) {
                g_free((char*)mixer_data->channels[i].channel_name);
            }
        }
        g_free(mixer_data->channels);
    }
    if (eq_data->ctl) {
        for (int i = 0; i < eq_data->num_bands; i++) {
            if (eq_data->bands[i].val) {
                snd_ctl_elem_value_free(eq_data->bands[i].val);
            }
            g_free((char *)eq_data->bands[i].band_name);
        }
        snd_ctl_close(eq_data->ctl);
        g_free(eq_data->bands);
    }
}

/* Dynamic mixer rebuild: clear and repopulate mixer_box with controls from specified card */
static void rebuild_mixer_for_card(int card_num) {
    if (!g_mixer_data || !g_mixer_data->mixer_box) return;
    
    fprintf(stderr, "rebuild_mixer_for_card: switching to card %d\n", card_num);
    
    /* Clear existing mixer UI */
    GList *children = gtk_container_get_children(GTK_CONTAINER(g_mixer_data->mixer_box));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    /* Reinitialize mixer for new card */
    init_alsa_mixer(g_mixer_data, card_num);
    
    /* Rebuild UI with new controls */
    if (g_mixer_data->num_channels == 0) {
        GtkWidget *no_mixer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_hexpand(no_mixer_box, TRUE);
        gtk_widget_set_halign(no_mixer_box, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(g_mixer_data->mixer_box), no_mixer_box, 0, 0, 8, 1);

        gchar *msg = g_strdup_printf(
            "No mixer controls detected on card %d.\n"
            "Audio may still work, but mixer sliders are unavailable.\n\n"
            "Try checking:\n"
            "• Card is properly detected: cat /proc/asound/cards\n"
            "• Mixer elements exist: alsamixer -c %d",
            card_num, card_num);
        GtkWidget *no_mixer_label = gtk_label_new(msg);
        g_free(msg);
        gtk_label_set_justify(GTK_LABEL(no_mixer_label), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(no_mixer_box), no_mixer_label, TRUE, TRUE, 8);
        gtk_widget_show_all(no_mixer_box);
        return;
    }
    
    /* Create slider for each control found (max 8 per row, auto-wrap) */
    for (int i = 0; i < g_mixer_data->num_channels; i++) {
        int col = i % 8;  // Max 8 columns per row
        int row = i / 8;  // Automatic row wrapping
        
        GtkWidget *channel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_grid_attach(GTK_GRID(g_mixer_data->mixer_box), channel_box, col, row, 1, 1);

        GtkWidget *label = gtk_label_new(g_mixer_data->channels[i].channel_name);
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(channel_box), label, FALSE, FALSE, 5);

        g_mixer_data->channels[i].scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 1, 0.01);
        gtk_range_set_inverted(GTK_RANGE(g_mixer_data->channels[i].scale), TRUE);
        gtk_scale_set_draw_value(GTK_SCALE(g_mixer_data->channels[i].scale), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(g_mixer_data->channels[i].scale), GTK_POS_BOTTOM);
        gtk_widget_set_size_request(g_mixer_data->channels[i].scale, -1, 150);
        gtk_box_pack_start(GTK_BOX(channel_box), g_mixer_data->channels[i].scale, TRUE, TRUE, 0);
        g_signal_connect(g_mixer_data->channels[i].scale, "value-changed", G_CALLBACK(slider_changed), &g_mixer_data->channels[i]);

        long min, max, value;
        if (g_mixer_data->channels[i].is_capture) {
            snd_mixer_selem_get_capture_volume_range(g_mixer_data->channels[i].elem, &min, &max);
            snd_mixer_selem_get_capture_volume(g_mixer_data->channels[i].elem, 0, &value);
        } else {
            snd_mixer_selem_get_playback_volume_range(g_mixer_data->channels[i].elem, &min, &max);
            snd_mixer_selem_get_playback_volume(g_mixer_data->channels[i].elem, 0, &value);
        }
        gtk_range_set_value(GTK_RANGE(g_mixer_data->channels[i].scale), (double)(value - min) / (max - min));

        /* Add Mute (playback) or Enable (capture) checkbox if control supports it */
        g_mixer_data->channels[i].mute_check = NULL;
        if (g_mixer_data->channels[i].is_capture) {
            /* Capture controls: expose Enable checkbox (checked=capture enabled) */
            if (snd_mixer_selem_has_capture_switch(g_mixer_data->channels[i].elem)) {
                int sw = 0;
                snd_mixer_selem_get_capture_switch(g_mixer_data->channels[i].elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
                GtkWidget *enable = gtk_check_button_new_with_label("Enable");
                gtk_widget_set_halign(enable, GTK_ALIGN_CENTER);
                gtk_widget_set_margin_top(enable, 4);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable), sw ? TRUE : FALSE);
                gtk_box_pack_start(GTK_BOX(channel_box), enable, FALSE, FALSE, 2);
                g_signal_connect(enable, "toggled", G_CALLBACK(on_mute_toggled), &g_mixer_data->channels[i]);
                g_mixer_data->channels[i].mute_check = enable;
            }
        } else {
            /* Playback controls: expose Mute checkbox (checked=muted) */
            if (snd_mixer_selem_has_playback_switch(g_mixer_data->channels[i].elem)) {
                int sw = 1;
                snd_mixer_selem_get_playback_switch(g_mixer_data->channels[i].elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
                GtkWidget *mute = gtk_check_button_new_with_label("Mute");
                gtk_widget_set_halign(mute, GTK_ALIGN_CENTER);
                gtk_widget_set_margin_top(mute, 4);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute), sw ? FALSE : TRUE);
                gtk_box_pack_start(GTK_BOX(channel_box), mute, FALSE, FALSE, 2);
                g_signal_connect(mute, "toggled", G_CALLBACK(on_mute_toggled), &g_mixer_data->channels[i]);
                g_mixer_data->channels[i].mute_check = mute;
            }
        }
    }
    
    gtk_widget_show_all(g_mixer_data->mixer_box);
    fprintf(stderr, "rebuild_mixer_for_card: rebuilt UI with %d controls for card %d\n",
            g_mixer_data->num_channels, card_num);
}
 
/* Recorder support: enhanced UX, safe child lifecycle, XDG Music path handling */

typedef struct {
    GtkWidget *status_label;
    GtkWidget *filename_entry;
    GtkWidget *channel_combo;   /* Mono / Stereo */
    GtkWidget *rate_combo;      /* 44100 / 48000 */
    GtkWidget *record_btn;
    GtkWidget *stop_btn;
} RecorderUI;

/* Globals to manage recording state */
static GPid record_pid = 0;
static guint record_timer_id = 0;
static time_t record_start_time = 0;
static RecorderUI *rec_ui = NULL;

/* Helper: ensure filename has .wav suffix (returns newly allocated string) */
static char *ensure_wav_extension(const char *name) {
    if (g_str_has_suffix(name, ".wav"))
        return g_strdup(name);
    return g_strdup_printf("%s.wav", name);
}

/* Helper: sanitize a basename by removing any path separators.
   Returns newly allocated string. */
static char *sanitize_basename(const char *name) {
    gchar *basename = g_path_get_basename(name);
    return g_strdup(basename);
}

/* Resolve user's Music directory via XDG; fallback to ~/Music.
   Returns newly allocated string path to directory (no trailing slash). */
static char *resolve_music_dir(void) {
    const char *xdg = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
    if (xdg && strlen(xdg) > 0) {
        return g_strdup(xdg);
    }
    const char *home = g_get_home_dir();
    char *fallback = g_build_filename(home, "Music", NULL);
    return fallback;
}

/* Format default filename like Alsa Sound Connect-YYYYmmdd-HHMMSS.wav (newly allocated) */
static char *format_default_filename(void) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char buf[128];
    /* Use a human-friendly prefix matching the application name */
    strftime(buf, sizeof(buf), "Alsa Sound Connect-%Y%m%d-%H%M%S.wav", &tm);
    return g_strdup(buf);
}

/* Timer callback to update recording duration label */
static gboolean update_timer(gpointer user_data) {
    (void)user_data;
    if (!rec_ui) return FALSE;
    time_t now = time(NULL);
    int seconds = (int)(now - record_start_time);
    int min = seconds / 60;
    int sec = seconds % 60;
    gchar *msg = g_strdup_printf("Recording… %02d:%02d", min, sec);
    gtk_label_set_text(GTK_LABEL(rec_ui->status_label), msg);
    g_free(msg);
    return TRUE;
}

/* Idle callback used to reset UI from non-main thread context */
static gboolean reset_ui_idle(gpointer user_data) {
    (void)user_data;
    if (!rec_ui) return FALSE;
    gtk_widget_set_sensitive(rec_ui->record_btn, TRUE);
    gtk_widget_set_sensitive(rec_ui->stop_btn, FALSE);
    gtk_label_set_text(GTK_LABEL(rec_ui->status_label), "Idle");
    return FALSE; /* remove source */
}

/* Child watch callback: called when arecord exits; status is child exit status */
static void on_record_child_exit(GPid pid, gint status, gpointer user_data) {
    (void)status;
    (void)user_data;
    /* Reap child */
    g_spawn_close_pid(pid);
    record_pid = 0;

    /* Stop timer if running */
    if (record_timer_id) {
        g_source_remove(record_timer_id);
        record_timer_id = 0;
    }

    /* Schedule UI reset on main loop */
    g_idle_add(reset_ui_idle, NULL);
}

/* Start recording: builds path, spawns arecord asynchronously, adds child watch, updates UI */
static void start_recording(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (record_pid != 0) {
        /* Already recording */
        return;
    }
    if (!rec_ui) return;

    const char *user_text = gtk_entry_get_text(GTK_ENTRY(rec_ui->filename_entry));
    if (!user_text || strlen(user_text) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Please enter a filename.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char *base = sanitize_basename(user_text);
    char *fname = ensure_wav_extension(base);
    g_free(base);

    char *music_dir = resolve_music_dir();
    g_mkdir_with_parents(music_dir, 0755);

    char *full_path = g_build_filename(music_dir, fname, NULL);
    g_free(music_dir);
    g_free(fname);

    /* Determine channels */
    int channels = 2; /* default stereo */
    const gchar *chan = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(rec_ui->channel_combo));
    if (chan) {
        if (g_strcmp0(chan, "Mono") == 0) channels = 1;
        g_free((gchar*)chan);
    }

    /* Determine sample rate */
    int rate = 48000;
    const gchar *rate_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(rec_ui->rate_combo));
    if (rate_text) {
        if (g_strcmp0(rate_text, "44100") == 0) rate = 44100;
        else rate = 48000;
        g_free((gchar*)rate_text);
    }

    /* Use ALSA 'jack' device for recording - connects to JACK's system:capture ports.
     * The 'jack' PCM is defined in /etc/asound.conf with capture_ports pointing to
     * system:capture_1/2, which are created by jackd when opened with -C hw:0.
     * This allows arecord to record from microphones/line-in through JACK.
     * Cannot use 'plughw:0' (jackd has exclusive hw access) or 'default' (playback only). */
    const char *input_dev = "jack";

    /* Build argv for arecord */
    gchar *channels_s = g_strdup_printf("%d", channels);
    gchar *rate_s = g_strdup_printf("%d", rate);

    gchar *argv[] = {
        "arecord",
        "-D", (gchar*)input_dev,
        "-r", rate_s,
        "-c", channels_s,
        "-f", "FLOAT_LE",  /* JACK plugin requires FLOAT_LE format */
        "-t", "wav",  /* Explicitly specify WAV format */
        full_path,
        NULL
    };

    GError *err = NULL;
    gchar *stderr_output = NULL;
    gboolean ok = g_spawn_async_with_pipes(
        NULL,
        argv,
        NULL,
        G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
        NULL, NULL,
        &record_pid,
        NULL,    /* stdin */
        NULL,    /* stdout */
        NULL,    /* stderr - we could capture this for debugging */
        &err
    );

    g_free(channels_s);
    g_free(rate_s);

    if (!ok) {
        gchar *error_msg = g_strdup_printf(
            "Failed to start recording.\n\n"
            "Error: %s\n\n"
            "Troubleshooting:\n"
            "• Ensure JACK is running: ps aux | grep jackd\n"
            "• Check ALSA config: aplay -L | grep jack\n"
            "• Verify capture device: arecord -l\n"
            "• Test manually: arecord -D jack -f S16_LE -r 48000 test.wav",
            err ? err->message : "unknown");
        
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", error_msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error_msg);
        if (err) g_error_free(err);
        if (stderr_output) g_free(stderr_output);
        g_free(full_path);
        return;
    }

    g_print("Recording started. PID=%d -> %s\n", record_pid, full_path);
    g_free(full_path);

    /* UI updates */
    gtk_widget_set_sensitive(rec_ui->record_btn, FALSE);
    gtk_widget_set_sensitive(rec_ui->stop_btn, TRUE);
    gtk_label_set_text(GTK_LABEL(rec_ui->status_label), "Recording… 00:00");

    record_start_time = time(NULL);
    record_timer_id = g_timeout_add_seconds(1, update_timer, NULL);

    /* Add child watch to reap and update UI when process exits */
    g_child_watch_add(record_pid, on_record_child_exit, NULL);
}

/* Stop recording: send SIGINT to arecord (if running) and rely on child-watch to finalize */
static void stop_recording(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (record_pid == 0) return;
    g_print("Stopping recording (PID %d)\n", record_pid);
    if (kill(record_pid, SIGINT) != 0) {
        /* If kill fails, try sending SIGTERM */
        kill(record_pid, SIGTERM);
    }
    /* Do not close pid here; wait for child watch to reap */
}

/* Build recorder UI and attach into provided main_box (caller retains ownership of main_box) */
static void create_recorder_ui(GtkWidget *main_box) {
    /* Allocate RecorderUI */
    rec_ui = g_new0(RecorderUI, 1);

    GtkWidget *rec_frame = gtk_frame_new("Recorder");
    gtk_box_pack_start(GTK_BOX(main_box), rec_frame, FALSE, FALSE, 5);

    GtkWidget *rec_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(rec_frame), rec_vbox);

    rec_ui->status_label = gtk_label_new("Idle");
    gtk_box_pack_start(GTK_BOX(rec_vbox), rec_ui->status_label, FALSE, FALSE, 0);

    GtkWidget *rec_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(rec_vbox), rec_box, FALSE, FALSE, 0);

    /* Filename entry */
    rec_ui->filename_entry = gtk_entry_new();
    char *default_name = format_default_filename();
    gtk_entry_set_text(GTK_ENTRY(rec_ui->filename_entry), default_name);
    g_free(default_name);
    gtk_entry_set_placeholder_text(GTK_ENTRY(rec_ui->filename_entry), "recording.wav");
    gtk_box_pack_start(GTK_BOX(rec_box), rec_ui->filename_entry, TRUE, TRUE, 5);

    /* Channel combo */
    rec_ui->channel_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rec_ui->channel_combo), "Mono");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rec_ui->channel_combo), "Stereo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(rec_ui->channel_combo), 1); // Stereo default
    gtk_box_pack_start(GTK_BOX(rec_box), rec_ui->channel_combo, FALSE, FALSE, 5);

    /* Rate combo */
    rec_ui->rate_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rec_ui->rate_combo), "44100");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rec_ui->rate_combo), "48000");
    gtk_combo_box_set_active(GTK_COMBO_BOX(rec_ui->rate_combo), 1); // 48000 default
    gtk_box_pack_start(GTK_BOX(rec_box), rec_ui->rate_combo, FALSE, FALSE, 5);

    /* Buttons */
    rec_ui->record_btn = gtk_button_new_with_label("Record");
    rec_ui->stop_btn = gtk_button_new_with_label("Stop");
    gtk_box_pack_start(GTK_BOX(rec_box), rec_ui->record_btn, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(rec_box), rec_ui->stop_btn, FALSE, FALSE, 5);

/* Removed nested Bluetooth callbacks: moved to file scope above */
    gtk_widget_set_sensitive(rec_ui->stop_btn, FALSE); // initially disabled

    g_signal_connect(rec_ui->record_btn, "clicked", G_CALLBACK(start_recording), NULL);
    g_signal_connect(rec_ui->stop_btn, "clicked", G_CALLBACK(stop_recording), NULL);
}

/* Build Bluetooth panel once and bind to GUI BT helpers */
static void on_bt_selection_changed(GtkTreeSelection *sel, gpointer user_data) {
    (void)user_data;
    GtkTreeView *tv = gtk_tree_selection_get_tree_view(sel);
    gboolean has = gtk_tree_selection_get_selected(sel, NULL, NULL);

    GtkWidget *pair_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "pair_btn"));
    GtkWidget *trust_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "trust_btn"));
    GtkWidget *connect_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "connect_btn"));
    GtkWidget *remove_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "remove_btn"));
    GtkWidget *set_input_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "set_input_btn"));

    /* Default: disable all until we have state */
    if (pair_btn) gtk_widget_set_sensitive(pair_btn, FALSE);
    if (trust_btn) gtk_widget_set_sensitive(trust_btn, FALSE);
    if (connect_btn) gtk_widget_set_sensitive(connect_btn, FALSE);
    if (remove_btn) gtk_widget_set_sensitive(remove_btn, has);
    if (set_input_btn) gtk_widget_set_sensitive(set_input_btn, has);

    if (!has) return;

    /* Determine selected object path */
    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return;

    gchar *obj = NULL;
    gtk_tree_model_get(model, &iter, 1, &obj, -1);
    if (!obj) return;

    gboolean paired = FALSE, trusted = FALSE, connected = FALSE;
    if (gui_bt_get_device_state(obj, &paired, &trusted, &connected) == 0) {
        /* Gate buttons based on Device1 properties */
        if (pair_btn)   gtk_widget_set_sensitive(pair_btn, !paired);
        if (trust_btn)  gtk_widget_set_sensitive(trust_btn, paired && !trusted);
        if (connect_btn)gtk_widget_set_sensitive(connect_btn, paired);
        if (remove_btn) gtk_widget_set_sensitive(remove_btn, TRUE);
    } else {
        /* If state query fails, fall back to selection-based enabling */
        if (pair_btn)   gtk_widget_set_sensitive(pair_btn, TRUE);
        if (trust_btn)  gtk_widget_set_sensitive(trust_btn, TRUE);
        if (connect_btn)gtk_widget_set_sensitive(connect_btn, TRUE);
        if (remove_btn) gtk_widget_set_sensitive(remove_btn, TRUE);
    }
    g_free(obj);
}

static void create_bt_panel(GtkWidget *main_box) {
    GtkWidget *bt_expander = gtk_expander_new("BLUETOOTH");
    gtk_expander_set_expanded(GTK_EXPANDER(bt_expander), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), bt_expander, FALSE, FALSE, 0);

    /* keep a reference for the expander toggle handler */
    g_bt_expander = bt_expander;
    g_signal_connect(G_OBJECT(bt_expander), "notify::expanded", G_CALLBACK(on_any_expander_toggled), NULL);

    GtkWidget *bt_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(bt_expander), bt_vbox);

    /* Discoverable toggle row (for security - disable when not pairing) */
    GtkWidget *disc_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(bt_vbox), disc_row, FALSE, FALSE, 0);
    
    GtkWidget *disc_label = gtk_label_new("Allow device discovery:");
    gtk_box_pack_start(GTK_BOX(disc_row), disc_label, FALSE, FALSE, 0);
    
    GtkWidget *disc_switch = gtk_switch_new();
    /* Initialize from current adapter state (default ON) */
    gboolean current_state = gui_bt_get_adapter_discoverable();
    gtk_switch_set_active(GTK_SWITCH(disc_switch), current_state);
    gtk_box_pack_start(GTK_BOX(disc_row), disc_switch, FALSE, FALSE, 0);
    g_signal_connect(disc_switch, "state-set", G_CALLBACK(on_bt_discoverable_toggled), NULL);
    
    GtkWidget *disc_info = gtk_label_new("(Disable when not pairing for security)");
    gtk_widget_set_opacity(disc_info, 0.7);  /* Dim the hint text */
    gtk_box_pack_start(GTK_BOX(disc_row), disc_info, FALSE, FALSE, 6);

    /* Discovery controls */
    GtkWidget *bt_ctrl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(bt_vbox), bt_ctrl_row, FALSE, FALSE, 0);
    GtkWidget *scan_btn = gtk_button_new_with_label("Scan");
    GtkWidget *stop_btn = gtk_button_new_with_label("Stop");
    gtk_box_pack_start(GTK_BOX(bt_ctrl_row), scan_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_ctrl_row), stop_btn, FALSE, FALSE, 0);
    /* Initial state: allow Scan; Stop disabled until discovery starts */
    gtk_widget_set_sensitive(stop_btn, FALSE);
    /* Cross-reference buttons for easy toggling inside callbacks */
    g_object_set_data(G_OBJECT(scan_btn), "peer_stop_btn", stop_btn);
    g_object_set_data(G_OBJECT(stop_btn),  "peer_scan_btn", scan_btn);
    /* Bind scan/stop to GUI BT so adapter Discovering state toggles sensitivity */
    gui_bt_bind_scan_buttons(scan_btn, stop_btn);

    /* Device list model and view */
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING); /* display, object */
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    /* Make BT device list accessible to the Devices (Playback) panel */
    g_bt_tree = tree;
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Discovered Devices", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    /* Scrolled window wrapper to constrain height (~140px) */
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroller, -1, 140);
    gtk_container_add(GTK_CONTAINER(scroller), tree);
    gtk_box_pack_start(GTK_BOX(bt_vbox), scroller, FALSE, FALSE, 0);

    /* Action buttons */
    GtkWidget *bt_action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(bt_vbox), bt_action_row, FALSE, FALSE, 0);
    GtkWidget *pair_btn = gtk_button_new_with_label("Pair");
    GtkWidget *trust_btn = gtk_button_new_with_label("Trust");
    GtkWidget *connect_btn = gtk_button_new_with_label("Connect");
    GtkWidget *remove_btn = gtk_button_new_with_label("Remove");
    /* Use clearer label for playback routing (this routes output to the selected device) */
    GtkWidget *set_input_btn = gtk_button_new_with_label("Set as Output");
    gtk_box_pack_start(GTK_BOX(bt_action_row), pair_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), trust_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), connect_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), remove_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), set_input_btn, FALSE, FALSE, 0);

    /* Disable action buttons until a device is selected */
    gtk_widget_set_sensitive(pair_btn, FALSE);
    gtk_widget_set_sensitive(trust_btn, FALSE);
    gtk_widget_set_sensitive(connect_btn, FALSE);
    gtk_widget_set_sensitive(remove_btn, FALSE);
    gtk_widget_set_sensitive(set_input_btn, FALSE);

    /* Bind device store to GUI BT helpers, register listeners, and populate existing devices */
    gui_bt_set_device_store_widget(tree, store);
    gui_bt_register_discovery_listeners();
    gui_bt_populate_existing_devices();

    /* Provide access for callbacks to the selected row and selection-driven sensitivity */
    g_object_set_data(G_OBJECT(pair_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(trust_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(connect_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(remove_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(set_input_btn), "device_tree", tree);

    /* Expose buttons via the tree so selection-changed can toggle sensitivity */
    g_object_set_data(G_OBJECT(tree), "pair_btn", pair_btn);
    g_object_set_data(G_OBJECT(tree), "trust_btn", trust_btn);
    g_object_set_data(G_OBJECT(tree), "connect_btn", connect_btn);
    g_object_set_data(G_OBJECT(tree), "remove_btn", remove_btn);
    g_object_set_data(G_OBJECT(tree), "set_input_btn", set_input_btn);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    g_signal_connect(sel, "changed", G_CALLBACK(on_bt_selection_changed), NULL);

    /* Connect signals */
    g_signal_connect(scan_btn, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(pair_btn, "clicked", G_CALLBACK(on_pair_clicked), NULL);
    g_signal_connect(trust_btn, "clicked", G_CALLBACK(on_trust_clicked), NULL);
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_clicked), NULL);
    g_signal_connect(set_input_btn, "clicked", G_CALLBACK(on_bt_set_output_clicked), NULL);
}

/* Main function starts here */
int main(int argc, char *argv[]) {
    /* Forward-declared Bluetooth helpers are defined in src/gui_bt.c */
    extern int gui_bt_init(void);
    extern void gui_bt_shutdown(void);

    gtk_init(&argc, &argv);

    /* Initialize Bluetooth GUI helpers (register BlueZ agent, obtain system bus).
       Failures are non-fatal for systems without Bluetooth but will be logged. */
    if (gui_bt_init() != 0) {
        g_warning("mxeq: gui_bt_init failed or BlueZ agent not available; continuing without Bluetooth controls");
    }

    // Set the default application icon (matches desktop entry Icon=alsa-sound-connect)
    gtk_window_set_default_icon_name("alsa-sound-connect");

    MixerData mixer_data = {0};
    EQData eq_data = {0};
    init_alsa_mixer(&mixer_data, 0);  /* Start with card 0 (internal) */
    init_alsa_eq(&eq_data);
    /* Store global reference for dynamic mixer switching */
    g_mixer_data = &mixer_data;
    /* Ensure per-user ALSA override exists so Recorder works without root/system writes */
    extern void ensure_user_asoundrc_bootstrap(void);
    ensure_user_asoundrc_bootstrap();

    // Create a CSS provider for custom styling
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "frame {"
        "  border-width: 2px;"
        "  border-style: solid;"
        "  border-color: #333333;"
        "  box-shadow: none;"
        "  border-radius: 4px;"
        "}",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    // Create window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Alsa Sound Connect");
    // Compact width for screen fit, homogeneous spacing keeps professional look
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 260);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Expose main window to expander handler so it can resize back to compact height */
    g_main_window = window;

    // Main vertical box (no global scroller) - keep height tight when expanders are collapsed.
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4); // Tighter spacing
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Mixer frame with border
    GtkWidget *mixer_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(main_box), mixer_frame, FALSE, FALSE, 0);

    // Scrolled window for mixer (vertical scrolling only, max ~300px height)
    GtkWidget *mixer_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mixer_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(mixer_scroller, -1, 300);
    gtk_container_add(GTK_CONTAINER(mixer_frame), mixer_scroller);

    // Mixer grid (homogeneous columns, max 8 per row, auto-wrap to new rows)
    GtkWidget *mixer_box = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(mixer_box), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(mixer_box), FALSE);
    gtk_grid_set_column_spacing(GTK_GRID(mixer_box), 2);
    gtk_grid_set_row_spacing(GTK_GRID(mixer_box), 5);
    gtk_container_set_border_width(GTK_CONTAINER(mixer_box), 5);
    gtk_container_add(GTK_CONTAINER(mixer_scroller), mixer_box);
    
    /* Store mixer_box reference for dynamic rebuild */
    mixer_data.mixer_box = mixer_box;

    // Add sliders for each mixer channel (if none found, show an informative message)
    if (mixer_data.num_channels == 0) {
        GtkWidget *no_mixer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_hexpand(no_mixer_box, TRUE);
        gtk_widget_set_halign(no_mixer_box, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(mixer_box), no_mixer_box, 0, 0, 8, 1);

        GtkWidget *no_mixer_label = gtk_label_new(
            "No mixer controls were detected on the default ALSA device.\n"
            "Audio may still play, but mixer sliders are unavailable.\n\n"
            "Possible fixes:\n"
            "• Ensure the default ALSA device has mixer elements (try 'alsamixer').\n"
            "• Check that ALSA's default device maps to your hardware (see /proc/asound/cards).\n"
            "• If using BlueALSA-only profiles, there may be no system mixer to control."
        );
        gtk_label_set_justify(GTK_LABEL(no_mixer_label), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(no_mixer_box), no_mixer_label, TRUE, TRUE, 8);
    } else {
        for (int i = 0; i < mixer_data.num_channels; i++) {
            int col = i % 8;  // Max 8 columns per row
            int row = i / 8;  // Automatic row wrapping
            
            GtkWidget *channel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_grid_attach(GTK_GRID(mixer_box), channel_box, col, row, 1, 1);

            GtkWidget *label = gtk_label_new(mixer_data.channels[i].channel_name);
            gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(channel_box), label, FALSE, FALSE, 5);

            mixer_data.channels[i].scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 1, 0.01);
            gtk_range_set_inverted(GTK_RANGE(mixer_data.channels[i].scale), TRUE);
            gtk_scale_set_draw_value(GTK_SCALE(mixer_data.channels[i].scale), TRUE);
            gtk_scale_set_value_pos(GTK_SCALE(mixer_data.channels[i].scale), GTK_POS_BOTTOM);
            gtk_widget_set_size_request(mixer_data.channels[i].scale, -1, 150);
            gtk_box_pack_start(GTK_BOX(channel_box), mixer_data.channels[i].scale, TRUE, TRUE, 0);
            g_signal_connect(mixer_data.channels[i].scale, "value-changed", G_CALLBACK(slider_changed), &mixer_data.channels[i]);

            long min, max, value;
            if (mixer_data.channels[i].is_capture) {
                snd_mixer_selem_get_capture_volume_range(mixer_data.channels[i].elem, &min, &max);
                snd_mixer_selem_get_capture_volume(mixer_data.channels[i].elem, 0, &value);
            } else {
                snd_mixer_selem_get_playback_volume_range(mixer_data.channels[i].elem, &min, &max);
                snd_mixer_selem_get_playback_volume(mixer_data.channels[i].elem, 0, &value);
            }
            gtk_range_set_value(GTK_RANGE(mixer_data.channels[i].scale), (double)(value - min) / (max - min));

            /* Optional per-channel Mute (playback) or Enable (capture) checkbox */
            mixer_data.channels[i].mute_check = NULL;
            if (mixer_data.channels[i].is_capture) {
                /* Capture controls: expose Enable checkbox (checked=capture enabled) */
                if (snd_mixer_selem_has_capture_switch(mixer_data.channels[i].elem)) {
                    int sw = 0;
                    snd_mixer_selem_get_capture_switch(mixer_data.channels[i].elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
                    GtkWidget *enable = gtk_check_button_new_with_label("Enable");
                    gtk_widget_set_halign(enable, GTK_ALIGN_CENTER);
                    gtk_widget_set_margin_top(enable, 4);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable), sw ? TRUE : FALSE);
                    gtk_box_pack_start(GTK_BOX(channel_box), enable, FALSE, FALSE, 2);
                    g_signal_connect(enable, "toggled", G_CALLBACK(on_mute_toggled), &mixer_data.channels[i]);
                    mixer_data.channels[i].mute_check = enable;
                }
            } else {
                /* Playback controls: expose Mute checkbox (checked=muted) */
                if (snd_mixer_selem_has_playback_switch(mixer_data.channels[i].elem)) {
                    int sw = 1;
                    snd_mixer_selem_get_playback_switch(mixer_data.channels[i].elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
                    GtkWidget *mute = gtk_check_button_new_with_label("Mute");
                    gtk_widget_set_halign(mute, GTK_ALIGN_CENTER);
                    gtk_widget_set_margin_top(mute, 4);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute), sw ? FALSE : TRUE);
                    gtk_box_pack_start(GTK_BOX(channel_box), mute, FALSE, FALSE, 2);
                    g_signal_connect(mute, "toggled", G_CALLBACK(on_mute_toggled), &mixer_data.channels[i]);
                    mixer_data.channels[i].mute_check = mute;
                }
            }
            /* Removed padding label so the mute button sits directly under the slider */
        }
    }

    // EQ and Recording expander (collapsed by default to minimize vertical footprint)
    GtkWidget *eq_expander = gtk_expander_new("EQ and Recording");
    gtk_expander_set_expanded(GTK_EXPANDER(eq_expander), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), eq_expander, FALSE, FALSE, 0);

    /* keep a reference so the expander toggle handler can resize the window */
    g_eq_expander = eq_expander;
    g_signal_connect(G_OBJECT(eq_expander), "notify::expanded", G_CALLBACK(on_any_expander_toggled), NULL);

    /* Put the EQ content inside its own scrolled window so it only requests space
       when expanded. This prevents the app window from leaving large blank space
       when the expander is collapsed. */
    GtkWidget *eq_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(eq_scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(eq_scroller, -1, 220); /* hint when expanded */

    GtkWidget *eq_content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(eq_scroller), eq_content_vbox);
    gtk_container_add(GTK_CONTAINER(eq_expander), eq_scroller);

    // EQ sliders row
    GtkWidget *eq_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(eq_box), 2); // Reduced padding
    gtk_widget_set_size_request(eq_box, -1, 200);
    gtk_box_pack_start(GTK_BOX(eq_content_vbox), eq_box, FALSE, FALSE, 0);

    // Add sliders for each EQ band
    for (int i = 0; i < eq_data.num_bands; i++) {
        if (!eq_data.bands[i].val) {
            fprintf(stderr, "Warning: NULL val for band %d: %s\n", i, eq_data.bands[i].band_name);
            continue;
        }

        GtkWidget *band_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_box_pack_start(GTK_BOX(eq_box), band_box, TRUE, TRUE, 5);

        // Use shorter labels with Hz/kHz units (e.g., "31 Hz" or "1 kHz")
        char short_label[16];
        const char *freq_start = eq_data.bands[i].band_name + 4; // Skip "00. " prefix
        const char *end = strstr(freq_start, " Playback Volume");
        if (end) {
            strncpy(short_label, freq_start, end - freq_start);
            short_label[end - freq_start] = '\0';
        } else {
            strncpy(short_label, freq_start, sizeof(short_label) - 1);
            short_label[sizeof(short_label) - 1] = '\0';
        }
        // Strip existing Hz or kHz
        char *unit = strstr(short_label, " Hz");
        if (!unit) unit = strstr(short_label, " kHz");
        if (unit) *unit = '\0';
        /* Append correct unit based on frequency */
        if (strcmp(short_label, "1") == 0 || strcmp(short_label, "2") == 0 || strcmp(short_label, "4") == 0 || strcmp(short_label, "8") == 0 || strcmp(short_label, "16") == 0) {
            strcat(short_label, " kHz");
        } else {
            strcat(short_label, " Hz");
        }
        GtkWidget *label = gtk_label_new(short_label);
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(band_box), label, FALSE, FALSE, 5);

        eq_data.bands[i].scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 1, 0.01);
        gtk_range_set_inverted(GTK_RANGE(eq_data.bands[i].scale), TRUE);
        gtk_scale_set_draw_value(GTK_SCALE(eq_data.bands[i].scale), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(eq_data.bands[i].scale), GTK_POS_BOTTOM);
        gtk_widget_set_size_request(eq_data.bands[i].scale, -1, 150);
        gtk_box_pack_start(GTK_BOX(band_box), eq_data.bands[i].scale, TRUE, TRUE, 0);
        g_signal_connect(eq_data.bands[i].scale, "value-changed", G_CALLBACK(eq_slider_changed), &eq_data.bands[i]);

        long value = snd_ctl_elem_value_get_integer(eq_data.bands[i].val, 0);
        gtk_range_set_value(GTK_RANGE(eq_data.bands[i].scale), (double)value / 100.0); // Map 0-100 to 0-1

        GtkWidget *padding = gtk_label_new("");
        gtk_box_pack_start(GTK_BOX(band_box), padding, FALSE, FALSE, 5);
    }

    // Preset controls: entry, save button, and combo box in a grid (under EQ sliders)
    GtkWidget *preset_box = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(preset_box), 5);
    gtk_widget_set_halign(preset_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(eq_content_vbox), preset_box, FALSE, FALSE, 1); // Kept 1px margin

    GtkWidget *preset_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(preset_entry), "Enter name for new pre-set");
    gtk_widget_set_size_request(preset_entry, 250, -1); // 250px width
    gtk_grid_attach(GTK_GRID(preset_box), preset_entry, 1, 0, 1, 1);

    GtkWidget *save_button = gtk_button_new_with_label("Save Pre-set");
    gtk_grid_attach(GTK_GRID(preset_box), save_button, 2, 0, 1, 1);

    GtkWidget *preset_combo = gtk_combo_box_text_new();
    gtk_widget_set_size_request(preset_combo, 250, -1); // 250px width
    gtk_grid_attach(GTK_GRID(preset_box), preset_combo, 3, 0, 1, 1);

    // Add spacers for equal left/right margins
    GtkWidget *spacer_left = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(preset_box), spacer_left, 0, 0, 1, 1);
    gtk_widget_set_hexpand(spacer_left, TRUE);
    GtkWidget *spacer_right = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(preset_box), spacer_right, 4, 0, 1, 1);
    gtk_widget_set_hexpand(spacer_right, TRUE);

    // Store references for callbacks
    g_object_set_data(G_OBJECT(save_button), "preset_entry", preset_entry);
    g_object_set_data(G_OBJECT(save_button), "preset_combo", preset_combo);
    g_object_set_data(G_OBJECT(save_button), "window", window);

    // Connect signals
    g_signal_connect(save_button, "clicked", G_CALLBACK(save_preset), &eq_data);
    g_signal_connect(preset_combo, "changed", G_CALLBACK(apply_preset), &eq_data);

    // Load existing presets into combo box
    load_presets(GTK_COMBO_BOX_TEXT(preset_combo));

    // Recorder UI directly under presets inside EQ expander
    create_recorder_ui(eq_content_vbox);

    /* Bluetooth panel (collapsible) */
    create_bt_panel(main_box);
 
    /* Devices panel (Playback) */
    create_devices_panel(main_box);

    gtk_widget_show_all(window);
    gtk_main();

    /* Unregister GUI discovery listeners and shutdown GUI Bluetooth helpers before exiting.
       These calls are safe no-ops if gui_bt wasn't initialized. */
    extern void gui_bt_unregister_discovery_listeners(void);
    extern void gui_bt_shutdown(void);
    gui_bt_unregister_discovery_listeners();
    gui_bt_shutdown();

    cleanup_alsa(&mixer_data, &eq_data);
    return 0;
}


static gboolean file_exists_readable(const char *path) {
    if (!path) return FALSE;
    FILE *f = fopen(path, "r");
    if (!f) return FALSE;
    fclose(f);
    return TRUE;
}

/* ---- Input Source selector helpers (user-level override in ~/.asoundrc) ----
 * We keep system defaults in /etc/asound.conf(.d), but the GUI writes a managed
 * block in ~/.asoundrc that takes precedence without needing root. The block is
 * delimited with:
 *   # BEGIN jack-bridge
 *   ... (managed content)
 *   # END jack-bridge
 * Content written:
 *   - pcm.current_input -> one of: input_card0, input_usb, input_hdmi, input_bt
 *   - Optional: pcm.input_bt_raw + pcm.input_bt when a Bluetooth MAC is chosen
 */
static const char *CURRENT_INPUT_PATH = "/etc/asound.conf.d/current_input.conf"; /* still used for fallback read */

static const char *JB_BEGIN = "# BEGIN jack-bridge";
static const char *JB_END   = "# END jack-bridge";

/* Simple helper to scan a text file for a substring (case-sensitive) */
static gboolean file_contains_substr(const char *path, const char *needle) {
    if (!path || !needle) return FALSE;
    FILE *f = fopen(path, "r");
    if (!f) return FALSE;
    char buf[512];
    gboolean found = FALSE;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, needle) != NULL) { found = TRUE; break; }
    }
    fclose(f);
    return found;
}

/* Detect USB card number (returns -1 if no USB found) */
static int get_usb_card_number(void) {
    FILE *f = fopen("/proc/asound/cards", "r");
    if (!f) return -1;
    char line[512];
    int card = -1;
    while (fgets(line, sizeof(line), f)) {
        /* Look for "card N: ... [USB" or "card N: USB" */
        if (strstr(line, "USB")) {
            /* Extract card number from "card N:" prefix */
            if (sscanf(line, " %d", &card) == 1) {
                fclose(f);
                return card;
            }
        }
    }
    fclose(f);
    return -1;
}

static gboolean is_usb_present(void) {
    return (get_usb_card_number() >= 0);
}
static gboolean is_hdmi_present(void) {
    if (file_contains_substr("/proc/asound/cards", "HDMI")) return TRUE;
    /* Fallback: check aplay -l for HDMI devices (e.g. on HDA Intel cards) */
    gchar *out = NULL;
    if (g_spawn_command_line_sync("aplay -l", &out, NULL, NULL, NULL)) {
        if (out && strstr(out, "HDMI")) {
            g_free(out);
            return TRUE;
        }
        g_free(out);
    }
    return FALSE;
}
static gboolean is_bt_present(void) {
    if (g_file_test("/usr/bin/bluealsa", G_FILE_TEST_IS_REGULAR) ||
        g_file_test("/usr/sbin/bluealsa", G_FILE_TEST_IS_REGULAR)) return TRUE;
    int status = 0;
    gboolean ok = g_spawn_command_line_sync("pidof bluealsad", NULL, NULL, &status, NULL);
    if (ok && WIFEXITED(status) && WEXITSTATUS(status) == 0) return TRUE;
    return FALSE;
}

/* System default reader: parse /etc/asound.conf.d/current_input.conf */
static gchar *read_current_input(void) {
    FILE *f = fopen(CURRENT_INPUT_PATH, "r");
    if (!f) return NULL;
    char line[512];
    gchar *result = NULL;
    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "slave.pcm");
        if (p) {
            char *q = strchr(p, '\"');
            if (q) {
                char *q2 = strchr(q+1, '\"');
                if (q2) {
                    *q2 = '\0';
                    result = g_strdup(q+1);
                    break;
                }
            }
        }
    }
    fclose(f);
    return result;
}

/* Build ~/.asoundrc path */
static char *user_asoundrc_path(void) {
    const char *home = g_get_home_dir();
    return g_build_filename(home, ".asoundrc", NULL);
}

/* XDG-style per-user config directory used for current_input fragment */
static char *user_config_dir(void) {
    const char *home = g_get_home_dir();
    return g_build_filename(home, ".config", "jack-bridge", NULL);
}

/* Full path to per-user current_input.conf fragment */
static char *user_current_input_conf_path(void) {
    char *dir = user_config_dir();
    char *path = g_build_filename(dir, "current_input.conf", NULL);
    g_free(dir);
    return path;
}

/* Compose the content of ~/.config/jack-bridge/current_input.conf
 * Mirrors the format of /etc/asound.conf.d/current_input.conf and embeds
 * Bluetooth input definitions when bt_mac_opt is provided. */
static char *compose_user_current_input_conf(const char *pcm_current, const char *bt_mac_opt) {
    GString *s = g_string_new("");
    if (bt_mac_opt && *bt_mac_opt) {
        g_string_append_printf(s,
            "pcm.input_bt_raw {\n"
            "    type bluealsa\n"
            "    device \"%s\"\n"
            "    profile \"a2dp\"\n"
            "}\n"
            "\n"
            "pcm.input_bt {\n"
            "    type plug\n"
            "    slave.pcm \"input_bt_raw\"\n"
            "}\n"
            "\n", bt_mac_opt);
    }
    g_string_append_printf(s,
        "pcm.current_input {\n"
        "    type plug\n"
        "    slave.pcm \"%s\"\n"
        "}\n", pcm_current ? pcm_current : "input_card0");
    return g_string_free(s, FALSE);
}

/* Write ~/.config/jack-bridge/current_input.conf atomically */
static int write_user_current_input_conf(const char *pcm_current, const char *bt_mac_opt) {
    char *dir = user_config_dir();
    if (g_mkdir_with_parents(dir, 0755) != 0) {
        g_free(dir);
        return -1;
    }
    char *path = user_current_input_conf_path();
    char *content = compose_user_current_input_conf(pcm_current, bt_mac_opt);
    int rc = write_string_atomic(path, content);
    g_free(content);
    g_free(path);
    g_free(dir);
    return rc;
}

/* Load whole file into memory (NULL if not exists/empty) */
static char *load_file_to_string(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = g_malloc(sz + 1);
    size_t rd = fread(buf, 1, sz, f);
    fclose(f);
    buf[rd] = '\0';
    return buf;
}

/* Atomic write: path.tmp then rename */
static int write_string_atomic(const char *path, const char *content) {
    if (!path || !content) return -1;
    char *tmp = g_strdup_printf("%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) { g_free(tmp); return -1; }
    if (fputs(content, f) == EOF) {
        fclose(f);
        g_unlink(tmp);
        g_free(tmp);
        return -1;
    }
    fclose(f);
    if (g_rename(tmp, path) != 0) {
        g_unlink(tmp);
        g_free(tmp);
        return -1;
    }
    g_free(tmp);
    return 0;
}

/* Strip existing managed block from content (if present). Returns newly allocated content. */
static char *strip_managed_block(const char *src) {
    if (!src) return g_strdup("");
    const char *begin = strstr(src, JB_BEGIN);
    if (!begin) return g_strdup(src);
    const char *end = strstr(begin, JB_END);
    if (!end) return g_strdup(src); /* malformed; leave as-is */
    end += strlen(JB_END);
    /* Include trailing newline after END if present */
    while (*end == '\r' || *end == '\n') end++;
    GString *out = g_string_new_len(src, (gssize)(begin - src));
    /* Trim trailing blank lines in prefix */
    while (out->len > 0 && (out->str[out->len - 1] == '\n' || out->str[out->len - 1] == '\r'))
        g_string_truncate(out, out->len - 1);
    g_string_append_c(out, '\n');
    return g_string_free(out, FALSE);
}

/* Compose managed block for ~/.asoundrc (include-only pointing at per-user fragment) */
static char *compose_managed_block(const char *pcm_current, const char *bt_mac_opt) {
    /* Compose a unified managed block that includes both the per-user
     * current_input and current_output fragments. This ensures ~/.asoundrc
     * consistently points to both user-managed fragments so both recording
     * and playback overrides take effect for non-JACK ALSA apps.
     */
    (void)pcm_current;
    (void)bt_mac_opt;
    char *dir = user_config_dir();
    char *in_path = g_build_filename(dir, "current_input.conf", NULL);
    char *out_path = g_build_filename(dir, "current_output.conf", NULL);
    GString *s = g_string_new("");
    g_string_append_printf(s, "%s\n", JB_BEGIN);
    g_string_append(s, "# Managed by jack-bridge GUI — do not edit between markers.\n");
    g_string_append_printf(s, "include \"%s\"\n", in_path);
    g_string_append_printf(s, "include \"%s\"\n", out_path);
    g_string_append_printf(s, "%s\n", JB_END);
    g_free(in_path);
    g_free(out_path);
    g_free(dir);
    return g_string_free(s, FALSE);
}

/* Write per-user current_input fragment and ensure ~/.asoundrc includes it; preserves user content outside the block */
static int write_user_asoundrc_block(const char *pcm_current, const char *bt_mac_opt) {
    /* First write the per-user fragment that mirrors /etc/asound.conf.d/current_input.conf */
    if (write_user_current_input_conf(pcm_current, bt_mac_opt) != 0) {
        return -1;
    }

    /* Then ensure ~/.asoundrc has our include block */
    char *path = user_asoundrc_path();
    char *orig = load_file_to_string(path);
    char *prefix = strip_managed_block(orig ? orig : "");
    char *block = compose_managed_block(pcm_current, bt_mac_opt);
    GString *final = g_string_new(prefix);
    if (final->len > 0 && final->str[final->len - 1] != '\n') g_string_append_c(final, '\n');
    g_string_append(final, "\n");
    g_string_append(final, block);
    g_string_append(final, "\n");
    int rc = write_string_atomic(path, final->str);
    g_string_free(final, TRUE);
    if (orig) g_free(orig);
    g_free(prefix);
    g_free(block);
    g_free(path);
    return rc;
}

/* Read pcm.current_input from per-user fragment (~/.config/jack-bridge/current_input.conf), or NULL if not present */
static gchar *read_user_current_input(void) {
    char *path = user_current_input_conf_path();
    FILE *f = fopen(path, "r");
    if (!f) { g_free(path); return NULL; }
    char line[512];
    gchar *result = NULL;
    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "slave.pcm");
        if (p) {
            char *q = strchr(p, '\"');
            if (q) {
                char *q2 = strchr(q + 1, '\"');
                if (q2) {
                    *q2 = '\0';
                    result = g_strdup(q + 1);
                    break;
                }
            }
        }
    }
    fclose(f);
    g_free(path);
    return result;
}

/* Bootstrap: ensure include exists and per-user fragment mirrors system default so apps can record immediately */
void ensure_user_asoundrc_bootstrap(void) {
    gchar *u = read_user_current_input();
    if (u) { g_free(u); return; }
    gchar *sys = read_current_input();
    const char *initial = sys ? sys : "input_card0";
    /* Write per-user fragment and include block */
    write_user_asoundrc_block(initial, NULL);
    if (sys) g_free(sys);
}




/* Input Devices panel removed in favor of the new Playback "Devices" panel.
 * Keep a small unused stub to avoid build warnings/errors on systems that
 * reference this symbol in older branches.
 */


/* ---- Devices panel (Internal / USB / HDMI / Bluetooth) ----
 * Provides runtime JACK routing without restarting jackd by invoking:
 *   /usr/local/lib/jack-bridge/jack-route-select {internal|usb|hdmi|bluetooth}
 * The helper persists selection into /etc/jack-bridge/devices.conf
 */
static const char *ROUTE_HELPER = "/usr/local/lib/jack-bridge/jack-route-select";
static const char *DEVCONF_PATH = "/etc/jack-bridge/devices.conf"; /* system-wide default (user override checked in loader) */

static gchar *load_preferred_output(void) {
    /* Prefer per-user config (~/.config/jack-bridge/devices.conf), fallback to system DEVCONF_PATH */
    gchar *user_conf = NULL;
    {
        const char *home = g_get_home_dir();
        if (home) user_conf = g_build_filename(home, ".config", "jack-bridge", "devices.conf", NULL);
    }
    const char *paths[3];
    int pi = 0;
    if (user_conf) paths[pi++] = user_conf;
    paths[pi++] = DEVCONF_PATH;
    paths[pi] = NULL;

    for (int i = 0; paths[i]; i++) {
        const char *p = paths[i];
        if (!file_exists_readable(p)) continue;
        FILE *f = fopen(p, "r");
        if (!f) continue;
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (g_str_has_prefix(line, "PREFERRED_OUTPUT=")) {
                char *eq = strchr(line, '=');
                if (eq) {
                    char *val = eq + 1;
                    while (*val == ' ' || *val == '\t' || *val == '\"') val++;
                    char *end = val + strlen(val);
                    while (end > val && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\"')) end--;
                    *end = '\0';
                    gchar *out = g_strdup(val && *val ? val : "internal");
                    fclose(f);
                    if (user_conf) g_free(user_conf);
                    return out;
                }
            }
        }
        fclose(f);
    }
    if (user_conf) g_free(user_conf);
    return g_strdup("internal");
}

static gboolean route_to_target_async_with_arg(const char *target, const char *opt_arg) {
    if (!target || !*target) return FALSE;
    if (!file_exists_readable(ROUTE_HELPER)) {
        g_warning("route helper missing: %s", ROUTE_HELPER);
        return FALSE;
    }
    gchar *cmd = NULL;
    if (opt_arg && *opt_arg) cmd = g_strdup_printf("%s %s %s", ROUTE_HELPER, target, opt_arg);
    else cmd = g_strdup_printf("%s %s", ROUTE_HELPER, target);
    GError *err = NULL;
    gboolean ok = g_spawn_command_line_async(cmd, &err);
    if (!ok) {
        g_warning("failed to invoke %s: %s", cmd, err ? err->message : "unknown");
        if (err) g_error_free(err);
    }
    g_free(cmd);
    return ok;
}
static gboolean route_to_target_async(const char *target) {
    return route_to_target_async_with_arg(target, NULL);
}

typedef struct {
    GtkWidget *rb_internal;
    GtkWidget *rb_usb;
    GtkWidget *rb_hdmi;
    GtkWidget *rb_bt;
} DevicesUI;

static void on_device_radio_toggled(GtkToggleButton *tb, gpointer user_data) {
    (void)user_data;
    if (!gtk_toggle_button_get_active(tb)) return;
    const char *label = gtk_button_get_label(GTK_BUTTON(tb));
    if (!label) return;

    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(tb));
    gboolean ok = TRUE;
    gchar *mac = NULL;

    if (g_strcmp0(label, "Internal") == 0) {
        ok = route_to_target_async("internal");
        /* Switch mixer to show internal card (hw:0) controls */
        rebuild_mixer_for_card(0);
    } else if (g_strcmp0(label, "USB") == 0) {
        ok = route_to_target_async("usb");
        /* Switch mixer to show USB card controls */
        int usb_card = get_usb_card_number();
        if (usb_card >= 0) {
            rebuild_mixer_for_card(usb_card);
        }
    } else if (g_strcmp0(label, "HDMI") == 0) {
        ok = route_to_target_async("hdmi");
        /* HDMI uses internal card for capture, so show internal mixer */
        rebuild_mixer_for_card(0);
    } else if (g_strcmp0(label, "Bluetooth") == 0) {
        /* Bluetooth uses internal card for capture, so show internal mixer */
        rebuild_mixer_for_card(0);
        
        /* Get MAC from BT panel selection */
        char *obj = NULL;
        if (g_bt_tree && GTK_IS_TREE_VIEW(g_bt_tree)) {
            obj = (char*)tree_get_selected_obj(GTK_TREE_VIEW(g_bt_tree));
            if (obj) mac = mac_from_bluez_object(obj);
            if (obj) g_free(obj);
        }
        
        if (!mac) {
            GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "No Bluetooth device selected.\n\nPlease:\n1. Expand 'BLUETOOTH' panel\n2. Connect a device\n3. Try again");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            /* Revert radio to previous selection (don't leave Bluetooth selected if it failed) */
            g_signal_handlers_block_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            gtk_toggle_button_set_active(tb, FALSE);
            if (g_rb_internal) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_internal), TRUE);
            g_signal_handlers_unblock_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            return;
        }
        
        /* Call helper SYNCHRONOUSLY so we can verify result */
        gchar *cmd = g_strdup_printf("%s bluetooth %s", ROUTE_HELPER, mac);
        gint status = 0;
        GError *err = NULL;
        ok = g_spawn_command_line_sync(cmd, NULL, NULL, &status, &err);
        g_free(cmd);
        
        if (!ok || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Failed to set Bluetooth output.\n\nError: %s\n\nCheck that device is connected and BlueALSA daemon is running.",
                                                  err ? err->message : "routing helper failed");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            if (err) g_error_free(err);
            /* Revert radio to Internal */
            g_signal_handlers_block_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            gtk_toggle_button_set_active(tb, FALSE);
            if (g_rb_internal) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_internal), TRUE);
            g_signal_handlers_unblock_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            g_free(mac);
            return;
        }
        
        /* Give ports time to spawn */
        g_usleep(1000000); /* 1 second */
        
        /* Verify ports exist */
        if (!bluealsa_ports_exist()) {
            GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Bluetooth ports failed to spawn.\n\nPossible causes:\n• Device disconnected\n• BlueALSA daemon not running\n• No A2DP transport available\n\nCheck /tmp/jack-route-select.log");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            /* Revert radio to Internal */
            g_signal_handlers_block_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            gtk_toggle_button_set_active(tb, FALSE);
            if (g_rb_internal) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_internal), TRUE);
            g_signal_handlers_unblock_by_func(tb, G_CALLBACK(on_device_radio_toggled), NULL);
            g_free(mac);
            return;
        }
        
        /* Success! Show confirmation */
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Bluetooth output ready!\n\nDevice: %s\nPorts: bluealsa:playback_1/2\n\nAudio will play through Bluetooth.", mac);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(mac);
        return;
    } else {
        return;
    }

    if (!ok) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "Routing helper is missing or failed.\nExpected at: %s\nRun: sudo ./contrib/install.sh",
                                              ROUTE_HELPER);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
    }
}

static void create_devices_panel(GtkWidget *main_box) {
    GtkWidget *dev_expander = gtk_expander_new("Devices");
    gtk_expander_set_expanded(GTK_EXPANDER(dev_expander), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), dev_expander, FALSE, FALSE, 0);

    /* Connect to window resize handler like other expanders */
    g_dev_expander = dev_expander;
    g_signal_connect(G_OBJECT(dev_expander), "notify::expanded", G_CALLBACK(on_any_expander_toggled), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(dev_expander), vbox);

    /* Radio buttons row */
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), row, FALSE, FALSE, 0);

    DevicesUI *ui = g_new0(DevicesUI, 1);

    ui->rb_internal = gtk_radio_button_new_with_label(NULL, "Internal");
    gtk_box_pack_start(GTK_BOX(row), ui->rb_internal, FALSE, FALSE, 0);

    ui->rb_usb = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ui->rb_internal), "USB");
    gtk_box_pack_start(GTK_BOX(row), ui->rb_usb, FALSE, FALSE, 0);

    ui->rb_hdmi = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ui->rb_internal), "HDMI");
    gtk_box_pack_start(GTK_BOX(row), ui->rb_hdmi, FALSE, FALSE, 0);

    ui->rb_bt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ui->rb_internal), "Bluetooth");
    gtk_box_pack_start(GTK_BOX(row), ui->rb_bt, FALSE, FALSE, 0);

    /* Phase 3: Store global references for state synchronization from Bluetooth panel */
    g_rb_internal = ui->rb_internal;
    g_rb_usb = ui->rb_usb;
    g_rb_hdmi = ui->rb_hdmi;
    g_rb_bt = ui->rb_bt;

    /* Presence-based sensitivity (no hardcoding) */
    gtk_widget_set_sensitive(ui->rb_internal, TRUE);
    gtk_widget_set_sensitive(ui->rb_usb, is_usb_present());
    gtk_widget_set_sensitive(ui->rb_hdmi, is_hdmi_present());
    gtk_widget_set_sensitive(ui->rb_bt, is_bt_present());

    g_signal_connect(ui->rb_internal, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_usb, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_hdmi, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_bt, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);

    /* Initialize from persisted preference, but fall back if device not present */
    gchar *pref = load_preferred_output();
    gboolean have_usb = gtk_widget_get_sensitive(ui->rb_usb);
    gboolean have_hdmi = gtk_widget_get_sensitive(ui->rb_hdmi);
    gboolean have_bt = gtk_widget_get_sensitive(ui->rb_bt);

    if (g_strcmp0(pref, "usb") == 0 && have_usb) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_usb), TRUE);
    } else if (g_strcmp0(pref, "hdmi") == 0 && have_hdmi) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_hdmi), TRUE);
    } else if (g_strcmp0(pref, "bluetooth") == 0 && have_bt) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_bt), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_internal), TRUE);
    }
    g_free(pref);
}


/* ---- Bluetooth: "Set as input" implementation (writes BlueALSA MAC + switches current_input) ---- */

/* Helper: derive MAC AA:BB:CC:DD:EE:FF from BlueZ object path ".../dev_AA_BB_CC_DD_EE_FF";
   if input is already a MAC, return a duplicate as-is. Caller must g_free(). */
static char *mac_from_bluez_object(const char *s) {
    if (!s) return NULL;
    if (strchr(s, '/') == NULL) {
        /* assume already MAC */
        return g_strdup(s);
    }
    const char *last = strrchr(s, '/');
    if (!last) return NULL;
    /* Expect "dev_XX_XX_..." */
    const char *p = last + 1;
    const char *prefix = "dev_";
    size_t plen = strlen(prefix);
    if (strncmp(p, prefix, plen) != 0) {
        /* Unexpected form; best-effort: take tail and replace '_' with ':' */
        char *dup = g_strdup(p);
        for (char *q = dup; *q; ++q) if (*q == '_') *q = ':';
        return dup;
    }
    p += plen;
    char *mac = g_strdup(p);
    /* Trim any trailing path junk */
    for (char *q = mac; *q; ++q) {
        if (*q == '/') { *q = '\0'; break; }
    }
    for (char *q = mac; *q; ++q) if (*q == '_') *q = ':';
    return mac;
}

/* Write /etc/asound.conf.d/input_bt.conf to map input_bt -> bluealsa MAC (unused with per-user override) */
static int __attribute__((unused)) write_bt_input_override(const char *mac) {
    if (!mac || strlen(mac) < 11) return -1;
    const char *dst = "/etc/asound.conf.d/input_bt.conf";

    /* Ensure parent dir exists */
    char *dir = g_path_get_dirname(dst);
    if (dir && dir[0]) g_mkdir_with_parents(dir, 0755);
    if (dir) g_free(dir);

    char *tmp = g_strdup_printf("%s.tmp", dst);
    FILE *f = fopen(tmp, "w");
    if (!f) { g_free(tmp); return -1; }

    /* Minimal, safe-to-override mapping for BlueALSA capture */
    fprintf(f,
            "pcm.input_bt_raw {\n"
            "    type bluealsa\n"
            "    device \"%s\"\n"
            "    profile \"a2dp\"\n"
            "}\n"
            "\n"
            "pcm.input_bt {\n"
            "    type plug\n"
            "    slave.pcm \"input_bt_raw\"\n"
            "}\n", mac);

    fclose(f);

    if (g_rename(tmp, dst) != 0) {
        g_unlink(tmp);
        g_free(tmp);
        return -1;
    }
    g_free(tmp);
    return 0;
}


/* Phase 3: Idle callback to sync Devices panel state after Bluetooth routing completes */
static gboolean sync_devices_panel_to_bluetooth_idle(gpointer user_data) {
    (void)user_data;
    /* Block the signal temporarily to prevent recursive routing calls */
    if (g_rb_bt && GTK_IS_TOGGLE_BUTTON(g_rb_bt)) {
        g_signal_handlers_block_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_bt), TRUE);
        g_signal_handlers_unblock_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
    }
    return G_SOURCE_REMOVE;
}

/* Helper: check if bluealsa JACK ports exist (returns TRUE if found) */
static gboolean bluealsa_ports_exist(void) {
    gchar *out = NULL;
    gint status = 0;
    
    if (!g_spawn_command_line_sync("jack_lsp", &out, NULL, &status, NULL)) {
        return FALSE;
    }
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        if (out) g_free(out);
        return FALSE;
    }
    
    gboolean found = (out && (strstr(out, "bluealsa:playback_1") != NULL));
    if (out) g_free(out);
    return found;
}

/* Button handler: set selected Bluetooth device as current OUTPUT (routes playback) */
static void on_bt_set_output_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkWidget *btnw = GTK_WIDGET(b);
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(btnw), "device_tree"));
    if (!tv) return;

    /* Get selected object path */
    GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        GtkWindow *parent = get_parent_window_from_widget(btnw);
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "No device selected");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    gchar *obj = NULL;
    gtk_tree_model_get(model, &iter, 1, &obj, -1);
    if (!obj) return;

    /* Derive MAC */
    char *mac = mac_from_bluez_object(obj);
    g_free(obj);

    if (!mac) {
        GtkWindow *parent = get_parent_window_from_widget(btnw);
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "Failed to derive Bluetooth MAC from selection.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    GtkWindow *parent = get_parent_window_from_widget(btnw);
    
    /* Call routing helper SYNCHRONOUSLY so we can verify result */
    if (!file_exists_readable(ROUTE_HELPER)) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "Routing helper missing: %s\nRun: sudo ./contrib/install.sh", ROUTE_HELPER);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(mac);
        return;
    }
    
    gchar *cmd = g_strdup_printf("%s bluetooth %s", ROUTE_HELPER, mac);
    gchar *stdout_data = NULL;
    gchar *stderr_data = NULL;
    gint exit_status = 0;
    GError *err = NULL;
    
    gboolean spawn_ok = g_spawn_command_line_sync(cmd, &stdout_data, &stderr_data, &exit_status, &err);
    g_free(cmd);
    
    if (!spawn_ok || !WIFEXITED(exit_status) || WEXITSTATUS(exit_status) != 0) {
        /* Helper failed to run or exited with error */
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "Failed to set Bluetooth output.\n\nHelper error: %s\n\nCheck that:\n• Device is connected\n• BlueALSA daemon is running\n• You are in 'audio' and 'bluetooth' groups",
                                              err ? err->message : (stderr_data ? stderr_data : "unknown"));
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
        if (stdout_data) g_free(stdout_data);
        if (stderr_data) g_free(stderr_data);
        g_free(mac);
        return;
    }
    
    if (stdout_data) g_free(stdout_data);
    if (stderr_data) g_free(stderr_data);
    
    /* Give ports a moment to appear (helper spawns alsa_out in background) */
    g_usleep(2000000); /* 2 seconds */
    
    /* Verify bluealsa ports actually appeared */
    if (!bluealsa_ports_exist()) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                              "Bluetooth ports failed to spawn.\n\nPossible causes:\n• Device disconnected during setup\n• BlueALSA daemon not running\n• No active A2DP transport\n\nCheck /tmp/jack-route-select.log for details");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(mac);
        return;
    }
    
    /* Success! Update Devices panel and show confirmation */
    g_idle_add(sync_devices_panel_to_bluetooth_idle, NULL);
    
    GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                          "Bluetooth output set to %s\n\nPorts: bluealsa:playback_1/2\nAudio will play through Bluetooth device.", mac);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    g_free(mac);
}

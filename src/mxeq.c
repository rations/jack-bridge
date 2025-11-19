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

typedef struct {
    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem;
    GtkWidget *scale;
    const char *channel_name;
} MixerChannel;

typedef struct {
    snd_mixer_t *mixer;
    MixerChannel *channels;
    int num_channels;
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
   main window back to a compact size when both expanders are collapsed. */
static GtkWidget *g_main_window = NULL;
static GtkWidget *g_eq_expander = NULL;
static GtkWidget *g_bt_expander = NULL;

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
    if (!g_main_window || !g_eq_expander || !g_bt_expander) return;

    gboolean eq_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_eq_expander));
    gboolean bt_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_bt_expander));

    if (!eq_exp && !bt_exp) {
        /* both collapsed - shrink to compact height */
        gtk_window_resize(GTK_WINDOW(g_main_window), 900, 260);
    } else {
        /* one or both expanded - give more vertical space */
        gtk_window_resize(GTK_WINDOW(g_main_window), 900, 480);
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
    snd_mixer_selem_get_playback_volume_range(channel->elem, &min, &max);
    long alsa_value = (long)(value * (max - min) + min);
    snd_mixer_selem_set_playback_volume_all(channel->elem, alsa_value);
}

static void eq_slider_changed(GtkRange *range, EQBand *band) {
    gdouble value = gtk_range_get_value(range);
    long alsa_value = (long)(value * 100.0); // Map 0-1 to 0-100
    snd_ctl_elem_value_set_integer(band->val, 0, alsa_value); // Left channel
    snd_ctl_elem_value_set_integer(band->val, 1, alsa_value); // Right channel
    snd_ctl_elem_write(band->ctl, band->val);
}

static void init_alsa_mixer(MixerData *data) {
    if (snd_mixer_open(&data->mixer, 0) < 0) {
        fprintf(stderr, "Failed to open ALSA mixer\n");
        return;
    }
    if (snd_mixer_attach(data->mixer, "default") < 0) {
        fprintf(stderr, "Failed to attach mixer to default device\n");
        snd_mixer_close(data->mixer);
        return;
    }
    if (snd_mixer_selem_register(data->mixer, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to register mixer\n");
        snd_mixer_close(data->mixer);
        return;
    }
    if (snd_mixer_load(data->mixer) < 0) {
        fprintf(stderr, "Failed to load mixer\n");
        snd_mixer_close(data->mixer);
        return;
    }

    const char *channel_names[] = {"Master", "Speaker", "PCM", "Headphone", "Mic", "Mic Boost", "Beep"};
    int max_channels = G_N_ELEMENTS(channel_names);
    data->channels = g_new(MixerChannel, max_channels);
    data->num_channels = 0;

    for (int i = 0; i < max_channels; i++) {
        snd_mixer_selem_id_t *sid;
        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_name(sid, channel_names[i]);
        snd_mixer_elem_t *elem = snd_mixer_find_selem(data->mixer, sid);
        if (elem) {
            data->channels[data->num_channels].elem = elem;
            data->channels[data->num_channels].channel_name = channel_names[i];
            data->num_channels++;
        }
    }
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

/* Format default filename like AlsaTune-YYYYmmdd-HHMMSS.wav (newly allocated) */
static char *format_default_filename(void) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "AlsaTune-%Y%m%d-%H%M%S.wav", &tm);
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

    /* Build argv for arecord */
    gchar *channels_s = g_strdup_printf("%d", channels);
    gchar *rate_s = g_strdup_printf("%d", rate);

    gchar *argv[] = {
        "arecord",
        "-r", rate_s,
        "-c", channels_s,
        "-f", "S16_LE",
        full_path,
        NULL
    };

    GError *err = NULL;
    gboolean ok = g_spawn_async(
        NULL,
        argv,
        NULL,
        G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
        NULL, NULL,
        &record_pid,
        &err
    );

    g_free(channels_s);
    g_free(rate_s);

    if (!ok) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Failed to start arecord: %s", err ? err->message : "unknown");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (err) g_error_free(err);
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

/* ---- Bluetooth latency controls (period/number of periods) ---- */

typedef struct {
    GtkRange *period_slider;        /* frames (128..1024) */
    GtkSpinButton *nperiods_spin;   /* 2..4 */
} BtLatencyUI;

/* Read a simple KEY=VALUE from a file; returns newly allocated string or NULL */
static char *read_kv_value(const char *path, const char *key) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    size_t klen = strlen(key);
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        /* trim newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if ((size_t)(eq - line) == klen && strncmp(line, key, klen) == 0) {
            char *val = eq + 1;
            /* trim leading spaces */
            while (*val == ' ' || *val == '\t') val++;
            fclose(f);
            return g_strdup(val);
        }
    }
    fclose(f);
    return NULL;
}

/* Atomically upsert KEY=VALUE into file */
static int upsert_kv_atomic(const char *path, const char *key, const char *value) {
    if (!path || !key || !value) return -1;

    /* Ensure parent directory exists before writing */
    char *dir = g_path_get_dirname(path);
    if (dir && dir[0]) {
        if (g_mkdir_with_parents(dir, 0755) != 0) {
            g_free(dir);
            return -1;
        }
    }
    if (dir) g_free(dir);

    FILE *in = fopen(path, "r");
    GString *out = g_string_new(NULL);
    gboolean replaced = FALSE;
    size_t klen = strlen(key);

    if (in) {
        char line[1024];
        while (fgets(line, sizeof(line), in)) {
            char *orig = line;
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (line[0] != '#') {
                char *eq = strchr(line, '=');
                if (eq && (size_t)(eq - line) == klen && strncmp(line, key, klen) == 0) {
                    g_string_append_printf(out, "%s=%s\n", key, value);
                    replaced = TRUE;
                    continue;
                }
            }
            g_string_append_printf(out, "%s\n", orig);
        }
        fclose(in);
    }
    if (!replaced) {
        g_string_append_printf(out, "%s=%s\n", key, value);
    }

    /* Write to temp and rename atomically */
    char *tmp = g_strdup_printf("%s.tmp", path);
    FILE *outf = fopen(tmp, "w");
    if (!outf) {
        g_string_free(out, TRUE);
        g_free(tmp);
        return -1;
    }
    if (fputs(out->str, outf) == EOF) {
        fclose(outf);
        g_string_free(out, TRUE);
        g_unlink(tmp);
        g_free(tmp);
        return -1;
    }
    fclose(outf);

    if (g_rename(tmp, path) != 0) {
        g_unlink(tmp);
        g_string_free(out, TRUE);
        g_free(tmp);
        return -1;
    }

    g_string_free(out, TRUE);
    g_free(tmp);
    return 0;
}

static void hup_autobridge(void) {
    FILE *f = fopen("/var/run/jack-bluealsa-autobridge.pid", "r");
    if (!f) return;
    int pid = 0;
    if (fscanf(f, "%d", &pid) == 1 && pid > 1) {
        kill(pid, SIGHUP);
    }
    fclose(f);
}

static void on_latency_period_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    int period = (int)gtk_range_get_value(range);
    if (period < 128) period = 128;
    if (period > 1024) period = 1024;
    char val[32];
    snprintf(val, sizeof(val), "%d", period);
    /* Write both alias and canonical to keep compatibility */
    upsert_kv_atomic("/etc/jack-bridge/bluetooth.conf", "BRIDGE_PERIOD", val);
    upsert_kv_atomic("/etc/jack-bridge/bluetooth.conf", "A2DP_PERIOD", val);
    hup_autobridge();
}

static void on_latency_nperiods_changed(GtkSpinButton *spin, gpointer user_data) {
    (void)user_data;
    int n = gtk_spin_button_get_value_as_int(spin);
    if (n < 2) n = 2;
    if (n > 4) n = 4;
    char val[16];
    snprintf(val, sizeof(val), "%d", n);
    upsert_kv_atomic("/etc/jack-bridge/bluetooth.conf", "BRIDGE_NPERIODS", val);
    upsert_kv_atomic("/etc/jack-bridge/bluetooth.conf", "A2DP_NPERIODS", val);
    hup_autobridge();
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

    /* Default: disable all until we have state */
    if (pair_btn) gtk_widget_set_sensitive(pair_btn, FALSE);
    if (trust_btn) gtk_widget_set_sensitive(trust_btn, FALSE);
    if (connect_btn) gtk_widget_set_sensitive(connect_btn, FALSE);
    if (remove_btn) gtk_widget_set_sensitive(remove_btn, has);

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
    gtk_box_pack_start(GTK_BOX(bt_action_row), pair_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), trust_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), connect_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), remove_btn, FALSE, FALSE, 0);

    /* Latency controls (A2DP/JACK bridge) */
    GtkWidget *lat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(bt_vbox), lat_box, FALSE, FALSE, 0);

    GtkWidget *lat_label = gtk_label_new("Bluetooth latency (period frames):");
    gtk_box_pack_start(GTK_BOX(lat_box), lat_label, FALSE, FALSE, 0);

    GtkWidget *lat_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 128, 1024, 64);
    gtk_widget_set_size_request(lat_scale, 200, -1);
    gtk_box_pack_start(GTK_BOX(lat_box), lat_scale, TRUE, TRUE, 0);

    GtkWidget *n_label = gtk_label_new("nperiods:");
    gtk_box_pack_start(GTK_BOX(lat_box), n_label, FALSE, FALSE, 0);

    GtkWidget *n_spin = gtk_spin_button_new_with_range(2, 4, 1);
    gtk_box_pack_start(GTK_BOX(lat_box), n_spin, FALSE, FALSE, 0);

    /* Initialize latency controls from config or defaults */
    char *v_period = read_kv_value("/etc/jack-bridge/bluetooth.conf", "BRIDGE_PERIOD");
    if (!v_period) v_period = read_kv_value("/etc/jack-bridge/bluetooth.conf", "A2DP_PERIOD");
    int init_period = v_period ? atoi(v_period) : 1024;
    if (init_period < 128) init_period = 128;
    if (init_period > 1024) init_period = 1024;
    gtk_range_set_value(GTK_RANGE(lat_scale), init_period);
    if (v_period) g_free(v_period);

    char *v_n = read_kv_value("/etc/jack-bridge/bluetooth.conf", "BRIDGE_NPERIODS");
    if (!v_n) v_n = read_kv_value("/etc/jack-bridge/bluetooth.conf", "A2DP_NPERIODS");
    int init_n = v_n ? atoi(v_n) : 3;
    if (init_n < 2) init_n = 2;
    if (init_n > 4) init_n = 4;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(n_spin), init_n);
    if (v_n) g_free(v_n);

    g_signal_connect(lat_scale, "value-changed", G_CALLBACK(on_latency_period_changed), NULL);
    g_signal_connect(n_spin, "value-changed", G_CALLBACK(on_latency_nperiods_changed), NULL);

    /* Disable action buttons until a device is selected */
    gtk_widget_set_sensitive(pair_btn, FALSE);
    gtk_widget_set_sensitive(trust_btn, FALSE);
    gtk_widget_set_sensitive(connect_btn, FALSE);
    gtk_widget_set_sensitive(remove_btn, FALSE);

    /* Bind device store to GUI BT helpers, register listeners, and populate existing devices */
    gui_bt_set_device_store_widget(tree, store);
    gui_bt_register_discovery_listeners();
    gui_bt_populate_existing_devices();

    /* Provide access for callbacks to the selected row and selection-driven sensitivity */
    g_object_set_data(G_OBJECT(pair_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(trust_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(connect_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(remove_btn), "device_tree", tree);

    /* Expose buttons via the tree so selection-changed can toggle sensitivity */
    g_object_set_data(G_OBJECT(tree), "pair_btn", pair_btn);
    g_object_set_data(G_OBJECT(tree), "trust_btn", trust_btn);
    g_object_set_data(G_OBJECT(tree), "connect_btn", connect_btn);
    g_object_set_data(G_OBJECT(tree), "remove_btn", remove_btn);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    g_signal_connect(sel, "changed", G_CALLBACK(on_bt_selection_changed), NULL);

    /* Connect signals */
    g_signal_connect(scan_btn, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(pair_btn, "clicked", G_CALLBACK(on_pair_clicked), NULL);
    g_signal_connect(trust_btn, "clicked", G_CALLBACK(on_trust_clicked), NULL);
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_clicked), NULL);
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

    // Set the default window icon for all windows
    gtk_window_set_default_icon_name("audio-speakers");

    MixerData mixer_data = {0};
    EQData eq_data = {0};
    init_alsa_mixer(&mixer_data);
    init_alsa_eq(&eq_data);

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
    gtk_window_set_title(GTK_WINDOW(window), "AlsaTune");
    // Make window wider so mixer columns fit comfortably but keep height tight
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 260); // Wider and shorter default to avoid blank area underneath collapsed expanders
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

    // Mixer horizontal box
    GtkWidget *mixer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(mixer_box), 5);
    gtk_container_add(GTK_CONTAINER(mixer_frame), mixer_box);

    // Add sliders for each mixer channel
    for (int i = 0; i < mixer_data.num_channels; i++) {
        GtkWidget *channel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_box_pack_start(GTK_BOX(mixer_box), channel_box, TRUE, TRUE, 5);

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
        snd_mixer_selem_get_playback_volume_range(mixer_data.channels[i].elem, &min, &max);
        snd_mixer_selem_get_playback_volume(mixer_data.channels[i].elem, 0, &value);
        gtk_range_set_value(GTK_RANGE(mixer_data.channels[i].scale), (double)(value - min) / (max - min));

        GtkWidget *padding = gtk_label_new("");
        gtk_box_pack_start(GTK_BOX(channel_box), padding, FALSE, FALSE, 5);
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

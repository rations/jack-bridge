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

/* Forward declarations for panels defined after main() */
static void create_devices_panel(GtkWidget *main_box);
static void create_steam_panel(GtkWidget *main_box);

/* Forward declarations needed by earlier callers */
static int write_string_atomic(const char *path, const char *content);
static gboolean bluealsa_ports_exist(void);

/* How a mixer element should be presented. Derived from the element's own ALSA
 * capabilities, never from its name, so it is correct on any codec. */
typedef enum {
    MIXER_CTL_SLIDER,  /* has a volume            -> vertical slider */
    MIXER_CTL_SWITCH,  /* switch only, no volume  -> single checkbox (e.g. IEC958) */
    MIXER_CTL_ENUM     /* enumerated              -> dropdown (e.g. Input Source) */
} MixerCtlKind;

typedef struct {
    snd_mixer_elem_t *elem;
    GtkWidget *scale;
    const char *channel_name;
    gboolean is_capture;  /* TRUE if this is a capture control, FALSE for playback */
    MixerCtlKind kind;
} MixerChannel;

typedef struct {
    snd_mixer_t *mixer;
    MixerChannel *channels;
    int num_channels;
    GtkWidget *mixer_box;   /* GtkGrid: the volume sliders */
    GtkWidget *switch_box;  /* GtkFlowBox: toggles and dropdowns */
    GtkWidget *switch_sep;  /* divider, hidden when switch_box is empty */
    /* Use the separate switch row beneath the grid? Internal card only. USB
     * interfaces keep the original layout: every control in the grid, with a
     * capture control's enable box under its own slider. */
    gboolean use_switch_row;
} MixerData;

/* ---- Which controls the internal card shows ---------------------------------
 * Matched as case-insensitive SUBSTRINGS, not exact names. HDA codecs label the
 * same function differently between machines ("Master" / "Master Front",
 * "Speaker" / "Front", "Mic" / "Front Mic" / "Internal Mic", "IEC958" /
 * "S/PDIF"), so exact-name matching hides real controls on hardware other than
 * the developer's. If this list matches nothing on a given card, the filter is
 * abandoned and every control is shown — an unrecognised codec must never
 * produce an empty mixer panel.
 *
 * Only the internal card is curated. USB interfaces keep showing everything,
 * since their controls are the whole point of the device.
 */
static const char *const kInternalAllow[] = {
    "master", "headphone", "speaker",
    "front", "surround", "center", "lfe", "side",  /* multi-channel outputs */
    "pcm", "mic", "line",
    "capture", "iec958", "s/pdif",
    "input source",
    NULL
};

/* Hardware-only controls that just confuse users here, on any card. Auto-Mute
 * drives jack-sense speaker muting and Loopback Mixing drives analog loopback;
 * both remain available in alsamixer(1). */
static const char *const kAlwaysHide[] = {
    "auto-mute", "automute", "loopback", "beep",
    NULL
};

static gboolean name_matches_any(const char *name, const char *const *patterns) {
    gchar *lower = g_ascii_strdown(name, -1);
    if (!lower) return FALSE;
    gboolean hit = FALSE;
    for (int i = 0; patterns[i]; i++) {
        if (strstr(lower, patterns[i])) { hit = TRUE; break; }
    }
    g_free(lower);
    return hit;
}

/* UI globals used to keep window/expander references for compacting behavior.
    These are used by the expander 'notify::expanded' handler to shrink the
    main window back to a compact size when all expanders are collapsed. */
static GtkWidget *g_main_window = NULL;
static GtkWidget *g_mixer_expander = NULL;
static GtkWidget *g_rec_expander = NULL;
static GtkWidget *g_bt_expander = NULL;
static GtkWidget *g_dev_expander = NULL;
static GtkWidget *g_steam_expander = NULL;

/* Approximate heights for precise window resizing to avoid blank space */
#define EXPANDER_HEADER_HEIGHT 5
#define MIXER_CONTENT_HEIGHT 320
#define REC_CONTENT_HEIGHT 100
#define BT_CONTENT_HEIGHT 150
#define DEV_CONTENT_HEIGHT 50
#define STEAM_CONTENT_HEIGHT 65
#define WINDOW_BASE_HEIGHT 40  /* borders, spacing */
/* Expose Bluetooth device tree to Devices (Playback) panel for MAC selection */
static GtkWidget *g_bt_tree = NULL;
/* Devices panel radio buttons the Bluetooth panel needs to drive: it selects
   Bluetooth on success, and falls back to Internal when a BT route fails. */
static GtkWidget *g_rb_internal = NULL;
static GtkWidget *g_rb_bt = NULL;
/* Global mixer data for dynamic card switching */
static MixerData *g_mixer_data = NULL;
/* Forward declaration used by Devices (Playback) panel to derive MAC from BlueZ path */
static char *mac_from_bluez_object(const char *s);
/* Forward declaration for mixer rebuild */
static void rebuild_mixer_for_card(int card_num, gboolean curate);

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

    if (g_main_window && g_mixer_expander && g_rec_expander && g_bt_expander &&
        g_dev_expander && g_steam_expander) {
        gboolean mixer_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_mixer_expander));
        gboolean rec_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_rec_expander));
        gboolean bt_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_bt_expander));
        gboolean dev_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_dev_expander));
        gboolean steam_exp = gtk_expander_get_expanded(GTK_EXPANDER(g_steam_expander));

        /* Calculate exact height based on expanded state to eliminate blank space */
        int height = WINDOW_BASE_HEIGHT;
        height += mixer_exp ? MIXER_CONTENT_HEIGHT : EXPANDER_HEADER_HEIGHT;
        height += rec_exp ? REC_CONTENT_HEIGHT : EXPANDER_HEADER_HEIGHT;
        height += bt_exp ? BT_CONTENT_HEIGHT : EXPANDER_HEADER_HEIGHT;
        height += dev_exp ? DEV_CONTENT_HEIGHT : EXPANDER_HEADER_HEIGHT;
        height += steam_exp ? STEAM_CONTENT_HEIGHT : EXPANDER_HEADER_HEIGHT;

        gtk_window_resize(GTK_WINDOW(g_main_window), 600, height);
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
/* Pair/Trust/Connect are async: they report the outcome via the callback, which
   gui_bt always invokes on the GTK main loop, so the UI never blocks on D-Bus. */
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
/* Forward declaration: the shared Bluetooth routing helpers block this handler
   while they update the Devices panel, so they must be able to name it. */
static void on_device_radio_toggled(GtkToggleButton *tb, gpointer user_data);

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

/* Shared completion callback for the async pair/trust/connect operations:
 * report failures to the user, stay quiet on success. gui_bt always invokes
 * this on the GTK main loop. user_data is unused — the dialog is parented on
 * the main window rather than a captured pointer, which may have been
 * destroyed while the D-Bus call was in flight. */
static void bt_op_cb(gboolean success, const char *message, gpointer user_data) {
    (void)user_data;
    if (!success && message)
        show_bt_error_dialog(g_main_window ? GTK_WINDOW(g_main_window) : NULL, message);
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
    gui_bt_pair_device_async(obj, bt_op_cb, parent);
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
    gui_bt_trust_device_async(obj, TRUE, bt_op_cb, parent);
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
    gui_bt_connect_device_async(obj, bt_op_cb, parent);
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

/* Plain on/off handler for switch-only controls and for the capture enable
   boxes in the switch row: checked means ON, with no inversion. (The separate
   on_mute_toggled() below is inverted, because a "Mute" box checked means off.) */
static void on_switch_toggled(GtkToggleButton *btn, gpointer user_data) {
    MixerChannel *ch = (MixerChannel *)user_data;
    if (!ch || !ch->elem) return;
    int on = gtk_toggle_button_get_active(btn) ? 1 : 0;
    if (ch->is_capture)
        snd_mixer_selem_set_capture_switch_all(ch->elem, on);
    else
        snd_mixer_selem_set_playback_switch_all(ch->elem, on);
}

/* Enumerated controls (e.g. "Input Source" choosing which mic is live). These
   were previously rendered as sliders, where dragging did nothing useful. */
static void on_enum_changed(GtkComboBox *combo, gpointer user_data) {
    MixerChannel *ch = (MixerChannel *)user_data;
    if (!ch || !ch->elem) return;
    int active = gtk_combo_box_get_active(combo);
    if (active < 0) return;
    /* Apply to every channel the element exposes; unsupported ones just fail. */
    for (int c = 0; c <= SND_MIXER_SCHN_LAST; c++)
        snd_mixer_selem_set_enum_item(ch->elem, (snd_mixer_selem_channel_id_t)c,
                                      (unsigned int)active);
}

/* Decide how an element should be drawn, from its ALSA capabilities alone.
   Returns FALSE if the element offers nothing we can present. */
static gboolean classify_mixer_elem(snd_mixer_elem_t *elem,
                                    MixerCtlKind *kind, gboolean *is_capture) {
    if (snd_mixer_selem_is_enumerated(elem)) {
        *kind = MIXER_CTL_ENUM;
        /* capture-side enum (e.g. Input Source) vs playback-side */
        *is_capture = snd_mixer_selem_is_enum_capture(elem) ? TRUE : FALSE;
        return TRUE;
    }

    if (snd_mixer_selem_has_playback_volume(elem) ||
        snd_mixer_selem_has_capture_volume(elem)) {
        *kind = MIXER_CTL_SLIDER;
        *is_capture = (snd_mixer_selem_has_capture_volume(elem) &&
                       !snd_mixer_selem_has_playback_volume(elem));
        return TRUE;
    }

    if (snd_mixer_selem_has_playback_switch(elem) ||
        snd_mixer_selem_has_capture_switch(elem)) {
        *kind = MIXER_CTL_SWITCH;
        *is_capture = (snd_mixer_selem_has_capture_switch(elem) &&
                       !snd_mixer_selem_has_playback_switch(elem));
        return TRUE;
    }

    return FALSE; /* nothing presentable */
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

/* Release the ALSA mixer handle and the channel array, including the per-channel
 * names strdup'd in init_alsa_mixer(). Safe to call repeatedly and on a zeroed
 * struct. This is the single teardown path — previously it was open-coded in
 * three places and one copy forgot the names, leaking them on every card switch. */
static void mixer_data_reset(MixerData *data) {
    if (!data) return;
    if (data->mixer) {
        snd_mixer_close(data->mixer);
        data->mixer = NULL;
    }
    if (data->channels) {
        for (int i = 0; i < data->num_channels; i++)
            g_free((char *)data->channels[i].channel_name);
        g_free(data->channels);
        data->channels = NULL;
    }
    data->num_channels = 0;
}

/* Populate data->channels from the card's simple mixer elements.
 * `curate` restricts the set to kInternalAllow (used for the internal card);
 * when FALSE every presentable control is kept (used for USB interfaces).
 * Returns the number of controls collected. */
static int collect_mixer_channels(MixerData *data, snd_mixer_t *m,
                                  int card_num, gboolean curate) {
    int idx = 0;
    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(m); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE) continue;

        const char *name = snd_mixer_selem_get_name(elem);
        if (!name) continue;

        if (name_matches_any(name, kAlwaysHide)) continue;
        if (curate && !name_matches_any(name, kInternalAllow)) continue;

        MixerCtlKind kind;
        gboolean is_capture = FALSE;
        if (!classify_mixer_elem(elem, &kind, &is_capture)) continue;

        /* Auto-enable capture switches so a fresh install can record without
         * the user first hunting through alsamixer. */
        if (is_capture && snd_mixer_selem_has_capture_switch(elem)) {
            int sw = 0;
            snd_mixer_selem_get_capture_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
            if (!sw) {
                snd_mixer_selem_set_capture_switch_all(elem, 1);
                fprintf(stderr, "Auto-enabled capture for '%s' on card %d\n", name, card_num);
            }
        }

        /* ALSA can expose several elements sharing a name, distinguished only by
         * index — e.g. 'Headphone',0 and 'Headphone',1 for front and rear jacks,
         * or one 'Capture'/'Input Source' pair per capture stream. They are
         * genuinely separate controls, so keep them all, but label the extras so
         * the panel does not show two identical-looking widgets. */
        unsigned int eidx = snd_mixer_selem_get_index(elem);
        data->channels[idx].elem = elem;
        data->channels[idx].channel_name = (eidx > 0)
            ? g_strdup_printf("%s #%u", name, eidx)
            : g_strdup(name);
        data->channels[idx].is_capture = is_capture;
        data->channels[idx].kind = kind;
        idx++;
    }
    return idx;
}

/* Open `card_num` and collect its mixer controls.
 * curate == TRUE narrows the internal card to the controls users actually need
 * (see kInternalAllow); USB interfaces pass FALSE and keep everything. */
static void init_alsa_mixer(MixerData *data, int card_num, gboolean curate) {
    /* Release any previous card's mixer and channel names before rebuilding */
    mixer_data_reset(data);

    /* The two-zone layout is part of the internal-card redesign; USB keeps the
       single-grid layout it already had. */
    data->use_switch_row = curate;

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

    /* Count simple mixer elements (upper bound for the array) */
    int count = 0;
    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(m); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE)
            count++;
    }

    if (count == 0) {
        fprintf(stderr, "init_alsa_mixer: no simple mixer elements found on card %d\n", card_num);
        snd_mixer_close(m);
        return;
    }

    data->channels = g_new0(MixerChannel, count);
    data->mixer = m;

    int idx = collect_mixer_channels(data, m, card_num, curate);

    /* Safety net for unknown codecs: if curation matched nothing, the name list
     * simply does not fit this hardware. Showing an empty mixer would be worse
     * than showing too much, so fall back to the uncurated set. */
    if (curate && idx == 0) {
        fprintf(stderr, "init_alsa_mixer: no curated controls matched on card %d; "
                        "showing all controls instead\n", card_num);
        idx = collect_mixer_channels(data, m, card_num, FALSE);
    }

    data->num_channels = idx;
    fprintf(stderr, "init_alsa_mixer: found %d mixer controls on card %d (curated=%s)\n",
            idx, card_num, curate ? "yes" : "no");
}

static void cleanup_alsa(MixerData *mixer_data) {
    mixer_data_reset(mixer_data);
}

/* ---- Mixer UI construction -------------------------------------------------
 * These three helpers are shared by the initial build in main() and by
 * rebuild_mixer_for_card(), which previously carried a verbatim copy of the
 * ~55-line per-channel loop. Any new control type only has to be handled once.
 */

/* Remove every widget from both mixer containers. */
static void mixer_clear_box(MixerData *data) {
    if (!data) return;
    GtkWidget *containers[2];
    containers[0] = data->mixer_box;
    containers[1] = data->switch_box;
    for (int c = 0; c < 2; c++) {
        if (!containers[c]) continue;
        GList *children = gtk_container_get_children(GTK_CONTAINER(containers[c]));
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        g_list_free(children);
    }
}

/* Explain an empty mixer. card_num < 0 means "the default device". */
static void mixer_build_no_controls_message(MixerData *data, int card_num) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(data->mixer_box), box, 0, 0, 8, 1);

    gchar *msg;
    if (card_num >= 0)
        msg = g_strdup_printf(
            "No mixer controls detected on card %d.\n"
            "Audio may still work, but mixer sliders are unavailable.\n\n"
            "Try checking:\n"
            "• Card is properly detected: cat /proc/asound/cards\n"
            "• Mixer elements exist: alsamixer -c %d",
            card_num, card_num);
    else
        msg = g_strdup(
            "No mixer controls were detected on the default ALSA device.\n"
            "Audio may still play, but mixer sliders are unavailable.\n\n"
            "Possible fixes:\n"
            "• Ensure the default ALSA device has mixer elements (try 'alsamixer').\n"
            "• Check that ALSA's default device maps to your hardware (see /proc/asound/cards).\n"
            "• If using BlueALSA-only profiles, there may be no system mixer to control.");

    GtkWidget *label = gtk_label_new(msg);
    g_free(msg);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 8);
}

/* One vertical slider column for a control that exposes a volume. */
static void mixer_add_slider(MixerData *data, MixerChannel *ch, int slot) {
    GtkWidget *channel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_grid_attach(GTK_GRID(data->mixer_box), channel_box, slot % 8, slot / 8, 1, 1);

    GtkWidget *label = gtk_label_new(ch->channel_name);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(channel_box), label, FALSE, FALSE, 5);

    ch->scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 1, 0.01);
    gtk_range_set_inverted(GTK_RANGE(ch->scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(ch->scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(ch->scale), GTK_POS_BOTTOM);
    gtk_widget_set_size_request(ch->scale, -1, 150);
    gtk_box_pack_start(GTK_BOX(channel_box), ch->scale, TRUE, TRUE, 0);
    g_signal_connect(ch->scale, "value-changed", G_CALLBACK(slider_changed), ch);

    long min = 0, max = 0, value = 0;
    if (ch->is_capture) {
        snd_mixer_selem_get_capture_volume_range(ch->elem, &min, &max);
        snd_mixer_selem_get_capture_volume(ch->elem, 0, &value);
    } else {
        snd_mixer_selem_get_playback_volume_range(ch->elem, &min, &max);
        snd_mixer_selem_get_playback_volume(ch->elem, 0, &value);
    }
    if (max > min)
        gtk_range_set_value(GTK_RANGE(ch->scale), (double)(value - min) / (max - min));

    /* Playback controls always keep their Mute box under the slider. */
    if (!ch->is_capture && snd_mixer_selem_has_playback_switch(ch->elem)) {
        int sw = 1;
        snd_mixer_selem_get_playback_switch(ch->elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
        GtkWidget *mute = gtk_check_button_new_with_label("Mute");
        gtk_widget_set_halign(mute, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(mute, 4);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute), sw ? FALSE : TRUE);
        gtk_box_pack_start(GTK_BOX(channel_box), mute, FALSE, FALSE, 2);
        g_signal_connect(mute, "toggled", G_CALLBACK(on_mute_toggled), ch);
    }

    /* Capture enable: in the switch row on the internal card, or under its own
       slider on USB, which keeps the layout users already had there. */
    if (ch->is_capture && !data->use_switch_row &&
        snd_mixer_selem_has_capture_switch(ch->elem)) {
        int sw = 0;
        snd_mixer_selem_get_capture_switch(ch->elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
        GtkWidget *enable = gtk_check_button_new_with_label("Enable");
        gtk_widget_set_halign(enable, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(enable, 4);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable), sw ? TRUE : FALSE);
        gtk_box_pack_start(GTK_BOX(channel_box), enable, FALSE, FALSE, 2);
        g_signal_connect(enable, "toggled", G_CALLBACK(on_switch_toggled), ch);
    }
}

/* A labelled on/off box for a switch-only control. */
static GtkWidget *mixer_make_switch(MixerChannel *ch) {
    int sw = 1;
    if (ch->is_capture)
        snd_mixer_selem_get_capture_switch(ch->elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
    else
        snd_mixer_selem_get_playback_switch(ch->elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);

    GtkWidget *chk = gtk_check_button_new_with_label(ch->channel_name);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), sw ? TRUE : FALSE);
    g_signal_connect(chk, "toggled", G_CALLBACK(on_switch_toggled), ch);
    return chk;
}

/* A labelled dropdown for an enumerated control. */
static GtkWidget *mixer_make_enum(MixerChannel *ch) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *label = gtk_label_new(ch->channel_name);
    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);

    GtkWidget *combo = gtk_combo_box_text_new();
    int n = snd_mixer_selem_get_enum_items(ch->elem);
    for (int i = 0; i < n; i++) {
        char item[64];
        if (snd_mixer_selem_get_enum_item_name(ch->elem, (unsigned int)i,
                                               sizeof(item), item) == 0)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), item);
    }
    unsigned int active = 0;
    if (snd_mixer_selem_get_enum_item(ch->elem, SND_MIXER_SCHN_FRONT_LEFT, &active) == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), (int)active);
    g_signal_connect(combo, "changed", G_CALLBACK(on_enum_changed), ch);

    gtk_box_pack_start(GTK_BOX(row), combo, FALSE, FALSE, 0);
    return row;
}

/* Place a non-slider widget: in the switch row on the internal card, or in the
 * next grid cell on USB, which has no switch row. */
static void mixer_place_aux(MixerData *data, GtkWidget *w, int *slot, int *switches) {
    if (data->use_switch_row) {
        gtk_container_add(GTK_CONTAINER(data->switch_box), w);
        (*switches)++;
        return;
    }
    GtkWidget *cell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_valign(cell, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(cell), w, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(data->mixer_box), cell, *slot % 8, *slot / 8, 1, 1);
    (*slot)++;
}

/* Build the whole mixer: sliders in the grid, toggles and dropdowns in the row
 * beneath it. The divider and switch row stay hidden when nothing lands there. */
static void mixer_build_channels(MixerData *data) {
    int slot = 0;
    int switches = 0;

    for (int i = 0; i < data->num_channels; i++) {
        MixerChannel *ch = &data->channels[i];

        switch (ch->kind) {
        case MIXER_CTL_SLIDER:
            mixer_add_slider(data, ch, slot++);
            /* On the internal card a capture control's enable box goes to the
               switch row, alongside IEC958 and friends; on USB mixer_add_slider
               has already placed it under the slider. */
            if (data->use_switch_row && ch->is_capture &&
                snd_mixer_selem_has_capture_switch(ch->elem)) {
                gtk_container_add(GTK_CONTAINER(data->switch_box), mixer_make_switch(ch));
                switches++;
            }
            break;
        case MIXER_CTL_SWITCH:
            mixer_place_aux(data, mixer_make_switch(ch), &slot, &switches);
            break;
        case MIXER_CTL_ENUM:
            mixer_place_aux(data, mixer_make_enum(ch), &slot, &switches);
            break;
        }
    }

    (void)switches;
}

/* Show the divider and switch row only when something actually landed in them.
 * Must be re-applied after any gtk_widget_show_all() that covers these widgets
 * — including the toplevel one in main() — since show_all would otherwise
 * reveal an empty row and a stray divider. */
static void mixer_sync_switch_row(MixerData *data) {
    if (!data || !data->switch_box || !data->switch_sep) return;
    GList *kids = gtk_container_get_children(GTK_CONTAINER(data->switch_box));
    gboolean any = (kids != NULL);
    g_list_free(kids);
    gtk_widget_set_visible(data->switch_sep, any);
    gtk_widget_set_visible(data->switch_box, any);
}

/* Dynamic mixer rebuild: clear and repopulate mixer_box with controls from specified card */
static void rebuild_mixer_for_card(int card_num, gboolean curate) {
    if (!g_mixer_data || !g_mixer_data->mixer_box) return;
    
    fprintf(stderr, "rebuild_mixer_for_card: switching to card %d\n", card_num);

    mixer_clear_box(g_mixer_data);
    init_alsa_mixer(g_mixer_data, card_num, curate);

    if (g_mixer_data->num_channels == 0) {
        mixer_build_no_controls_message(g_mixer_data, card_num);
    } else {
        mixer_build_channels(g_mixer_data);
    }

    gtk_widget_show_all(g_mixer_data->mixer_box);
    if (g_mixer_data->switch_box) gtk_widget_show_all(g_mixer_data->switch_box);
    mixer_sync_switch_row(g_mixer_data);

    fprintf(stderr, "rebuild_mixer_for_card: rebuilt UI with %d controls for card %d\n",
            g_mixer_data->num_channels, card_num);
}
 
static void show_mixer_placeholder(const char *msg) {
    if (!g_mixer_data || !g_mixer_data->mixer_box) return;

    /* Destroy the widgets before releasing the channel array: each slider and
       checkbox carries a &channels[i] as its signal user_data, so the array must
       outlive them. The previous order freed the array first. */
    mixer_clear_box(g_mixer_data);
    mixer_data_reset(g_mixer_data);

    GtkWidget *placeholder_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_hexpand(placeholder_box, TRUE);
    gtk_widget_set_halign(placeholder_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(placeholder_box, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(g_mixer_data->mixer_box), placeholder_box, 0, 0, 8, 1);

    GtkWidget *label = gtk_label_new(msg);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(placeholder_box), label, TRUE, TRUE, 24);
    gtk_widget_show_all(g_mixer_data->mixer_box);
    /* HDMI/Bluetooth have no mixer at all, so the switch row is empty too */
    mixer_sync_switch_row(g_mixer_data);
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
    return g_path_get_basename(name); /* already newly allocated */
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

/* Return the recorder controls to their idle state. */
static void reset_recorder_ui(void) {
    if (!rec_ui) return;
    gtk_widget_set_sensitive(rec_ui->record_btn, TRUE);
    gtk_widget_set_sensitive(rec_ui->stop_btn, FALSE);
    gtk_label_set_text(GTK_LABEL(rec_ui->status_label), "Idle");
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

    /* g_child_watch callbacks already run on the main loop, so update directly. */
    reset_recorder_ui();
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

    /* Record via the ALSA 'jack' PCM, defined in 50-jack.conf (installed to
     * /etc/alsa/conf.d/ or /usr/share/alsa/alsa.conf.d/). Its capture_ports point
     * at system:capture_1/2, which jackd creates from its -C device, so arecord
     * captures mic/line-in through JACK. 'plughw:N' is unavailable (jackd holds
     * the hardware exclusively) and 'default' is playback-only here. */
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
    GtkWidget *set_output_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tv), "set_output_btn"));

    /* Default: disable all until we have state */
    if (pair_btn) gtk_widget_set_sensitive(pair_btn, FALSE);
    if (trust_btn) gtk_widget_set_sensitive(trust_btn, FALSE);
    if (connect_btn) gtk_widget_set_sensitive(connect_btn, FALSE);
    if (remove_btn) gtk_widget_set_sensitive(remove_btn, has);
    if (set_output_btn) gtk_widget_set_sensitive(set_output_btn, has);

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
    GtkWidget *set_output_btn = gtk_button_new_with_label("Set as Output");
    gtk_box_pack_start(GTK_BOX(bt_action_row), pair_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), trust_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), connect_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), remove_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bt_action_row), set_output_btn, FALSE, FALSE, 0);

    /* Disable action buttons until a device is selected */
    gtk_widget_set_sensitive(pair_btn, FALSE);
    gtk_widget_set_sensitive(trust_btn, FALSE);
    gtk_widget_set_sensitive(connect_btn, FALSE);
    gtk_widget_set_sensitive(remove_btn, FALSE);
    gtk_widget_set_sensitive(set_output_btn, FALSE);

    /* Bind device store to GUI BT helpers, register listeners, and populate existing devices */
    gui_bt_set_device_store_widget(tree, store);
    gui_bt_register_discovery_listeners();
    gui_bt_populate_existing_devices();

    /* Provide access for callbacks to the selected row and selection-driven sensitivity */
    g_object_set_data(G_OBJECT(pair_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(trust_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(connect_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(remove_btn), "device_tree", tree);
    g_object_set_data(G_OBJECT(set_output_btn), "device_tree", tree);

    /* Expose buttons via the tree so selection-changed can toggle sensitivity */
    g_object_set_data(G_OBJECT(tree), "pair_btn", pair_btn);
    g_object_set_data(G_OBJECT(tree), "trust_btn", trust_btn);
    g_object_set_data(G_OBJECT(tree), "connect_btn", connect_btn);
    g_object_set_data(G_OBJECT(tree), "remove_btn", remove_btn);
    g_object_set_data(G_OBJECT(tree), "set_output_btn", set_output_btn);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    g_signal_connect(sel, "changed", G_CALLBACK(on_bt_selection_changed), NULL);

    /* Connect signals */
    g_signal_connect(scan_btn, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(pair_btn, "clicked", G_CALLBACK(on_pair_clicked), NULL);
    g_signal_connect(trust_btn, "clicked", G_CALLBACK(on_trust_clicked), NULL);
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_clicked), NULL);
    g_signal_connect(set_output_btn, "clicked", G_CALLBACK(on_bt_set_output_clicked), NULL);
}


/* Detect internal card number: the first card whose `aplay -l` section does not
 * mention USB. Returns 0 if none found.
 *
 * jack-route-select carries an awk implementation of exactly this rule (its own
 * get_internal_card_number). The two must agree — if they drift, the GUI shows
 * one card's mixer while audio routes to another. They were differential-tested
 * against each other over gaps in card numbering, two-digit card numbers, a
 * matching card appearing last, and USB named only on a subdevice line.
 * Change both together. */
static int get_internal_card_number(void) {
    FILE *fp = popen("aplay -l 2>/dev/null", "r");
    if (!fp) return 0;

    char line[256];
    /* Must be initialised: the "is this card USB?" test below reads card_num on
     * every line, including any output preceding the first "card N:" header. */
    int card_num = -1;
    int found_non_usb = -1;

    while (fgets(line, sizeof(line), fp)) {
        // Check for card header
        if (sscanf(line, "card %d:", &card_num) == 1) {
            // If we found a non-USB card from previous iteration, return it
            if (found_non_usb != -1) {
                pclose(fp);
                return found_non_usb;
            }
            // Assume this card is non-USB until we find "usb" in its section
            found_non_usb = card_num;
        }

        // Check if current card section contains "usb" anywhere (case-insensitive)
        if (found_non_usb == card_num) {
            char *lower_line = g_ascii_strdown(line, -1);
            if (lower_line && strstr(lower_line, "usb")) {
                // This card is USB, mark as invalid
                found_non_usb = -1;
            }
            g_free(lower_line);
        }
    }

    pclose(fp);

    // Return the last valid non-USB card found, or 0 as fallback
    return (found_non_usb != -1) ? found_non_usb : 0;
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
    int internal_card = get_internal_card_number();
    fprintf(stderr, "mxeq: Detected internal card %d for mixer initialization\n", internal_card);
    init_alsa_mixer(&mixer_data, internal_card, TRUE);  /* internal card: curated control set */
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
    // Natural sizing - let GTK determine height based on content, set reasonable minimum
    gtk_window_set_default_size(GTK_WINDOW(window), 600, -1);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Expose main window to expander handler so it can resize back to compact height */
    g_main_window = window;

    // Main vertical box (no global scroller) - keep height tight when expanders are collapsed.
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4); // Tighter spacing
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Mixer expander (expanded by default to show mixer controls)
    GtkWidget *mixer_expander = gtk_expander_new("Mixer Controls");
    gtk_expander_set_expanded(GTK_EXPANDER(mixer_expander), TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), mixer_expander, FALSE, FALSE, 0);

    /* keep a reference so the expander toggle handler can resize the window */
    g_mixer_expander = mixer_expander;
    g_signal_connect(G_OBJECT(mixer_expander), "notify::expanded", G_CALLBACK(on_any_expander_toggled), NULL);

    // Mixer frame with border
    GtkWidget *mixer_frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(mixer_expander), mixer_frame);

    // Scrolled window for mixer (vertical scrolling only, max ~300px height)
    GtkWidget *mixer_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mixer_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(mixer_scroller, -1, 300);
    gtk_container_add(GTK_CONTAINER(mixer_frame), mixer_scroller);

    /* Two zones inside the scroller: a grid of volume sliders, then a divider
       and a wrapping row of toggles and dropdowns (Capture, IEC958, Input
       Source). Switch-only and enumerated controls do not belong on a slider. */
    GtkWidget *mixer_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(mixer_vbox), 5);
    gtk_container_add(GTK_CONTAINER(mixer_scroller), mixer_vbox);

    // Mixer grid (homogeneous columns, max 8 per row, auto-wrap to new rows)
    GtkWidget *mixer_box = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(mixer_box), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(mixer_box), FALSE);
    gtk_grid_set_column_spacing(GTK_GRID(mixer_box), 2);
    gtk_grid_set_row_spacing(GTK_GRID(mixer_box), 5);
    gtk_box_pack_start(GTK_BOX(mixer_vbox), mixer_box, TRUE, TRUE, 0);

    GtkWidget *switch_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(mixer_vbox), switch_sep, FALSE, FALSE, 0);

    GtkWidget *switch_box = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(switch_box), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(switch_box), 6);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(switch_box), 12);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(switch_box), 4);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(switch_box), FALSE);
    gtk_box_pack_start(GTK_BOX(mixer_vbox), switch_box, FALSE, FALSE, 0);

    /* Store container references for dynamic rebuild */
    mixer_data.mixer_box  = mixer_box;
    mixer_data.switch_sep = switch_sep;
    mixer_data.switch_box = switch_box;

    /* Populate the mixer grid (shared with rebuild_mixer_for_card) */
    if (mixer_data.num_channels == 0)
        mixer_build_no_controls_message(&mixer_data, -1);
    else
        mixer_build_channels(&mixer_data);

    // Recording expander (collapsed by default to minimize vertical footprint)
    GtkWidget *rec_expander = gtk_expander_new("Recording");
    gtk_expander_set_expanded(GTK_EXPANDER(rec_expander), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), rec_expander, FALSE, FALSE, 0);

    /* keep a reference so the expander toggle handler can resize the window */
    g_rec_expander = rec_expander;
    g_signal_connect(G_OBJECT(rec_expander), "notify::expanded", G_CALLBACK(on_any_expander_toggled), NULL);

    /* Recording content box: pack directly into the expander so its height
       naturally follows the Recorder UI without leaving extra blank space
       when expanded or collapsed (similar behaviour to the Devices expander). */
    GtkWidget *rec_content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(rec_expander), rec_content_vbox);

    // Recorder UI inside Recording expander
    create_recorder_ui(rec_content_vbox);

    /* Bluetooth panel (collapsible) */
    create_bt_panel(main_box);
 
    /* Devices panel (Playback) */
    create_devices_panel(main_box);

    /* Steam Gaming Mode panel */
    create_steam_panel(main_box);

    gtk_widget_show_all(window);
    /* show_all reveals the switch row unconditionally; hide it again if this
       card contributed no toggles or dropdowns. */
    mixer_sync_switch_row(&mixer_data);

    gtk_main();

    /* Unregister GUI discovery listeners and shutdown GUI Bluetooth helpers before exiting.
       These calls are safe no-ops if gui_bt wasn't initialized. */
    extern void gui_bt_unregister_discovery_listeners(void);
    extern void gui_bt_shutdown(void);
    gui_bt_unregister_discovery_listeners();
    gui_bt_shutdown();

    cleanup_alsa(&mixer_data);
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


/* Detect USB card number (first USB card, returns -1 if none found) */
static int get_usb_card_number(void) {
    // Use system call to get aplay output
    FILE *fp = popen("aplay -l 2>/dev/null", "r");
    if (!fp) return -1;

    char line[256];
    int card_num;
    while (fgets(line, sizeof(line), fp)) {
        // Check for card header
        if (sscanf(line, "card %d:", &card_num) == 1) {
            // Check if this card section contains "usb" anywhere (case-insensitive)
            char *lower_line = g_ascii_strdown(line, -1);
            if (lower_line && strstr(lower_line, "usb")) {
                g_free(lower_line);
                pclose(fp);
                return card_num; // Found USB card
            }
            g_free(lower_line);
        }
    }

    pclose(fp);
    return -1; // No USB card found
}

static gboolean is_usb_present(void) {
    return (get_usb_card_number() >= 0);
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
 * Mirrors the format of /etc/asound.conf.d/current_input.conf. */
static char *compose_user_current_input_conf(const char *pcm_current) {
    GString *s = g_string_new("");
    g_string_append_printf(s,
        "pcm.current_input {\n"
        "    type plug\n"
        "    slave.pcm \"%s\"\n"
        "}\n", pcm_current ? pcm_current : "input_card0");
    return g_string_free(s, FALSE);
}

/* Write ~/.config/jack-bridge/current_input.conf atomically */
static int write_user_current_input_conf(const char *pcm_current) {
    char *dir = user_config_dir();
    if (g_mkdir_with_parents(dir, 0755) != 0) {
        g_free(dir);
        return -1;
    }
    char *path = user_current_input_conf_path();
    char *content = compose_user_current_input_conf(pcm_current);
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

/* Compose the managed ~/.asoundrc block: includes for both the per-user
 * current_input and current_output fragments, so recording and playback
 * overrides both take effect for non-JACK ALSA apps. The fragment contents
 * themselves are written elsewhere, so this block needs no parameters. */
static char *compose_managed_block(void) {
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
static int write_user_asoundrc_block(const char *pcm_current) {
    /* First write the per-user fragment that mirrors /etc/asound.conf.d/current_input.conf */
    if (write_user_current_input_conf(pcm_current) != 0) {
        return -1;
    }

    /* Then ensure ~/.asoundrc has our include block */
    char *path = user_asoundrc_path();
    char *orig = load_file_to_string(path);
    char *prefix = strip_managed_block(orig ? orig : "");
    char *block = compose_managed_block();
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
    write_user_asoundrc_block(initial);
    if (sys) g_free(sys);
}





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

static gchar *load_bluetooth_device_mac(void) {
    const char *home = g_get_home_dir();
    if (!home) return NULL;
    gchar *path = g_build_filename(home, ".config", "jack-bridge", "devices.conf", NULL);
    if (!file_exists_readable(path)) { g_free(path); return NULL; }
    FILE *f = fopen(path, "r");
    g_free(path);
    if (!f) return NULL;
    char line[512];
    gchar *mac = NULL;
    while (fgets(line, sizeof(line), f)) {
        if (!g_str_has_prefix(line, "BLUETOOTH_DEVICE=")) continue;
        char *val = line + strlen("BLUETOOTH_DEVICE=");
        while (*val == ' ' || *val == '\t' || *val == '"') val++;
        char *dev = strstr(val, "DEV=");
        if (!dev) break;
        char *mac_start = dev + 4;
        if (strlen(mac_start) >= 17) mac = g_strndup(mac_start, 17);
        break;
    }
    fclose(f);
    return mac;
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

/* ---- Shared Bluetooth "set as output" flow ---------------------------------
 * Drives both the Devices panel's Bluetooth radio and the Bluetooth panel's
 * "Set as Output" button, which previously carried near-identical copies of
 * this logic that had already drifted apart (1s vs 2s waits, different wording).
 *
 * Everything runs off the GTK main loop rather than on it. The old code spawned
 * the routing helper synchronously and then slept another 1-2s; since the helper
 * itself waits up to 5s for its ports to appear, that froze the whole window for
 * several seconds on every Bluetooth switch. Here the helper is spawned async and
 * the ports are polled on a timeout, so the UI stays responsive throughout.
 */
typedef struct {
    gchar *mac;
    guint  poll_id;
    int    polls_left;
} BtRouteCtx;

static gboolean bt_route_active = FALSE; /* serialise: one switch at a time */

static void bt_route_ctx_free(BtRouteCtx *c) {
    if (!c) return;
    if (c->poll_id) g_source_remove(c->poll_id);
    g_free(c->mac);
    g_free(c);
    bt_route_active = FALSE;
}

/* Put the Devices panel back on Internal after a failed Bluetooth switch.
 * Selecting Internal deliberately fires its handler, which routes audio back to
 * the internal card — leaving the radio on Bluetooth with no bluealsa ports
 * would show a device that is not actually carrying audio. */
static void bt_route_revert_to_internal(void) {
    if (g_rb_bt) {
        g_signal_handlers_block_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_bt), FALSE);
        g_signal_handlers_unblock_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
    }
    if (g_rb_internal)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_internal), TRUE);
}

static void bt_route_fail(BtRouteCtx *c, const char *msg) {
    show_bt_error_dialog(g_main_window ? GTK_WINDOW(g_main_window) : NULL, msg);
    bt_route_revert_to_internal();
    bt_route_ctx_free(c);
}

/* Poll for bluealsa:playback_* after the helper reports success. */
static gboolean bt_route_poll_ports(gpointer data) {
    BtRouteCtx *c = data;

    if (bluealsa_ports_exist()) {
        c->poll_id = 0;
        /* Reflect the result in the Devices panel without re-entering routing */
        if (g_rb_bt) {
            g_signal_handlers_block_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_rb_bt), TRUE);
            g_signal_handlers_unblock_by_func(g_rb_bt, G_CALLBACK(on_device_radio_toggled), NULL);
        }
        GtkWidget *d = gtk_message_dialog_new(
            g_main_window ? GTK_WINDOW(g_main_window) : NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Bluetooth output ready.\n\nDevice: %s\nPorts: bluealsa:playback_1/2",
            c->mac ? c->mac : "(unknown)");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        bt_route_ctx_free(c);
        return G_SOURCE_REMOVE;
    }

    if (--c->polls_left <= 0) {
        c->poll_id = 0;
        bt_route_fail(c,
            "Bluetooth ports failed to spawn.\n\nPossible causes:\n"
            "• Device disconnected\n• BlueALSA daemon not running\n"
            "• No active A2DP transport\n\nCheck /tmp/jack-route-select.log");
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void bt_route_child_exit(GPid pid, gint status, gpointer data) {
    BtRouteCtx *c = data;
    g_spawn_close_pid(pid);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        bt_route_fail(c,
            "Failed to set Bluetooth output.\n\nThe routing helper reported an error.\n"
            "Check that the device is connected and the BlueALSA daemon is running.");
        return;
    }

    /* Helper exited cleanly; confirm the ports really came up (its own exit
     * status does not distinguish a failed alsa_out spawn). Up to 5s. */
    c->polls_left = 20;
    c->poll_id = g_timeout_add(250, bt_route_poll_ports, c);
}

/* Begin switching playback to `mac`. Returns FALSE if it could not be started. */
static gboolean bt_set_output_begin(const char *mac) {
    if (bt_route_active) return TRUE; /* a switch is already in flight */
    if (!mac || !*mac) return FALSE;

    GtkWindow *parent = g_main_window ? GTK_WINDOW(g_main_window) : NULL;
    if (!file_exists_readable(ROUTE_HELPER)) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK, "Routing helper missing: %s\nRun: sudo ./contrib/install.sh",
            ROUTE_HELPER);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return FALSE;
    }

    /* argv form rather than a command line: no shell quoting to get wrong */
    gchar *argv[] = { (gchar *)ROUTE_HELPER, (gchar *)"bluetooth", (gchar *)mac, NULL };
    GPid pid = 0;
    GError *err = NULL;
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL, &pid, &err)) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK, "Failed to run the routing helper:\n%s",
            err ? err->message : "unknown error");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
        return FALSE;
    }

    BtRouteCtx *c = g_new0(BtRouteCtx, 1);
    c->mac = g_strdup(mac);
    bt_route_active = TRUE;
    g_child_watch_add(pid, bt_route_child_exit, c);
    return TRUE;
}

/* Resolve the MAC to route to: the Bluetooth panel's selection if there is one,
 * otherwise the device jack-route-select saved on the last successful session.
 * Caller frees. */
static gchar *bt_target_mac(void) {
    gchar *mac = NULL;
    if (g_bt_tree && GTK_IS_TREE_VIEW(g_bt_tree)) {
        gchar *obj = tree_get_selected_obj(GTK_TREE_VIEW(g_bt_tree));
        if (obj) {
            mac = mac_from_bluez_object(obj);
            g_free(obj);
        }
    }
    if (!mac) mac = load_bluetooth_device_mac();
    return mac;
}

static void on_device_radio_toggled(GtkToggleButton *tb, gpointer user_data) {
    (void)user_data;
    if (!gtk_toggle_button_get_active(tb)) return;

    /* Dispatch on the target attached to the widget, not on its visible label:
       renaming a button in the UI (or translating it) must not silently break
       routing. create_devices_panel() sets this via g_object_set_data(). */
    const char *label = g_object_get_data(G_OBJECT(tb), "route_target");
    if (!label) return;

    GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(tb));
    gboolean ok = TRUE;
    gchar *mac = NULL;

    if (g_strcmp0(label, "internal") == 0) {
        ok = route_to_target_async("internal");
        /* Switch mixer to show detected internal card controls */
        int internal_card = get_internal_card_number();
        rebuild_mixer_for_card(internal_card, TRUE);
    } else if (g_strcmp0(label, "usb") == 0) {
        ok = route_to_target_async("usb");
        /* Switch mixer to show USB card controls */
        int usb_card = get_usb_card_number();
        if (usb_card >= 0) {
            rebuild_mixer_for_card(usb_card, FALSE);
        }
    } else if (g_strcmp0(label, "hdmi") == 0) {
        ok = route_to_target_async("hdmi");
        show_mixer_placeholder("Mixer controls are not available for HDMI output.\nUse your display or receiver to adjust the volume.");
    } else if (g_strcmp0(label, "bluetooth") == 0) {
        show_mixer_placeholder("Mixer controls are not available for Bluetooth output.\nUse your Bluetooth device to adjust the volume.");

        /* If the BT bridge is already up, nothing to do — re-routing here would
         * also raise a spurious "no device selected" dialog when the tree has no
         * selection yet (e.g. GUI opened while alsa_out was already running). */
        if (bluealsa_ports_exist()) return;

        mac = bt_target_mac();
        if (!mac) {
            GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "No Bluetooth device selected.\n\nPlease:\n1. Expand 'BLUETOOTH' panel\n2. Connect a device\n3. Try again");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            bt_route_revert_to_internal();
            return;
        }

        if (!bt_set_output_begin(mac))
            bt_route_revert_to_internal();
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

/* ======================================================================
 * Steam Gaming Mode panel — starts/stops pulse-jack-bridge
 * ====================================================================== */

static GPid      bridge_pid          = 0;
static GtkWidget *bridge_status_lbl  = NULL;
static GtkWidget *bridge_toggle_btn  = NULL;

static void bridge_set_inactive_ui(void) {
    if (bridge_status_lbl)
        gtk_label_set_text(GTK_LABEL(bridge_status_lbl), "Bridge: inactive");
    if (bridge_toggle_btn)
        gtk_button_set_label(GTK_BUTTON(bridge_toggle_btn), "Enable Steam Mode");
}

/* Called on the main loop when pulse-jack-bridge exits, however it exited.
 * This replaces a 1Hz waitpid(WNOHANG) timeout that polled for the whole time
 * the bridge was up — needless wakeups in a stack whose routing daemon is
 * explicitly event-driven and idle at zero CPU. */
static void on_bridge_child_exit(GPid pid, gint status, gpointer user_data) {
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
    if (pid == bridge_pid) {
        bridge_pid = 0;
        bridge_set_inactive_ui();
    }
}

static void on_bridge_toggle_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;

    if (bridge_pid != 0) {
        /* Ask it to exit; the child watch reaps it and resets the UI. Do not
           g_spawn_close_pid() here — that would drop the watch's handle. */
        kill((pid_t)bridge_pid, SIGTERM);
        bridge_set_inactive_ui();
        return;
    }

    /* Verify JACK is running before spawning */
    gchar *out = NULL; gint st = 0;
    gboolean jack_ok = g_spawn_command_line_sync("jack_lsp", &out, NULL, &st, NULL) &&
                       WIFEXITED(st) && WEXITSTATUS(st) == 0;
    if (out) g_free(out);
    if (!jack_ok) {
        GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(btn));
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "JACK is not running.\n\nStart JACK first via the Settings dialog.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    GError *err = NULL;
    gchar *argv_bridge[] = { (gchar *)"pulse-jack-bridge", NULL };
    gboolean ok = g_spawn_async(NULL, argv_bridge, NULL,
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL, &bridge_pid, &err);
    if (!ok) {
        GtkWindow *parent = get_parent_window_from_widget(GTK_WIDGET(btn));
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Failed to start pulse-jack-bridge:\n%s\n\n"
            "Ensure it is installed: sudo install.sh",
            err ? err->message : "unknown error");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
        return;
    }

    gtk_label_set_text(GTK_LABEL(bridge_status_lbl), "Bridge: active (JACK connected)");
    gtk_button_set_label(GTK_BUTTON(bridge_toggle_btn), "Disable Steam Mode");
    g_child_watch_add(bridge_pid, on_bridge_child_exit, NULL);
}

static void create_steam_panel(GtkWidget *main_box) {
    GtkWidget *expander = gtk_expander_new("Steam Gaming Mode");
    gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
    gtk_box_pack_start(GTK_BOX(main_box), expander, FALSE, FALSE, 0);

    g_steam_expander = expander;
    g_signal_connect(G_OBJECT(expander), "notify::expanded",
                     G_CALLBACK(on_any_expander_toggled), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_container_add(GTK_CONTAINER(expander), vbox);

    bridge_status_lbl = gtk_label_new("Bridge: inactive");
    gtk_widget_set_halign(bridge_status_lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), bridge_status_lbl, FALSE, FALSE, 0);

    bridge_toggle_btn = gtk_button_new_with_label("Enable Steam Mode");
    gtk_box_pack_start(GTK_BOX(vbox), bridge_toggle_btn, FALSE, FALSE, 0);
    g_signal_connect(bridge_toggle_btn, "clicked",
                     G_CALLBACK(on_bridge_toggle_clicked), NULL);
}

/* ====================================================================== */

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

    /* Tag each radio with the jack-route-select target it selects. The handler
       dispatches on this rather than the button label, so the visible text can
       change freely without affecting routing. */
    g_object_set_data(G_OBJECT(ui->rb_internal), "route_target", (gpointer)"internal");
    g_object_set_data(G_OBJECT(ui->rb_usb),      "route_target", (gpointer)"usb");
    g_object_set_data(G_OBJECT(ui->rb_hdmi),     "route_target", (gpointer)"hdmi");
    g_object_set_data(G_OBJECT(ui->rb_bt),       "route_target", (gpointer)"bluetooth");

    /* Expose the two the Bluetooth panel drives (select BT on success, fall
       back to Internal on failure) */
    g_rb_internal = ui->rb_internal;
    g_rb_bt = ui->rb_bt;

    /* Presence-based sensitivity (no hardcoding) */
    gtk_widget_set_sensitive(ui->rb_internal, TRUE);
    gtk_widget_set_sensitive(ui->rb_usb, is_usb_present());
    gtk_widget_set_sensitive(ui->rb_hdmi, TRUE);  /* HDMI always available if card supports it */
    gtk_widget_set_sensitive(ui->rb_bt, is_bt_present());

    /* Initialize radio button based on current JACK routing state, fallback to persisted preference.
     * Signals are connected AFTER this block so setting the initial state does not trigger
     * routing callbacks. Connecting first and then calling gtk_toggle_button_set_active()
     * would fire on_device_radio_toggled() during init — the Bluetooth branch requires a
     * device selected in the BT tree (which has no selection yet), causing a spurious
     * error dialog and an unwanted re-route to Internal that kills the active alsa_out. */
    gchar *pref = load_preferred_output();
    gboolean have_usb = gtk_widget_get_sensitive(ui->rb_usb);
    gboolean have_hdmi = gtk_widget_get_sensitive(ui->rb_hdmi);
    gboolean have_bt = gtk_widget_get_sensitive(ui->rb_bt);

    /* Check current routing state by looking for active JACK ports */
    gboolean usb_active = FALSE;
    gboolean hdmi_active = FALSE;
    gboolean bt_active = FALSE;

    if (have_usb) {
        FILE *fp = popen("jack_lsp 2>/dev/null | grep -q '^usb_out:'", "r");
        if (fp) { usb_active = (pclose(fp) == 0); }
    }

    if (have_hdmi) {
        FILE *fp = popen("jack_lsp 2>/dev/null | grep -q '^hdmi_out:'", "r");
        if (fp) { hdmi_active = (pclose(fp) == 0); }
    }

    if (have_bt) {
        bt_active = bluealsa_ports_exist();
    }

    /* Track which on-demand device needs its alsa_out spawned.
     * jack-bridge-ports restores HDMI/USB at boot; this is a fallback for when that
     * didn't run (e.g. GUI opened without a full boot cycle).
     * BT cannot be spawned here — requires an active BlueZ connection. */
    const char *startup_route = NULL;
    int init_dev_type = 0; /* 0=internal, 1=usb, 2=hdmi, 3=bt */

    if (bt_active) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_bt), TRUE);
        init_dev_type = 3;
    } else if (usb_active) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_usb), TRUE);
        init_dev_type = 1;
    } else if (hdmi_active || (g_strcmp0(pref, "hdmi") == 0 && have_hdmi)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_hdmi), TRUE);
        if (!hdmi_active) startup_route = "hdmi"; /* ports not yet running */
        init_dev_type = 2;
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->rb_internal), TRUE);
        init_dev_type = 0;
    }
    g_free(pref);

    /* Connect routing signals only after initial state is set (see comment above) */
    g_signal_connect(ui->rb_internal, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_usb, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_hdmi, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);
    g_signal_connect(ui->rb_bt, "toggled", G_CALLBACK(on_device_radio_toggled), NULL);

    /* Spawn on-demand ports for the startup device. This replaces the accidental
     * routing that previously fired because signals were connected before
     * gtk_toggle_button_set_active() was called. */
    if (startup_route) {
        route_to_target_async(startup_route);
    }

    /* Sync mixer UI to the initially-selected device.
     * main() always initializes the mixer with the internal card; if a different
     * device is active on open we must update the mixer here, since signals were
     * intentionally connected after gtk_toggle_button_set_active() so
     * on_device_radio_toggled() is never fired during init. */
    if (init_dev_type == 1) {
        int usb_card = get_usb_card_number();
        if (usb_card >= 0) rebuild_mixer_for_card(usb_card, FALSE);
    } else if (init_dev_type == 2) {
        show_mixer_placeholder("Mixer controls are not available for HDMI output.\nUse your display or receiver to adjust the volume.");
    } else if (init_dev_type == 3) {
        show_mixer_placeholder("Mixer controls are not available for Bluetooth output.\nUse your Bluetooth device to adjust the volume.");
    }
    /* init_dev_type == 0 (internal): already loaded correctly in main() */
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


/* Helper: check if bluealsa JACK ports exist (returns TRUE if found) */
static gboolean bluealsa_ports_exist(void) {
    gchar *out = NULL;
    gint status = 0;

    if (!g_spawn_command_line_sync("jack_lsp", &out, NULL, &status, NULL))
        return FALSE;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        g_free(out);
        return FALSE;
    }

    gboolean found = (out && strstr(out, "bluealsa:playback_1") != NULL);
    g_free(out);
    return found;
}

/* Button handler: set the selected Bluetooth device as the current OUTPUT.
 * Shares bt_set_output_begin() with the Devices panel's Bluetooth radio, which
 * handles the spawn, the port polling, the result dialogs and the panel sync. */
static void on_bt_set_output_clicked(GtkButton *b, gpointer user_data) {
    (void)user_data;
    GtkWidget *btnw = GTK_WIDGET(b);
    GtkWindow *parent = get_parent_window_from_widget(btnw);
    GtkTreeView *tv = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(btnw), "device_tree"));
    if (!tv) return;

    gchar *obj = tree_get_selected_obj(tv);
    if (!obj) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK, "No device selected");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    gchar *mac = mac_from_bluez_object(obj);
    g_free(obj);
    if (!mac) {
        GtkWidget *d = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "Failed to derive Bluetooth MAC from selection.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    bt_set_output_begin(mac);
    g_free(mac);
}

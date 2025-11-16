#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h> // For g_mkdir_with_parents and file operations

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

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

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
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 510); // Skinnier width, adjusted height
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Main vertical box to hold both mixer and EQ sections
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // Reduced spacing
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

    // EQ frame with border
    GtkWidget *eq_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(main_box), eq_frame, FALSE, FALSE, 0);

    // EQ horizontal box
    GtkWidget *eq_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(eq_box), 2); // Reduced padding
    gtk_widget_set_size_request(eq_box, -1, 200);
    gtk_container_add(GTK_CONTAINER(eq_frame), eq_box);

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
        // Append correct unit based on frequency
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

    // Preset controls: entry, save button, and combo box in a grid
    GtkWidget *preset_box = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(preset_box), 5);
    gtk_widget_set_halign(preset_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), preset_box, FALSE, FALSE, 1); // Kept 1px margin

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

    gtk_widget_show_all(window);
    gtk_main();

    cleanup_alsa(&mixer_data, &eq_data);
    return 0;
}

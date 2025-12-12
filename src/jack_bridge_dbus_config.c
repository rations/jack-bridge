/*
 * jack_bridge_dbus_config.c
 * JackConfigure interface implementation for jack-bridge D-Bus service
 * 
 * Implements org.jackaudio.Configure D-Bus interface
 * Translates qjackctl's parameter paths to /etc/default/jackd-rt variables
 */

#include "jack_bridge_dbus_config.h"
#include "jack_bridge_settings_sync.h"
#include "jack_bridge_dbus_live.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>

/* Parameter mapping structure */
typedef struct {
    const gchar *path[3];      /* D-Bus path: ["driver", "rate", NULL] */
    const gchar *shell_var;    /* Shell variable: "JACKD_SR" */
    ParamType type;            /* TYPE_INT, TYPE_STRING, TYPE_BOOL */
    const gchar *default_val;  /* Default value */
} ParamMapping;

/* Complete parameter mapping table */
static const ParamMapping PARAM_MAP[] = {
    /* Driver parameters */
    { {"driver", "rate", NULL}, "JACKD_SR", TYPE_INT, "48000" },
    { {"driver", "period", NULL}, "JACKD_PERIOD", TYPE_INT, "256" },
    { {"driver", "nperiods", NULL}, "JACKD_NPERIODS", TYPE_INT, "3" },
    { {"driver", "device", NULL}, "JACKD_DEVICE", TYPE_STRING, "" },
    { {"driver", "midi-driver", NULL}, "JACKD_MIDI", TYPE_STRING, "seq" },
    
    /* Engine parameters */
    { {"engine", "driver", NULL}, NULL, TYPE_STRING, "alsa" },  /* Always "alsa" */
    { {"engine", "realtime", NULL}, NULL, TYPE_BOOL, "true" },  /* Always true for -R */
    { {"engine", "realtime-priority", NULL}, "JACKD_PRIORITY", TYPE_INT, "70" },
    { {"engine", "port-max", NULL}, NULL, TYPE_INT, "256" },    /* Use jackd default */
    { {"engine", "sync", NULL}, NULL, TYPE_BOOL, "false" },     /* Not supported */
    
    /* Terminator */
    { {NULL, NULL, NULL}, NULL, TYPE_INT, NULL }
};

/*
 * find_mapping()
 * Find parameter mapping by D-Bus path
 */
static const ParamMapping *find_mapping(const gchar **path_array) {
    gint i, j;
    
    for (i = 0; PARAM_MAP[i].path[0] != NULL; i++) {
        gboolean match = TRUE;
        
        /* Compare path components */
        for (j = 0; j < 3; j++) {
            if (path_array[j] == NULL && PARAM_MAP[i].path[j] == NULL) {
                break; /* Both NULL, match */
            }
            if (path_array[j] == NULL || PARAM_MAP[i].path[j] == NULL) {
                match = FALSE; /* One NULL, no match */
                break;
            }
            if (g_strcmp0(path_array[j], PARAM_MAP[i].path[j]) != 0) {
                match = FALSE;
                break;
            }
        }
        
        if (match) {
            return &PARAM_MAP[i];
        }
    }
    
    return NULL;
}

/*
 * handle_get_parameter_value()
 * D-Bus method: GetParameterValue(path: as) → (is_set: b, default: v, value: v)
 */
void handle_get_parameter_value(GDBusConnection *connection,
                                 const gchar *sender,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    
    const gchar **path_array;
    const ParamMapping *mapping;
    GVariant *result;
    gboolean is_set;
    GVariant *default_variant, *value_variant;
    
    /* Extract path array from parameters */
    g_variant_get(parameters, "(^as)", &path_array);
    
    g_print("jack-bridge-dbus: GetParameterValue([");
    for (gint i = 0; path_array[i] != NULL; i++) {
        g_print("%s%s", i > 0 ? ", " : "", path_array[i]);
    }
    g_print("])\n");
    
    /* Find mapping */
    mapping = find_mapping(path_array);
    if (!mapping) {
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "Unknown parameter path");
        return;
    }
    
    /* Handle read-only parameters (no shell_var) */
    if (mapping->shell_var == NULL) {
        /* Return hardcoded values for engine parameters */
        is_set = TRUE;
        
        if (mapping->type == TYPE_STRING) {
            default_variant = g_variant_new_string(mapping->default_val);
            value_variant = g_variant_new_string(mapping->default_val);
        } else if (mapping->type == TYPE_INT) {
            gint val = atoi(mapping->default_val);
            default_variant = g_variant_new_uint32(val);
            value_variant = g_variant_new_uint32(val);
        } else { /* TYPE_BOOL */
            gboolean val = (g_strcmp0(mapping->default_val, "true") == 0);
            default_variant = g_variant_new_boolean(val);
            value_variant = g_variant_new_boolean(val);
        }
        
        result = g_variant_new("(bvv)", is_set, default_variant, value_variant);
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_value(invocation, result);
        return;
    }
    
    /* Read from configuration file */
    gchar *config_val = get_config_value(mapping->shell_var);
    is_set = (config_val != NULL && strlen(config_val) > 0);
    
    /* Build variants based on type */
    if (mapping->type == TYPE_STRING) {
        const gchar *val = is_set ? config_val : mapping->default_val;
        default_variant = g_variant_new_string(mapping->default_val);
        value_variant = g_variant_new_string(val);
    } else if (mapping->type == TYPE_INT) {
        guint32 default_int = (guint32)atoi(mapping->default_val);
        guint32 val = is_set ? (guint32)atoi(config_val) : default_int;
        default_variant = g_variant_new_uint32(default_int);
        value_variant = g_variant_new_uint32(val);
    } else { /* TYPE_BOOL */
        gboolean default_bool = (g_strcmp0(mapping->default_val, "true") == 0);
        gboolean val = is_set ? (g_strcmp0(config_val, "true") == 0 || atoi(config_val) != 0) : default_bool;
        default_variant = g_variant_new_boolean(default_bool);
        value_variant = g_variant_new_boolean(val);
    }
    
    result = g_variant_new("(bvv)", is_set, default_variant, value_variant);
    
    g_free(config_val);
    g_strfreev((gchar **)path_array);
    
    g_dbus_method_invocation_return_value(invocation, result);
}

/*
 * handle_set_parameter_value()
 * D-Bus method: SetParameterValue(path: as, value: v) → void
 */
void handle_set_parameter_value(GDBusConnection *connection,
                                 const gchar *sender,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    
    const gchar **path_array;
    GVariant *value_variant;
    const ParamMapping *mapping;
    gboolean success = FALSE;
    
    /* Extract parameters */
    g_variant_get(parameters, "(^asv)", &path_array, &value_variant);
    
    g_print("jack-bridge-dbus: SetParameterValue([");
    for (gint i = 0; path_array[i] != NULL; i++) {
        g_print("%s%s", i > 0 ? ", " : "", path_array[i]);
    }
    g_print("])\n");
    
    /* Find mapping */
    mapping = find_mapping(path_array);
    if (!mapping) {
        g_variant_unref(value_variant);
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "Unknown parameter path");
        return;
    }
    
    /* Reject changes to read-only parameters */
    if (mapping->shell_var == NULL) {
        g_variant_unref(value_variant);
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Parameter is read-only");
        return;
    }
    
    /* Extract and validate value based on type */
    if (mapping->type == TYPE_INT) {
        guint32 int_val = g_variant_get_uint32(value_variant);
        
        /* Validate based on parameter */
        if (g_strcmp0(mapping->shell_var, "JACKD_SR") == 0) {
            if (!validate_sample_rate(int_val)) {
                g_variant_unref(value_variant);
                g_strfreev((gchar **)path_array);
                g_dbus_method_invocation_return_error(invocation,
                                                      G_DBUS_ERROR,
                                                      G_DBUS_ERROR_INVALID_ARGS,
                                                      "Invalid sample rate: %u", int_val);
                return;
            }
        } else if (g_strcmp0(mapping->shell_var, "JACKD_PERIOD") == 0) {
            if (!validate_period(int_val)) {
                g_variant_unref(value_variant);
                g_strfreev((gchar **)path_array);
                g_dbus_method_invocation_return_error(invocation,
                                                      G_DBUS_ERROR,
                                                      G_DBUS_ERROR_INVALID_ARGS,
                                                      "Invalid period (must be power of 2, 16-4096): %u", int_val);
                return;
            }
        } else if (g_strcmp0(mapping->shell_var, "JACKD_NPERIODS") == 0) {
            if (!validate_nperiods(int_val)) {
                g_variant_unref(value_variant);
                g_strfreev((gchar **)path_array);
                g_dbus_method_invocation_return_error(invocation,
                                                      G_DBUS_ERROR,
                                                      G_DBUS_ERROR_INVALID_ARGS,
                                                      "Invalid nperiods (must be 2-8): %u", int_val);
                return;
            }
        } else if (g_strcmp0(mapping->shell_var, "JACKD_PRIORITY") == 0) {
            if (!validate_priority(int_val)) {
                g_variant_unref(value_variant);
                g_strfreev((gchar **)path_array);
                g_dbus_method_invocation_return_error(invocation,
                                                      G_DBUS_ERROR,
                                                      G_DBUS_ERROR_INVALID_ARGS,
                                                      "Invalid priority (0 or 10-89): %u", int_val);
                return;
            }
        }
        
        /* Special handling for JACKD_PERIOD - try live update first */
        if (g_strcmp0(mapping->shell_var, "JACKD_PERIOD") == 0) {
            gint live_result = try_live_buffer_size_change(int_val);
            
            if (live_result == 0) {
                /* Live change succeeded */
                g_print("jack-bridge-dbus: Set %s=%u → OK (live update)\n",
                        mapping->shell_var, int_val);
                success = TRUE;
            } else if (live_result == 2) {
                /* JACK not running, config saved */
                g_print("jack-bridge-dbus: Set %s=%u → OK (JACK not running, saved to config)\n",
                        mapping->shell_var, int_val);
                success = TRUE;
            } else {
                /* Live change failed, config saved for restart */
                g_print("jack-bridge-dbus: Set %s=%u → Saved (restart required for change to take effect)\n",
                        mapping->shell_var, int_val);
                success = TRUE; /* Config was saved successfully */
            }
        } else {
            /* Other integer parameters - just write to config */
            success = set_config_int(mapping->shell_var, int_val);
            g_print("jack-bridge-dbus: Set %s=%u → %s\n",
                    mapping->shell_var, int_val, success ? "OK" : "FAILED");
        }
        
    } else if (mapping->type == TYPE_STRING) {
        const gchar *str_val = g_variant_get_string(value_variant, NULL);
        
        /* Special handling for device (preserve auto-detection) */
        if (g_strcmp0(mapping->shell_var, "JACKD_DEVICE") == 0) {
            /* Empty string means auto-detect */
            success = set_config_value(mapping->shell_var, str_val);
        } else {
            success = set_config_value(mapping->shell_var, str_val);
        }
        
        g_print("jack-bridge-dbus: Set %s=\"%s\" → %s\n",
                mapping->shell_var, str_val, success ? "OK" : "FAILED");
        
    } else { /* TYPE_BOOL */
        gboolean bool_val = g_variant_get_boolean(value_variant);
        const gchar *str_val = bool_val ? "1" : "0";
        success = set_config_value(mapping->shell_var, str_val);
        
        g_print("jack-bridge-dbus: Set %s=%s → %s\n",
                mapping->shell_var, str_val, success ? "OK" : "FAILED");
    }
    
    g_variant_unref(value_variant);
    g_strfreev((gchar **)path_array);
    
    if (success) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Failed to write configuration");
    }
}

/*
 * handle_reset_parameter_value()
 * D-Bus method: ResetParameterValue(path: as) → void
 */
void handle_reset_parameter_value(GDBusConnection *connection,
                                   const gchar *sender,
                                   GVariant *parameters,
                                   GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    
    const gchar **path_array;
    const ParamMapping *mapping;
    
    g_variant_get(parameters, "(^as)", &path_array);
    
    g_print("jack-bridge-dbus: ResetParameterValue([");
    for (gint i = 0; path_array[i] != NULL; i++) {
        g_print("%s%s", i > 0 ? ", " : "", path_array[i]);
    }
    g_print("])\n");
    
    mapping = find_mapping(path_array);
    if (!mapping || !mapping->shell_var) {
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "Cannot reset this parameter");
        return;
    }
    
    /* Reset to default value */
    gboolean success = set_config_value(mapping->shell_var, mapping->default_val);
    
    g_strfreev((gchar **)path_array);
    
    if (success) {
        g_print("jack-bridge-dbus: Reset %s to default\n", mapping->shell_var);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Failed to reset parameter");
    }
}

/*
 * handle_get_parameter_constraint()
 * D-Bus method: GetParameterConstraint(path: as) → (is_strict: b, is_fake: b, values: av)
 * 
 * Returns constraints for parameters (e.g., valid sample rates, devices list)
 */
void handle_get_parameter_constraint(GDBusConnection *connection,
                                      const gchar *sender,
                                      GVariant *parameters,
                                      GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    
    const gchar **path_array;
    const ParamMapping *mapping;
    GVariantBuilder builder;
    GVariant *result;
    gboolean is_strict = TRUE;
    gboolean is_fake = FALSE;
    
    g_variant_get(parameters, "(^as)", &path_array);
    
    mapping = find_mapping(path_array);
    if (!mapping) {
        g_strfreev((gchar **)path_array);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "Unknown parameter path");
        return;
    }
    
    g_variant_builder_init(&builder, G_VARIANT_TYPE("av"));
    
    /* Build constraint list based on parameter */
    if (g_strcmp0(mapping->shell_var, "JACKD_SR") == 0) {
        /* Sample rates */
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(22050));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(44100));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(48000));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(88200));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(96000));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(176400));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(192000));
        
    } else if (g_strcmp0(mapping->shell_var, "JACKD_PERIOD") == 0) {
        /* Common buffer sizes (powers of 2) */
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(16));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(32));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(64));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(128));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(256));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(512));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(1024));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(2048));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(4096));
        
    } else if (g_strcmp0(mapping->shell_var, "JACKD_NPERIODS") == 0) {
        /* Periods/buffer range */
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(2));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(3));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(4));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(5));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(6));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(7));
        g_variant_builder_add(&builder, "v", g_variant_new_uint32(8));
        
    } else if (g_strcmp0(mapping->shell_var, "JACKD_DEVICE") == 0) {
        /* Device list from aplay -l */
        FILE *fp = popen("aplay -l 2>/dev/null", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                /* Parse "card N: NAME [DESC]" */
                if (strstr(line, "card ") == line) {
                    int card_num;
                    char name[128], desc[128];
                    if (sscanf(line, "card %d: %127[^[][%127[^]]]", &card_num, name, desc) >= 2) {
                        /* Remove trailing spaces from name */
                        char *end = name + strlen(name) - 1;
                        while (end > name && (*end == ' ' || *end == '\t')) {
                            *end = '\0';
                            end--;
                        }
                        
                        /* Add hw:N device */
                        gchar *device = g_strdup_printf("hw:%d", card_num);
                        g_variant_builder_add(&builder, "v", g_variant_new_string(device));
                        g_free(device);
                        
                        /* Also add hw:CARD=NAME if name is valid */
                        if (strlen(name) > 0) {
                            /* Sanitize name for CARD= usage */
                            for (char *p = name; *p; p++) {
                                if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-') {
                                    *p = '_';
                                }
                            }
                            device = g_strdup_printf("hw:CARD=%s", name);
                            g_variant_builder_add(&builder, "v", g_variant_new_string(device));
                            g_free(device);
                        }
                    }
                }
            }
            pclose(fp);
        }
        
        /* Add empty option for auto-detect */
        g_variant_builder_add(&builder, "v", g_variant_new_string(""));
        is_strict = FALSE; /* Allow custom entries */
        
    } else if (g_strcmp0(mapping->shell_var, "JACKD_MIDI") == 0) {
        /* MIDI driver options */
        g_variant_builder_add(&builder, "v", g_variant_new_string("seq"));
        g_variant_builder_add(&builder, "v", g_variant_new_string("raw"));
        g_variant_builder_add(&builder, "v", g_variant_new_string("none"));
        
    } else {
        /* No constraints for other parameters */
        is_strict = FALSE;
    }
    
    result = g_variant_new("(bbav)", is_strict, is_fake, &builder);
    
    g_strfreev((gchar **)path_array);
    
    g_dbus_method_invocation_return_value(invocation, result);
}
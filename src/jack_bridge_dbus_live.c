/*
 * jack_bridge_dbus_live.c
 * Live JACK parameter updates without full restart
 * 
 * Implements live buffer size changes via jack_set_buffer_size() API
 * Falls back to full restart if live update fails
 */

#include "jack_bridge_dbus_live.h"
#include "jack_bridge_settings_sync.h"
#include <stdio.h>
#include <jack/jack.h>
#include <glib.h>

/*
 * try_live_buffer_size_change()
 * Attempt to change buffer size without restarting JACK
 * 
 * Returns:
 *   0 = Success (live change applied)
 *   1 = Failed (restart required)
 *   2 = JACK not running (change saved to config only)
 */
gint try_live_buffer_size_change(guint32 new_period) {
    jack_client_t *client;
    jack_status_t status;
    gint result;
    
    g_print("jack-bridge-dbus-live: Attempting live buffer size change: %u frames\n", new_period);
    
    /* Try to connect to JACK */
    client = jack_client_open("jack-bridge-dbus-bufsize",
                              JackNoStartServer,
                              &status);
    
    if (!client) {
        g_print("jack-bridge-dbus-live: JACK not running, saving to config only\n");
        
        /* Write to config file for next restart */
        if (!set_config_int("JACKD_PERIOD", (gint)new_period)) {
            g_printerr("jack-bridge-dbus-live: Failed to write config\n");
            return 1;
        }
        
        return 2; /* JACK not running */
    }
    
    g_print("jack-bridge-dbus-live: Connected to JACK, attempting jack_set_buffer_size()\n");
    
    /* Try live buffer size change */
    if (jack_set_buffer_size(client, new_period) == 0) {
        g_print("jack-bridge-dbus-live: Live buffer size change successful!\n");
        
        /* Update config file to match */
        if (!set_config_int("JACKD_PERIOD", (gint)new_period)) {
            g_printerr("jack-bridge-dbus-live: WARNING: Failed to write config (live change succeeded but config not saved)\n");
        }
        
        result = 0; /* Success */
    } else {
        g_printerr("jack-bridge-dbus-live: jack_set_buffer_size() failed, restart required\n");
        
        /* Save to config for restart */
        if (!set_config_int("JACKD_PERIOD", (gint)new_period)) {
            g_printerr("jack-bridge-dbus-live: Failed to write config\n");
        }
        
        result = 1; /* Failed - restart required */
    }
    
    jack_client_close(client);
    return result;
}

/*
 * check_needs_restart()
 * Determine if a parameter change requires full JACK restart
 * 
 * Returns TRUE if restart required, FALSE if live update possible
 */
gboolean check_needs_restart(const gchar *param_name) {
    /* Only JACKD_PERIOD (frames/period) can potentially change live */
    if (g_strcmp0(param_name, "JACKD_PERIOD") == 0) {
        return FALSE; /* Try live update first */
    }
    
    /* All other parameters require full restart */
    if (g_strcmp0(param_name, "JACKD_NPERIODS") == 0 ||
        g_strcmp0(param_name, "JACKD_SR") == 0 ||
        g_strcmp0(param_name, "JACKD_DEVICE") == 0 ||
        g_strcmp0(param_name, "JACKD_PRIORITY") == 0 ||
        g_strcmp0(param_name, "JACKD_MIDI") == 0 ||
        g_strcmp0(param_name, "JACKD_USER") == 0) {
        return TRUE;
    }
    
    /* Unknown parameter - assume restart required */
    return TRUE;
}

/*
 * get_restart_message()
 * Get user-friendly message explaining restart requirement
 */
const gchar *get_restart_message(const gchar *param_name) {
    if (g_strcmp0(param_name, "JACKD_NPERIODS") == 0) {
        return "Periods/buffer changed. Click OK then use Stop→Start buttons to apply.";
    } else if (g_strcmp0(param_name, "JACKD_SR") == 0) {
        return "Sample rate changed. Click OK then use Stop→Start buttons to apply.";
    } else if (g_strcmp0(param_name, "JACKD_DEVICE") == 0) {
        return "Audio device changed. Click OK then use Stop→Start buttons to apply.";
    } else if (g_strcmp0(param_name, "JACKD_PRIORITY") == 0) {
        return "Realtime priority changed. Click OK then use Stop→Start buttons to apply.";
    } else if (g_strcmp0(param_name, "JACKD_MIDI") == 0) {
        return "MIDI driver changed. Click OK then use Stop→Start buttons to apply.";
    }
    
    return "Setting changed. Click OK then use Stop→Start buttons to apply.";
}
/*
 * jack_connection_manager.c
 * Event-driven JACK connection manager for jack-bridge
 *
 * Uses JACK's port registration callback (zero-CPU when idle) instead of polling.
 * Automatically routes new audio sources to user's PREFERRED_OUTPUT selection.
 * Runs as the user, reads ~/.config/jack-bridge/devices.conf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <jack/jack.h>

#define MAX_LINE 256
#define USER_CONF_PATH ".config/jack-bridge/devices.conf"
#define SYS_CONF_PATH "/etc/jack-bridge/devices.conf"

/* Global state */
static jack_client_t *client = NULL;
static volatile int keep_running = 1;
static volatile int needs_reconnect = 0; /* Flag for deferred connection */
static char preferred_output[64] = "internal";
static char target_sink_prefix[64] = "system:playback_";

/* Signal handler for clean shutdown */
static void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

/* Read PREFERRED_OUTPUT from config files */
static void load_config(void) {
    FILE *f;
    char line[MAX_LINE];
    char path[512];
    const char *home = getenv("HOME");
    
    /* Try system config first */
    f = fopen(SYS_CONF_PATH, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "PREFERRED_OUTPUT=", 17) == 0) {
                char *val = line + 17;
                char *end = strchr(val, '\n');
                if (end) *end = '\0';
                /* Remove quotes if present */
                if (*val == '"' || *val == '\'') val++;
                end = val + strlen(val) - 1;
                if (end > val && (*end == '"' || *end == '\'')) *end = '\0';
                strncpy(preferred_output, val, sizeof(preferred_output) - 1);
            }
        }
        fclose(f);
    }
    
    /* Try user config (overrides system) */
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, USER_CONF_PATH);
        f = fopen(path, "r");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "PREFERRED_OUTPUT=", 17) == 0) {
                    char *val = line + 17;
                    char *end = strchr(val, '\n');
                    if (end) *end = '\0';
                    /* Remove quotes */
                    if (*val == '"' || *val == '\'') val++;
                    end = val + strlen(val) - 1;
                    if (end > val && (*end == '"' || *end == '\'')) *end = '\0';
                    strncpy(preferred_output, val, sizeof(preferred_output) - 1);
                }
            }
            fclose(f);
        }
    }
    
    /* Set target sink prefix based on preferred output */
    if (strcmp(preferred_output, "usb") == 0) {
        strcpy(target_sink_prefix, "usb_out:playback_");
    } else if (strcmp(preferred_output, "hdmi") == 0) {
        strcpy(target_sink_prefix, "hdmi_out:playback_");
    } else if (strcmp(preferred_output, "bluetooth") == 0) {
        strcpy(target_sink_prefix, "bluealsa:playback_");
    } else {
        strcpy(target_sink_prefix, "system:playback_");
    }
}

/* Check if port is a known sink (output device) */
static int is_sink_port(const char *port_name) {
    return (strstr(port_name, "system:playback_") != NULL ||
            strstr(port_name, "usb_out:playback_") != NULL ||
            strstr(port_name, "hdmi_out:playback_") != NULL ||
            strstr(port_name, "bluealsa:playback_") != NULL);
}

/* Check if port is a capture port */
static int is_capture_port(const char *port_name) {
    return strstr(port_name, ":capture_") != NULL;
}

/* Check if port is a MIDI port */
static int is_midi_port(const char *port_name) {
    return (strstr(port_name, ":midi_") != NULL ||
            strstr(port_name, "Midi-Through:") != NULL);
}

/* Disconnect source port from ALL known sinks */
static void disconnect_from_all_sinks(const char *source_port) {
    const char **connections;
    jack_port_t *port;
    int i;
    
    port = jack_port_by_name(client, source_port);
    if (!port) return;
    
    connections = jack_port_get_all_connections(client, port);
    if (!connections) return;
    
    /* Disconnect from any known sink port */
    for (i = 0; connections[i]; i++) {
        if (is_sink_port(connections[i])) {
            jack_disconnect(client, source_port, connections[i]);
        }
    }
    
    jack_free(connections);
}

/* Connect source port to target sink with smart channel mapping */
static void connect_source_to_sink(const char *source_port) {
    char target1[128], target2[128];
    int ret;
    
    /* CRITICAL: Disconnect from all other sinks first
     * Apps connect to system:playback via ALSA->JACK bridge by default.
     * We must disconnect them before routing to user's preferred output. */
    disconnect_from_all_sinks(source_port);
    
    snprintf(target1, sizeof(target1), "%s1", target_sink_prefix);
    snprintf(target2, sizeof(target2), "%s2", target_sink_prefix);
    
    /* Verify target ports exist before trying to connect */
    if (!jack_port_by_name(client, target1) || !jack_port_by_name(client, target2)) {
        fprintf(stderr, "jack-connection-manager: ERROR: Target ports %s/%s do not exist!\n",
                target1, target2);
        fprintf(stderr, "jack-connection-manager: Bridge ports may not be spawned yet. Skipping.\n");
        return;
    }
    
    /* Smart channel mapping based on port name */
    if (strstr(source_port, ":out_1") || strstr(source_port, ":left") ||
        strstr(source_port, ":L") || strstr(source_port, "playback_1")) {
        /* Left channel only */
        ret = jack_connect(client, source_port, target1);
        if (ret != 0 && ret != EEXIST) {
            fprintf(stderr, "jack-connection-manager: ERROR: Failed to connect %s -> %s (error %d)\n",
                    source_port, target1, ret);
        }
    } else if (strstr(source_port, ":out_2") || strstr(source_port, ":right") ||
               strstr(source_port, ":R") || strstr(source_port, "playback_2")) {
        /* Right channel only */
        ret = jack_connect(client, source_port, target2);
        if (ret != 0 && ret != EEXIST) {
            fprintf(stderr, "jack-connection-manager: ERROR: Failed to connect %s -> %s (error %d)\n",
                    source_port, target2, ret);
        }
    } else {
        /* Mono or unknown: connect to both */
        ret = jack_connect(client, source_port, target1);
        if (ret != 0 && ret != EEXIST) {
            fprintf(stderr, "jack-connection-manager: ERROR: Failed to connect %s -> %s (error %d)\n",
                    source_port, target1, ret);
        }
        ret = jack_connect(client, source_port, target2);
        if (ret != 0 && ret != EEXIST) {
            fprintf(stderr, "jack-connection-manager: ERROR: Failed to connect %s -> %s (error %d)\n",
                    source_port, target2, ret);
        }
    }
}

/* Port registration callback - called when ports appear/disappear
 * NOTE: We CANNOT call jack_connect() from this callback (runs in JACK's notification thread).
 * Instead, set a flag and defer connection to main thread. */
static void port_registration_callback(jack_port_id_t port_id, int registered, void *arg) {
    (void)arg;
    (void)port_id;
    
    if (!registered) return; /* Only care about new ports */
    
    /* Signal main thread to process connections */
    needs_reconnect = 1;
}

/* Process all pending connections (called from main thread, safe for jack_connect) */
static void process_connections(void) {
    const char **ports;
    int i;
    
    /* Reload config to catch GUI changes */
    load_config();
    
    /* Get all output ports */
    ports = jack_get_ports(client, NULL, NULL, JackPortIsOutput);
    if (!ports) return;
    
    for (i = 0; ports[i]; i++) {
        const char *port_name = ports[i];
        
        /* Skip sink ports, capture ports, and MIDI ports */
        if (is_sink_port(port_name) || is_capture_port(port_name) || is_midi_port(port_name))
            continue;
        
        /* Check if this source is already connected to our target */
        const char **connections = jack_port_get_all_connections(client, jack_port_by_name(client, port_name));
        int already_connected = 0;
        
        if (connections) {
            for (int j = 0; connections[j]; j++) {
                if (strstr(connections[j], target_sink_prefix) != NULL) {
                    already_connected = 1;
                    break;
                }
            }
            jack_free(connections);
        }
        
        if (!already_connected) {
            fprintf(stderr, "jack-connection-manager: Routing '%s' -> %s\n",
                    port_name, target_sink_prefix);
            connect_source_to_sink(port_name);
        }
    }
    
    jack_free(ports);
}

/* JACK shutdown callback */
static void jack_shutdown_callback(void *arg) {
    (void)arg;
    fprintf(stderr, "jack-connection-manager: JACK server shutdown\n");
    keep_running = 0;
}

int main(void) {
    jack_status_t status;
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Load initial config */
    load_config();
    fprintf(stderr, "jack-connection-manager: Starting (preferred output: %s)\n", preferred_output);
    
    /* Open JACK client */
    client = jack_client_open("connection_manager", JackNoStartServer, &status);
    if (!client) {
        fprintf(stderr, "jack-connection-manager: Failed to connect to JACK server\n");
        return 1;
    }
    
    /* Register callbacks */
    jack_set_port_registration_callback(client, port_registration_callback, NULL);
    jack_on_shutdown(client, jack_shutdown_callback, NULL);
    
    /* Activate client */
    if (jack_activate(client)) {
        fprintf(stderr, "jack-connection-manager: Cannot activate JACK client\n");
        jack_client_close(client);
        return 1;
    }
    
    fprintf(stderr, "jack-connection-manager: Running (event-driven, zero CPU when idle)\n");
    
    /* CRITICAL: Process existing ports at startup (don't wait for new ports)
     * At boot, apps may already be connected to system:playback via ALSA defaults.
     * We need to disconnect them and reconnect to the user's preferred output. */
    sleep(1);  /* Brief wait for bridge ports (reduced from 2s) */
    fprintf(stderr, "jack-connection-manager: Processing existing connections at startup\n");
    process_connections();
    
    /* Main loop: process connections when signaled, otherwise sleep */
    struct timespec sleep_time = {0, 100000000}; /* 100ms = 100,000,000 nanoseconds */
    while (keep_running) {
        if (needs_reconnect) {
            needs_reconnect = 0;
            process_connections();
        }
        nanosleep(&sleep_time, NULL); /* 100ms sleep - responsive but low CPU */
    }
    
    /* Clean shutdown */
    fprintf(stderr, "jack-connection-manager: Shutting down\n");
    jack_client_close(client);
    
    return 0;
}
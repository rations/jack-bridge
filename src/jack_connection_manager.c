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
#include <jack/jack.h>

#define MAX_LINE 256
#define USER_CONF_PATH ".config/jack-bridge/devices.conf"
#define SYS_CONF_PATH "/etc/jack-bridge/devices.conf"

/* Global state */
static jack_client_t *client = NULL;
static volatile int keep_running = 1;
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

/* Connect source port to target sink with smart channel mapping */
static void connect_source_to_sink(const char *source_port) {
    char target1[128], target2[128];
    
    snprintf(target1, sizeof(target1), "%s1", target_sink_prefix);
    snprintf(target2, sizeof(target2), "%s2", target_sink_prefix);
    
    /* Smart channel mapping based on port name */
    if (strstr(source_port, ":out_1") || strstr(source_port, ":left") ||
        strstr(source_port, ":L") || strstr(source_port, "playback_1")) {
        /* Left channel only */
        jack_connect(client, source_port, target1);
    } else if (strstr(source_port, ":out_2") || strstr(source_port, ":right") ||
               strstr(source_port, ":R") || strstr(source_port, "playback_2")) {
        /* Right channel only */
        jack_connect(client, source_port, target2);
    } else {
        /* Mono or unknown: connect to both */
        jack_connect(client, source_port, target1);
        jack_connect(client, source_port, target2);
    }
}

/* Port registration callback - called when ports appear/disappear */
static void port_registration_callback(jack_port_id_t port_id, int registered, void *arg) {
    (void)arg;
    
    if (!registered) return; /* Only care about new ports */
    
    /* Reload config to get latest user selection */
    load_config();
    
    jack_port_t *port = jack_port_by_id(client, port_id);
    if (!port) return;
    
    const char *port_name = jack_port_name(port);
    if (!port_name) return;
    
    /* Check if this is an output port (JackPortIsOutput flag) */
    int flags = jack_port_flags(port);
    if (!(flags & JackPortIsOutput)) return;
    
    /* Skip sink ports, capture ports, and MIDI ports */
    if (is_sink_port(port_name) || is_capture_port(port_name) || is_midi_port(port_name))
        return;
    
    /* This is a new audio source - connect it to preferred output */
    fprintf(stderr, "jack-connection-manager: New source '%s' -> routing to %s\n",
            port_name, target_sink_prefix);
    
    connect_source_to_sink(port_name);
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
    
    /* Wait for shutdown signal (uses zero CPU) */
    while (keep_running) {
        sleep(1);
    }
    
    /* Clean shutdown */
    fprintf(stderr, "jack-connection-manager: Shutting down\n");
    jack_client_close(client);
    
    return 0;
}
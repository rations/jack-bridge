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
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <jack/jack.h>

#define MAX_LINE 256
#define MAX_SRC_PORTS 64
#define USER_CONF_PATH ".config/jack-bridge/devices.conf"
#define SYS_CONF_PATH "/etc/jack-bridge/devices.conf"

/* Global state */
static jack_client_t *client = NULL;
static volatile int keep_running = 1;
static volatile int needs_reconnect = 0; /* Flag for deferred connection */
static volatile int is_processing = 0; /* Lock to prevent concurrent routing */
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

/* Disconnect source port from ALL known sinks EXCEPT the target sink */
static void disconnect_from_other_sinks(const char *source_port, const char *keep_prefix) {
    const char **connections;
    jack_port_t *port;
    int i, ret;

    port = jack_port_by_name(client, source_port);
    if (!port) return;

    connections = jack_port_get_all_connections(client, port);
    if (!connections) return;

    /* Disconnect from any known sink port EXCEPT those matching keep_prefix */
    for (i = 0; connections[i]; i++) {
        if (is_sink_port(connections[i])) {
            /* Skip if this is our target sink */
            if (keep_prefix && strstr(connections[i], keep_prefix) != NULL) {
                continue;
            }
            ret = jack_disconnect(client, source_port, connections[i]);
            if (ret == 0) {
                fprintf(stderr, "jack-connection-manager: Disconnected '%s' from '%s'\n",
                        source_port, connections[i]);
            }
        }
    }

    jack_free(connections);
}

/* Extract trailing channel number for port ordering.
 * "REAPER:out3" -> 3. Returns INT_MAX if no trailing digit (sort last). */
static int port_channel_num(const char *name) {
    const char *p = name + strlen(name);
    while (p > name && isdigit((unsigned char)*(p - 1))) p--;
    if (*p != '\0')
        return atoi(p);
    return INT_MAX;
}

static int port_cmp(const void *a, const void *b) {
    return port_channel_num(*(const char **)a) - port_channel_num(*(const char **)b);
}

/* Connect output ports of client_name to the target sink positionally.
 * Ports are sorted by trailing channel number before mapping so that
 * out1->sink1, out2->sink2 regardless of JACK's global registration order
 * (native JACK apps such as Reaper may register ports non-sequentially).
 * Source ports beyond the sink's channel count are disconnected from all
 * known sinks so they don't accumulate stale connections. */
static void connect_client_to_sink(const char *client_name, size_t name_len, const char **all_ports) {
    const char **sink_ports;
    const char *src[MAX_SRC_PORTS];
    const char *sink[MAX_SRC_PORTS];
    int i, src_count, sink_count, sink_idx, ret;
    char target1[128];

    snprintf(target1, sizeof(target1), "%s1", target_sink_prefix);
    if (!jack_port_by_name(client, target1)) {
        fprintf(stderr, "jack-connection-manager: ERROR: Target sink %s not available, skipping\n",
                target_sink_prefix);
        return;
    }

    /* Audio sinks only — JACK_DEFAULT_AUDIO_TYPE excludes any MIDI ports. */
    sink_ports = jack_get_ports(client, target_sink_prefix, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    if (!sink_ports) {
        fprintf(stderr, "jack-connection-manager: ERROR: No input ports found for %s\n",
                target_sink_prefix);
        return;
    }

    /* Copy sink ports into a sortable array and sort by channel number. */
    sink_count = 0;
    for (i = 0; sink_ports[i] && sink_count < MAX_SRC_PORTS; i++)
        sink[sink_count++] = sink_ports[i];
    qsort(sink, (size_t)sink_count, sizeof(char *), port_cmp);

    /* Collect source ports for this client and sort by channel number. */
    src_count = 0;
    for (i = 0; all_ports[i] && src_count < MAX_SRC_PORTS; i++) {
        if (strncmp(all_ports[i], client_name, name_len) != 0 || all_ports[i][name_len] != ':')
            continue;
        if (is_sink_port(all_ports[i]) || is_capture_port(all_ports[i]) || is_midi_port(all_ports[i]))
            continue;
        src[src_count++] = all_ports[i];
    }
    qsort(src, (size_t)src_count, sizeof(char *), port_cmp);

    sink_idx = 0;
    for (i = 0; i < src_count; i++) {
        if (sink_idx >= sink_count) {
            /* Sink full — disconnect excess source ports from all known sinks. */
            for (; i < src_count; i++)
                disconnect_from_other_sinks(src[i], NULL);
            break;
        }
        disconnect_from_other_sinks(src[i], target_sink_prefix);
        ret = jack_connect(client, src[i], sink[sink_idx]);
        if (ret != 0 && ret != EEXIST)
            fprintf(stderr, "jack-connection-manager: ERROR: Failed to connect %s -> %s (error %d)\n",
                    src[i], sink[sink_idx], ret);
        sink_idx++;
    }

    jack_free(sink_ports);
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
    int i, k;
    char seen_clients[64][128];
    int seen_count = 0;

    /* Prevent concurrent execution */
    if (is_processing) {
        fprintf(stderr, "jack-connection-manager: Skipping concurrent process_connections call\n");
        return;
    }
    is_processing = 1;

    /* Reload config to catch GUI changes */
    load_config();

    /* Get all AUDIO output ports. Passing JACK_DEFAULT_AUDIO_TYPE as the
     * type filter excludes MIDI ports at the source — this is the reliable,
     * JACK-ABI-correct way to tell audio from MIDI. Apps such as Reaper expose
     * MIDI ports ("REAPER:MIDI Output 1") that string-name matching misses;
     * if those leak into routing they steal sink slots and shift the audio
     * channels (out1 -> playback_2, out2 -> playback_4). */
    ports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    if (!ports) { is_processing = 0; return; }

    for (i = 0; ports[i]; i++) {
        const char *port_name = ports[i];

        if (is_sink_port(port_name) || is_capture_port(port_name) || is_midi_port(port_name))
            continue;

        /* Extract client name (text before ':') */
        const char *colon = strchr(port_name, ':');
        if (!colon) continue;
        size_t name_len = (size_t)(colon - port_name);
        if (name_len >= sizeof(seen_clients[0])) continue;

        char client_name[128];
        memcpy(client_name, port_name, name_len);
        client_name[name_len] = '\0';

        /* Skip clients already handled this pass */
        int already_seen = 0;
        for (k = 0; k < seen_count; k++) {
            if (strcmp(seen_clients[k], client_name) == 0) { already_seen = 1; break; }
        }
        if (already_seen) continue;
        if (seen_count < 64) {
            strncpy(seen_clients[seen_count++], client_name, sizeof(seen_clients[0]) - 1);
        }

        /* Check if any port of this client is not yet routed to the target */
        int needs_connect = 0;
        for (k = 0; ports[k]; k++) {
            if (strncmp(ports[k], client_name, name_len) != 0 || ports[k][name_len] != ':')
                continue;
            if (is_sink_port(ports[k]) || is_capture_port(ports[k]) || is_midi_port(ports[k]))
                continue;

            jack_port_t *p = jack_port_by_name(client, ports[k]);
            if (!p) continue;
            const char **conns = jack_port_get_all_connections(client, p);
            int connected = 0;
            if (conns) {
                int m;
                for (m = 0; conns[m]; m++) {
                    if (strstr(conns[m], target_sink_prefix)) { connected = 1; break; }
                }
                jack_free(conns);
            }
            if (!connected) { needs_connect = 1; break; }
        }

        if (needs_connect) {
            fprintf(stderr, "jack-connection-manager: Routing '%s' -> %s\n",
                    client_name, target_sink_prefix);
            connect_client_to_sink(client_name, name_len, ports);
        } else {
            /* Client already routed to target — still remove any stale
             * connections to other sinks. This handles the race where the
             * ALSA JACK plugin auto-connects a freshly opened client to
             * system:playback after the manager already routed it to USB/HDMI,
             * and similar reconnect behaviour in native JACK apps. */
            for (k = 0; ports[k]; k++) {
                if (strncmp(ports[k], client_name, name_len) != 0 || ports[k][name_len] != ':')
                    continue;
                if (is_sink_port(ports[k]) || is_capture_port(ports[k]) || is_midi_port(ports[k]))
                    continue;
                jack_port_t *sp = jack_port_by_name(client, ports[k]);
                if (!sp) continue;
                const char **sc = jack_port_get_all_connections(client, sp);
                if (!sc) continue;
                int on_target = 0, m;
                for (m = 0; sc[m]; m++) {
                    if (strstr(sc[m], target_sink_prefix)) { on_target = 1; break; }
                }
                jack_free(sc);
                if (on_target)
                    disconnect_from_other_sinks(ports[k], target_sink_prefix);
            }
        }
    }

    jack_free(ports);
    is_processing = 0;
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
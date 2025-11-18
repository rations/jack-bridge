/*
 * src/jack-bluealsa-autobridge.c
 *
 * Lightweight autobridge daemon for BlueALSA <-> JACK.
 * - Loads /etc/jack-bridge/bluetooth.conf
 * - Handles SIGHUP to reload config atomically
 * - Subscribes to BlueALSA D-Bus InterfacesAdded/InterfacesRemoved signals (org.freedesktop.DBus.ObjectManager)
 *   and detects PCM objects exposing org.bluealsa.PCM1 (common in bluez-alsa)
 * - Spawns alsa_in / alsa_out bridge processes and supervises them
 * - Reaps children and performs bounded graceful shutdown
 *
 * Build:
 *   gcc -Wall -Wextra -std=c11 `pkg-config --cflags glib-2.0 gio-2.0` \
 *       src/jack-bluealsa-autobridge.c -o bin/jack-bluealsa-autobridge \
 *       `pkg-config --libs glib-2.0 gio-2.0`
 *
 * Note: keep dependencies minimal (glib2 with GIO/GDBus).
 */

#define _POSIX_C_SOURCE 200809L

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

/* Optional ALSA check for existing clients */
#include <alsa/asoundlib.h>

#define CONFIG_PATH "/etc/jack-bridge/bluetooth.conf"
#define DEFAULT_LOG "/var/log/jack-bluealsa-autobridge.log"
#define DEFAULT_PID  "/var/run/jack-bluealsa-autobridge.pid"
#define MAX_CHILDREN 64

/* Simple logger that writes to file and to stderr for debugging */
static gchar *log_path = NULL;
static FILE *log_fp = NULL;
static gchar *pid_path = NULL;

/* Configuration structure parsed from CONFIG_PATH */
typedef struct {
    int a2dp_rate;
    int a2dp_period;
    int a2dp_nperiods;
    int a2dp_channels;
    int a2dp_drift_comp;

    int sco_rate;
    int sco_period;
    int sco_nperiods;
    int sco_channels;

    int spawn_delay;
    int child_term_timeout;
    gchar *log_file;
    gchar *pid_file;
    gchar *runtime_user;
    int max_bridges;
} jb_config_t;

static jb_config_t config;

/* Track managed child processes (map pid -> command string) */
typedef struct {
    pid_t pid;
    gchar *name;
    gchar *cmdline;
    int restart_count; /* number of automatic restarts attempted (for restart-once policy) */
} child_t;

static GHashTable *children = NULL; /* key: GINT_TO_POINTER(pid), value: child_t* */

static GMainLoop *main_loop = NULL;
static GDBusConnection *system_bus = NULL;
static guint interfaces_added_sub = 0;
static guint interfaces_removed_sub = 0;

/* Forward declarations */
static void load_default_config(jb_config_t *cfg);
static gboolean load_config_from_file(const char *path, jb_config_t *cfg, GError **error);
static void apply_config(const jb_config_t *cfg);
static void on_hup(int signo);
static void on_term(int signo);
static void install_signal_handlers(void);
static void write_pid_file(const char *path);
static void remove_pid_file(const char *path);
static void reopen_log(void);

/* Logging helpers */
static void jb_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (log_fp && log_fp != stderr) {
        vfprintf(log_fp, fmt, ap);
        fprintf(log_fp, "\n");
        fflush(log_fp);
    }
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

/* CHILD MANAGEMENT */
/* Free function for child_t stored in GHashTable */
static void child_free(gpointer data) {
    child_t *c = data;
    if (!c) return;
    if (c->name) g_free(c->name);
    if (c->cmdline) g_free(c->cmdline);
    g_free(c);
}

static void add_child(pid_t pid, const gchar *name, const gchar *cmdline) {
    child_t *c = g_new0(child_t, 1);
    c->pid = pid;
    c->name = g_strdup(name ? name : "(child)");
    c->cmdline = g_strdup(cmdline ? cmdline : "");
    c->restart_count = 0; /* initialize restart counter for restart-once policy */
    g_hash_table_insert(children, GINT_TO_POINTER(pid), c);
    jb_log("Added child pid=%d name=%s", pid, c->name);
}

static void remove_child(pid_t pid) {
    gpointer key = GINT_TO_POINTER(pid);
    child_t *c = g_hash_table_lookup(children, key);
    if (c) {
        jb_log("Removing child pid=%d name=%s", pid, c->name ? c->name : "(nil)");
        /* g_hash_table_remove will call child_free to release memory */
        g_hash_table_remove(children, key);
    }
}

/* Check whether a bridge for the given MAC is already running.
 * This prevents spawning duplicate alsa_in/alsa_out processes for the
 * same device. We search child->name for the mac substring (job names
 * were created as bt_in_<MAC>, bt_out_<MAC>, bt_sco_<MAC>).
 */
static gboolean is_bridge_running_for_mac(const char *mac) {
    if (!mac || !children) return FALSE;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, children);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        child_t *c = value;
        if (!c || !c->name) continue;
        if (strstr(c->name, mac) != NULL) {
            jb_log("Detected existing bridge '%s' for MAC %s (pid=%d), skipping spawn", c->name, mac, c->pid);
            return TRUE;
        }
    }
    return FALSE;
}

/* Reap children in periodic reaper (called from GLib context).
 *
 * Emits structured failure logs including exit status and stored cmdline.
 * Implements a restart-once policy: if a supervised bridge exits unexpectedly,
 * attempt one automatic restart. The old record is removed once handled; the
 * restart bookkeeping is applied to the newly spawned child record.
 */
static void handle_sigchld(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Lookup the child record (if present) to include the command line in logs */
        child_t *c = g_hash_table_lookup(children, GINT_TO_POINTER(pid));
        if (c) {
            if (WIFEXITED(status)) {
                int code = WEXITSTATUS(status);
                jb_log("Child exited: pid=%d name=%s exit=%d cmd=%s", pid, c->name ? c->name : "(nil)", code, c->cmdline ? c->cmdline : "(nil)");
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                jb_log("Child killed by signal: pid=%d name=%s signal=%d cmd=%s", pid, c->name ? c->name : "(nil)", sig, c->cmdline ? c->cmdline : "(nil)");
            } else {
                jb_log("Child changed state: pid=%d name=%s status=%d cmd=%s", pid, c->name ? c->name : "(nil)", status, c->cmdline ? c->cmdline : "(nil)");
            }

            /* Restart-once policy */
            if (c->restart_count < 1) {
                jb_log("Attempting automatic restart for %s (pid=%d)", c->name ? c->name : "(nil)", pid);

                /* Prepare argv from stored cmdline: split by whitespace */
                GPtrArray *parts = g_ptr_array_new_with_free_func(g_free);
                gchar **tokens = g_strsplit_set(c->cmdline ? c->cmdline : "", " \t", -1);
                for (int i = 0; tokens[i] != NULL; ++i) {
                    if (tokens[i][0] == '\0') continue;
                    g_ptr_array_add(parts, g_strdup(tokens[i]));
                }
                g_strfreev(tokens);

                /* Build argv */
                int argc = parts->len;
                char **argv = g_new0(char*, argc + 1);
                for (guint i = 0; i < parts->len; ++i) {
                    argv[i] = g_strdup((char*)g_ptr_array_index(parts, i));
                }
                argv[argc] = NULL;

                /* Spawn new child with same job name */
                pid_t newpid = spawn_bridge(c->name ? c->name : "(restarted)", argv);

                /* Free argv we created (spawn_bridge copies cmdline into record) */
                for (int i = 0; argv[i] != NULL; ++i) g_free(argv[i]);
                g_free(argv);
                g_ptr_array_free(parts, TRUE);

                if (newpid > 0) {
                    jb_log("Restarted %s as pid=%d", c->name ? c->name : "(restarted)", newpid);
                    /* Increment restart_count on the new record if present */
                    child_t *nc = g_hash_table_lookup(children, GINT_TO_POINTER(newpid));
                    if (nc) nc->restart_count = (c->restart_count) + 1;
                } else {
                    jb_log("Automatic restart failed for %s", c->name ? c->name : "(nil)");
                }

                /* Remove the old (exited) record now that we've attempted restart.
                 * This avoids leaving stale entries for dead PIDs. */
                g_hash_table_remove(children, GINT_TO_POINTER(pid));
                continue;
            } else {
                jb_log("Not restarting %s (already restarted once)", c->name ? c->name : "(nil)");
            }
        } else {
            jb_log("Child %d exited with status %d (no record found)", pid, status);
        }

        /* Remove the child record if still present */
        g_hash_table_remove(children, GINT_TO_POINTER(pid));
    }
}

/* Spawn a supervised bridge process (non-blocking) */
static pid_t spawn_bridge(const char *name, char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        jb_log("fork failed: %s", strerror(errno));
        return -1;
    } else if (pid == 0) {
        /* Child */
        /* Reset signals to default */
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /* Exec the bridge command */
        execvp(argv[0], argv);
        /* If exec fails */
        _exit(EXIT_FAILURE);
    } else {
        /* Parent */
        gchar *cmdline = NULL;
        for (char *const *p = argv; *p; ++p) {
            if (cmdline)
                cmdline = g_strjoin(" ", cmdline, *p, NULL);
            else
                cmdline = g_strdup(*p);
        }
        add_child(pid, name, cmdline);
        g_free(cmdline);
        return pid;
    }
}

/* Gracefully terminate a child then force-kill after timeout */
static void terminate_child_graceful(pid_t pid, int timeout_secs) {
    if (pid <= 0) return;
    jb_log("Terminating child %d", pid);
    kill(pid, SIGTERM);
    int waited = 0;
    int status;
    while (waitpid(pid, &status, WNOHANG) == 0 && waited < timeout_secs) {
        sleep(1);
        waited++;
    }
    if (waitpid(pid, &status, WNOHANG) == 0) {
        jb_log("Killing child %d after timeout", pid);
        kill(pid, SIGKILL);
    }
}

/* CONFIG LOADING */
static void load_default_config(jb_config_t *cfg) {
    cfg->a2dp_rate = 48000;
    cfg->a2dp_period = 1024;
    cfg->a2dp_nperiods = 3;
    cfg->a2dp_channels = 2;
    cfg->a2dp_drift_comp = 1;

    cfg->sco_rate = 16000;
    cfg->sco_period = 256;
    cfg->sco_nperiods = 3;
    cfg->sco_channels = 1;

    cfg->spawn_delay = 0;
    cfg->child_term_timeout = 4;
    cfg->log_file = g_strdup(DEFAULT_LOG);
    cfg->pid_file = g_strdup(DEFAULT_PID);
    cfg->runtime_user = g_strdup("jack");
    cfg->max_bridges = 8;
}

static gchar *trim(gchar *s) {
    if (!s) return NULL;
    gchar *start = s;
    while (*start && g_ascii_isspace(*start)) start++;
    gchar *end = start + strlen(start);
    while (end > start && g_ascii_isspace(*(end - 1))) end--;
    *end = '\0';
    return g_strdup(start);
}

/* Very simple KEY=VALUE parser; ignores quotes */
static gboolean load_config_from_file(const char *path, jb_config_t *cfg, GError **error) {
    FILE *f = fopen(path, "r");
    if (!f) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to open %s: %s", path, strerror(errno));
        return FALSE;
    }
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        /* strip newline */
        char *nl = strchr(s, '\n');
        if (nl) *nl = '\0';
        /* trim */
        gchar *t = trim(s);
        if (!t || t[0] == '#') { g_free(t); continue; }
        char *eq = strchr(t, '=');
        if (!eq) { g_free(t); continue; }
        *eq = '\0';
        gchar *k = trim(t);
        gchar *v = trim(eq + 1);
        if (!k || !v) { g_free(k); g_free(v); g_free(t); continue; }

        /* parse known keys */
        if (g_strcmp0(k, "A2DP_RATE") == 0) cfg->a2dp_rate = atoi(v);
        else if (g_strcmp0(k, "A2DP_PERIOD") == 0) cfg->a2dp_period = atoi(v);
        else if (g_strcmp0(k, "A2DP_NPERIODS") == 0) cfg->a2dp_nperiods = atoi(v);
        else if (g_strcmp0(k, "A2DP_CHANNELS") == 0) cfg->a2dp_channels = atoi(v);
        else if (g_strcmp0(k, "A2DP_DRIFT_COMP") == 0) cfg->a2dp_drift_comp = atoi(v);

        else if (g_strcmp0(k, "SCO_RATE") == 0) cfg->sco_rate = atoi(v);
        else if (g_strcmp0(k, "SCO_PERIOD") == 0) cfg->sco_period = atoi(v);
        else if (g_strcmp0(k, "SCO_NPERIODS") == 0) cfg->sco_nperiods = atoi(v);
        else if (g_strcmp0(k, "SCO_CHANNELS") == 0) cfg->sco_channels = atoi(v);

        else if (g_strcmp0(k, "SPAWN_DELAY") == 0) cfg->spawn_delay = atoi(v);
        else if (g_strcmp0(k, "CHILD_TERM_TIMEOUT") == 0) cfg->child_term_timeout = atoi(v);
        else if (g_strcmp0(k, "LOG_FILE") == 0) { g_free(cfg->log_file); cfg->log_file = g_strdup(v); }
        else if (g_strcmp0(k, "PID_FILE") == 0) { g_free(cfg->pid_file); cfg->pid_file = g_strdup(v); }
        else if (g_strcmp0(k, "RUNTIME_USER") == 0) { g_free(cfg->runtime_user); cfg->runtime_user = g_strdup(v); }
        else if (g_strcmp0(k, "MAX_BRIDGES") == 0) cfg->max_bridges = atoi(v);
        /* unrecognized keys are ignored for now */

        g_free(k);
        g_free(v);
        g_free(t);
    }
    fclose(f);
    return TRUE;
}

/* Apply config into globals and reopen log if changed */
static void apply_config(const jb_config_t *cfg) {
    if (log_path) g_free(log_path);
    log_path = g_strdup(cfg->log_file ? cfg->log_file : DEFAULT_LOG);
    if (pid_path) g_free(pid_path);
    pid_path = g_strdup(cfg->pid_file ? cfg->pid_file : DEFAULT_PID);
    reopen_log();
}

/* Reopen log file for appending */
static void reopen_log(void) {
    if (log_fp && log_fp != stderr) {
        fclose(log_fp);
        log_fp = NULL;
    }
    if (!log_path) log_path = g_strdup(DEFAULT_LOG);
    log_fp = fopen(log_path, "a");
    if (!log_fp) {
        /* best-effort fallback to stderr */
        log_fp = stderr;
        jb_log("Failed to open log file %s: %s", log_path, strerror(errno));
    }
}

/* Write pid file atomically */
static void write_pid_file(const char *path) {
    if (!path) return;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        jb_log("Failed to write pid file %s: %s", path, strerror(errno));
        return;
    }
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%d\n", getpid());
    write(fd, buf, n);
    close(fd);
}

/* Remove pid file */
static void remove_pid_file(const char *path) {
    if (!path) return;
    unlink(path);
}

/* Signal handlers */
static volatile sig_atomic_t got_hup = 0;
static volatile sig_atomic_t got_term = 0;

static void on_hup(int signo) {
    (void)signo;
    got_hup = 1;
    if (main_loop) g_main_loop_quit(main_loop); /* wake main loop iteration to handle */
}

static void on_term(int signo) {
    (void)signo;
    got_term = 1;
    if (main_loop) g_main_loop_quit(main_loop);
}

/* Install standard signal handlers */
static void install_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = on_hup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* SIGCHLD handled using periodic reaper in GLib main loop */
    signal(SIGCHLD, SIG_DFL);
}

/* Example helper to form bluealsa device argument for exec command */
static gchar *format_bluealsa_device_arg(const char *mac, const char *profile) {
    return g_strdup_printf("bluealsa:DEV=%s,PROFILE=%s", mac, profile);
}

/* Check whether an ALSA PCM specified by a bluealsa-style device string is available.
 * We attempt a non-blocking snd_pcm_open to detect whether another client already
 * has exclusive access or if the device can be opened. This is a heuristic: some
 * ALSA plugins allow multiple opens; but this helps avoid races where another
 * client (user-launched) already took the PCM.
 *
 * Returns TRUE if device seems available (we can open/close), FALSE if it cannot
 * be opened (in use) or on error.
 */
static gboolean is_alsa_device_available(const char *device_arg, snd_pcm_stream_t stream) {
    if (!device_arg) return FALSE;
    snd_pcm_t *handle = NULL;
    int err;

    /* Try non-blocking open; many ALSA plugins ignore non-blocking, but this
     * avoids hanging if device is busy. */
    err = snd_pcm_open(&handle, device_arg, stream, SND_PCM_NONBLOCK);
    if (err < 0) {
        /* Could not open - likely in use or unavailable */
        jb_log("ALSA open failed for %s: %s", device_arg, snd_strerror(err));
        return FALSE;
    }

    /* Successfully opened - close and indicate available */
    snd_pcm_close(handle);
    return TRUE;
}

/* Helper to normalize MAC-like strings from object path:
 * Expect object path like /org/bluealsa/hci0/dev_XX_XX_XX_XX_XX_XX/fd0
 * We extract the MAC-like part and replace underscores with colons.
 */
static gchar *extract_mac_from_object_path(const gchar *object_path) {
    if (!object_path) return NULL;
    /* Find 'dev_' segment */
    const gchar *p = strstr(object_path, "/dev_");
    if (!p) {
        /* Some bluealsa object paths might embed MAC as XX:XX... - try last path segment */
        const gchar *last = strrchr(object_path, '/');
        if (!last) return NULL;
        gchar *dup = g_strdup(last + 1);
        /* sanitize */
        for (gchar *q = dup; *q; ++q) if (*q == '_') *q = ':';
        return dup;
    }
    p += 5; /* skip "/dev_" */
    /* copy until next slash */
    const gchar *end = strchr(p, '/');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    gchar *mac = g_strndup(p, len);
    /* replace underscores with colons */
    for (gchar *q = mac; *q; ++q) if (*q == '_') *q = ':';
    return mac;
}

/* PLACEHOLDER: actual spawn functions should create argv arrays dynamically and free them.
   For clarity we create small temporary strings here and rely on exec to replace child. */

static void spawn_a2dp_sink_for(const char *mac) {
    /* Prevent duplicate spawn if a bridge for this MAC already exists */
    if (is_bridge_running_for_mac(mac)) {
        jb_log("spawn_a2dp_sink_for: bridge already running for %s, skipping", mac);
        return;
    }

    /* Build argv dynamically and free all temporaries after spawn to avoid leaks */
    gchar *job = g_strdup_printf("bt_in_%s", mac);
    gchar *device_arg = g_strdup_printf("bluealsa:DEV=%s,PROFILE=a2dp", mac);

    /* If the ALSA device cannot be opened now, skip spawning to avoid stepping
     * on an existing client (heuristic to prevent races).
     */
    if (!is_alsa_device_available(device_arg, SND_PCM_STREAM_CAPTURE)) {
        jb_log("spawn_a2dp_sink_for: ALSA PCM %s not available, skipping spawn", device_arg);
        g_free(device_arg);
        g_free(job);
        return;
    }

    gchar *rate_s = g_strdup_printf("%d", config.a2dp_rate);
    gchar *period_s = g_strdup_printf("%d", config.a2dp_period);
    gchar *n_s = g_strdup_printf("%d", config.a2dp_nperiods);
    gchar *ch_s = g_strdup_printf("%d", config.a2dp_channels);

    /* Assemble argv vector */
    char **argv = g_new0(char*, 16);
    int i = 0;
    argv[i++] = g_strdup("alsa_in");
    argv[i++] = g_strdup("-j");
    argv[i++] = g_strdup(job);
    argv[i++] = g_strdup("-d");
    argv[i++] = g_strdup(device_arg);
    argv[i++] = g_strdup("-r");
    argv[i++] = g_strdup(rate_s);
    argv[i++] = g_strdup("-p");
    argv[i++] = g_strdup(period_s);
    argv[i++] = g_strdup("-n");
    argv[i++] = g_strdup(n_s);
    argv[i++] = g_strdup("-c");
    argv[i++] = g_strdup(ch_s);
    argv[i] = NULL;

    /* Spawn */
    spawn_bridge(job, argv);

    /* Free allocated argv strings and containers */
    for (int j = 0; argv[j] != NULL; ++j) g_free(argv[j]);
    g_free(argv);

    g_free(job);
    g_free(device_arg);
    g_free(rate_s);
    g_free(period_s);
    g_free(n_s);
    g_free(ch_s);
}

static void spawn_a2dp_source_for(const char *mac) {
    /* Prevent duplicate spawn if a bridge for this MAC already exists */
    if (is_bridge_running_for_mac(mac)) {
        jb_log("spawn_a2dp_source_for: bridge already running for %s, skipping", mac);
        return;
    }

    gchar *job = g_strdup_printf("bt_out_%s", mac);
    gchar *device_arg = g_strdup_printf("bluealsa:DEV=%s,PROFILE=a2dp", mac);

    /* For playback (alsa_out) we try to open the PCM for playback to ensure it's available */
    if (!is_alsa_device_available(device_arg, SND_PCM_STREAM_PLAYBACK)) {
        jb_log("spawn_a2dp_source_for: ALSA PCM %s not available, skipping spawn", device_arg);
        g_free(device_arg);
        g_free(job);
        return;
    }

    gchar *rate_s = g_strdup_printf("%d", config.a2dp_rate);
    gchar *period_s = g_strdup_printf("%d", config.a2dp_period);
    gchar *n_s = g_strdup_printf("%d", config.a2dp_nperiods);
    gchar *ch_s = g_strdup_printf("%d", config.a2dp_channels);

    char **argv = g_new0(char*, 16);
    int i = 0;
    argv[i++] = g_strdup("alsa_out");
    argv[i++] = g_strdup("-j");
    argv[i++] = g_strdup(job);
    argv[i++] = g_strdup("-d");
    argv[i++] = g_strdup(device_arg);
    argv[i++] = g_strdup("-r");
    argv[i++] = g_strdup(rate_s);
    argv[i++] = g_strdup("-p");
    argv[i++] = g_strdup(period_s);
    argv[i++] = g_strdup("-n");
    argv[i++] = g_strdup(n_s);
    argv[i++] = g_strdup("-c");
    argv[i++] = g_strdup(ch_s);
    argv[i] = NULL;

    spawn_bridge(job, argv);

    for (int j = 0; argv[j] != NULL; ++j) g_free(argv[j]);
    g_free(argv);

    g_free(job);
    g_free(device_arg);
    g_free(rate_s);
    g_free(period_s);
    g_free(n_s);
    g_free(ch_s);
}

static void spawn_sco_for(const char *mac) {
    /* Prevent duplicate spawn if a bridge for this MAC already exists */
    if (is_bridge_running_for_mac(mac)) {
        jb_log("spawn_sco_for: bridge already running for %s, skipping", mac);
        return;
    }

    gchar *job = g_strdup_printf("bt_sco_%s", mac);
    gchar *device_arg = g_strdup_printf("bluealsa:DEV=%s,PROFILE=sco", mac);

    /* SCO uses capture/playback in different contexts; try a capture open first */
    if (!is_alsa_device_available(device_arg, SND_PCM_STREAM_CAPTURE)) {
        jb_log("spawn_sco_for: ALSA PCM %s not available (capture), skipping spawn", device_arg);
        g_free(device_arg);
        g_free(job);
        return;
    }

    gchar rate_s[16], period_s[16], n_s[16], ch_s[8];
    snprintf(rate_s, sizeof(rate_s), "%d", config.sco_rate);
    snprintf(period_s, sizeof(period_s), "%d", config.sco_period);
    snprintf(n_s, sizeof(n_s), "%d", config.sco_nperiods);
    snprintf(ch_s, sizeof(ch_s), "%d", config.sco_channels);

    char *argv[] = {
        "alsa_in",
        "-j", job,
        "-d", device_arg,
        "-r", rate_s,
        "-p", period_s,
        "-n", n_s,
        "-c", ch_s,
        NULL
    };

    spawn_bridge(job, argv);

    g_free(job);
    g_free(device_arg);
}

/* Placeholder: connect JACK ports via existing autoconnect script */
static void run_jack_autoconnect(void) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("/usr/lib/jack-bridge/jack-autoconnect", "/usr/lib/jack-bridge/jack-autoconnect", (char *)NULL);
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        add_child(pid, "jack-autoconnect", "/usr/lib/jack-bridge/jack-autoconnect");
    } else {
        jb_log("Failed to fork for jack-autoconnect: %s", strerror(errno));
    }
}

/* D-Bus helper: query org.freedesktop.DBus.Properties.GetAll on the object for org.bluealsa.PCM1
 * Returns TRUE if properties were successfully queried and filled into provided result vars.
 * Caller must g_variant_unref(result) if non-NULL result returned.
 */
static gboolean query_pcm_properties(const char *object_path, GVariant **result, GError **error) {
    if (!system_bus || !object_path) return FALSE;
    GError *err = NULL;
    GVariant *res = g_dbus_connection_call_sync(system_bus,
                                                "org.bluealsa",
                                                object_path,
                                                "org.freedesktop.DBus.Properties",
                                                "GetAll",
                                                g_variant_new("(s)", "org.bluealsa.PCM1"),
                                                G_VARIANT_TYPE("(a{sv})"),
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &err);
    if (!res) {
        if (error) *error = err;
        else if (err) g_error_free(err);
        return FALSE;
    }
    if (result) *result = res;
    else g_variant_unref(res);
    return TRUE;
}

/* Placeholders for D-Bus event handling when PCM appears/disappears */
static void on_bluealsa_pcm_added(const char *object_path) {
    gchar *mac = extract_mac_from_object_path(object_path);
    if (!mac) {
        jb_log("Could not extract MAC from object path: %s", object_path);
        return;
    }
    jb_log("PCM added for object %s -> mac %s", object_path, mac);

    /* Prevent duplicate bridges for the same MAC */
    if (is_bridge_running_for_mac(mac)) {
        jb_log("Bridge already running for MAC %s; ignoring new PCM %s", mac, object_path);
        g_free(mac);
        return;
    }

    /* Attempt to query properties via GetAll to determine profile and direction */
    GError *err = NULL;
    GVariant *props_wrapped = NULL;
    gboolean used_specific_spawn = FALSE;

    if (query_pcm_properties(object_path, &props_wrapped, &err)) {
        /* props_wrapped has signature (a{sv}) */
        GVariantIter *iter = NULL;
        GVariant *dict = NULL;
        g_variant_get(props_wrapped, "(a{sv})", &iter);
        gchar *key = NULL;
        GVariant *val = NULL;
        gchar *profile = NULL;
        gchar *direction = NULL;
        gchar *type = NULL;

        while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
            if (g_strcmp0(key, "Profile") == 0 && val && g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                profile = g_strdup(g_variant_get_string(val, NULL));
            } else if ((g_strcmp0(key, "Direction") == 0 || g_strcmp0(key, "Type") == 0) && val && g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                if (g_strcmp0(key, "Direction") == 0) direction = g_strdup(g_variant_get_string(val, NULL));
                else type = g_strdup(g_variant_get_string(val, NULL));
            }
            g_variant_unref(val);
            g_free(key);
        }
        g_variant_iter_free(iter);
        g_variant_unref(props_wrapped);

        jb_log("Parsed PCM properties for %s: Profile=%s Direction=%s Type=%s", mac,
               profile ? profile : "(nil)", direction ? direction : "(nil)", type ? type : "(nil)");

        /* Decide which spawn to call:
         * - profile contains "a2dp" + direction indicates whether remote is sink or source:
         *     - if direction == "source" -> remote is source (phone->us) => spawn_a2dp_sink_for
         *     - if direction == "sink"   -> remote is sink (us->headset) => spawn_a2dp_source_for
         * - profile contains "sco" or "hfp"/"hsp" -> spawn_sco_for
         * - fallback to heuristics below
         */
        if (profile && g_strrstr(profile, "a2dp")) {
            if (direction && g_strcmp0(direction, "source") == 0) {
                spawn_a2dp_sink_for(mac);
                used_specific_spawn = TRUE;
            } else if (direction && g_strcmp0(direction, "sink") == 0) {
                spawn_a2dp_source_for(mac);
                used_specific_spawn = TRUE;
            } else {
                /* If direction not provided, prefer sink (phone->us) */
                spawn_a2dp_sink_for(mac);
                used_specific_spawn = TRUE;
            }
        } else if ((profile && (g_strrstr(profile, "sco") || g_strrstr(profile, "hfp") || g_strrstr(profile, "hsp"))) ||
                   (type && (g_strrstr(type, "sco") || g_strrstr(type, "hfp") || g_strrstr(type, "hsp")))) {
            spawn_sco_for(mac);
            used_specific_spawn = TRUE;
        }

        g_free(profile);
        g_free(direction);
        g_free(type);
    } else {
        jb_log("Could not GetAll properties for %s: %s", object_path, err ? err->message : "(no error)");
        if (err) g_error_free(err);
    }

    if (!used_specific_spawn) {
        /* Fallback heuristic: if object path contains 'sco' then SCO, else default to A2DP sink */
        if (strstr(object_path, "sco") || strstr(object_path, "SCO")) {
            spawn_sco_for(mac);
        } else {
            spawn_a2dp_sink_for(mac);
        }
    }

    run_jack_autoconnect();
    g_free(mac);
}

static void on_bluealsa_pcm_removed(const char *object_path) {
    gchar *mac = extract_mac_from_object_path(object_path);
    if (!mac) {
        jb_log("Could not extract MAC from removed object path: %s", object_path);
        return;
    }
    jb_log("PCM removed for object %s -> mac %s", object_path, mac);
    /* terminate children matching mac */
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, children);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        child_t *c = value;
        if (c->name && strstr(c->name, mac)) {
            terminate_child_graceful(c->pid, config.child_term_timeout);
            remove_child(c->pid);
        }
    }
    g_free(mac);
}

/* D-Bus signal callbacks */

static void interfaces_added_cb(GDBusConnection *connection,
                                const gchar *sender_name,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters,
                                gpointer user_data) {
    (void)connection; (void)sender_name; (void)interface_name; (void)user_data;
    /* parameters: object_path, dict of interfaces -> dict of properties */
    gchar *added_path = NULL;
    GVariantIter *interfaces = NULL;
    const gchar *p = NULL;

    /* Unpack: (so we accept both ObjectManager.InterfacesAdded signature and generic signals)
       The ObjectManager.InterfacesAdded signature is (oa{sa{sv}}) */
    g_variant_get(parameters, "(oa{sa{sv}})", &added_path, &interfaces);
    jb_log("InterfacesAdded for %s", added_path);

    /* Check if org.bluealsa.PCM1 is among interfaces */
    gboolean found_pcm = FALSE;
    gchar *iface_name = NULL;
    GVariantIter *props = NULL;

    while (g_variant_iter_next(interfaces, "{sa{sv}}", &iface_name, &props)) {
        if (g_strcmp0(iface_name, "org.bluealsa.PCM1") == 0) {
            found_pcm = TRUE;
            /* Optionally inspect props to determine profile and direction */
        }
        if (props) g_variant_iter_free(props);
        g_free(iface_name);
    }
    g_variant_iter_free(interfaces);

    if (found_pcm) {
        on_bluealsa_pcm_added(added_path);
    }

    g_free(added_path);
}

static void interfaces_removed_cb(GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data) {
    (void)connection; (void)sender_name; (void)interface_name; (void)user_data;
    /* parameters: object_path, array of interface names */
    gchar *removed_path = NULL;
    GVariantIter *ifaces = NULL;
    g_variant_get(parameters, "(oao)", &removed_path, &ifaces);
    jb_log("InterfacesRemoved for %s", removed_path);

    /* iterate interface names and if org.bluealsa.PCM1 present, treat removal */
    gchar *iname = NULL;
    while (g_variant_iter_next(ifaces, "o", &iname)) {
        /* ifname may come as object-like; safer to just check if removed_path contains 'dev_' */
        /* The simple approach: if object_path contains '/dev_' then call removal */
        g_free(iname);
    }
    g_variant_iter_free(ifaces);

    /* heuristic: call removal handler */
    on_bluealsa_pcm_removed(removed_path);
    g_free(removed_path);
}

/* Reload configuration on SIGHUP */
static gboolean handle_reload(gpointer user_data) {
    GError *err = NULL;
    jb_config_t new_cfg;
    load_default_config(&new_cfg);
    if (!load_config_from_file(CONFIG_PATH, &new_cfg, &err)) {
        jb_log("Failed to reload config: %s", err ? err->message : "unknown");
        if (err) g_error_free(err);
        /* keep old config */
    } else {
        /* apply atomically */
        /* free old strings where appropriate */
        if (config.log_file) g_free(config.log_file);
        if (config.pid_file) g_free(config.pid_file);
        if (config.runtime_user) g_free(config.runtime_user);
        config = new_cfg;
        apply_config(&config);
        jb_log("Reloaded configuration from %s", CONFIG_PATH);
    }
    return G_SOURCE_REMOVE;
}

/* Periodic child reaper using GLib timeout */
static gboolean child_reaper_cb(gpointer user_data) {
    (void)user_data;
    handle_sigchld();
    return G_SOURCE_CONTINUE;
}

/* Subscribe to BlueALSA/DBus ObjectManager signals */
static gboolean subscribe_to_bluealsa_signals(GError **error) {
    /* Subscribe to org.freedesktop.DBus.ObjectManager InterfacesAdded and InterfacesRemoved
       on the system bus; bluealsa typically registers objects under /org/bluealsa/... */
    interfaces_added_sub = g_dbus_connection_signal_subscribe(system_bus,
                                                              NULL, /* sender - any (org.bluealsa?) */
                                                              "org.freedesktop.DBus.ObjectManager",
                                                              "InterfacesAdded",
                                                              NULL, /* object path - any */
                                                              NULL,
                                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                                              interfaces_added_cb,
                                                              NULL, NULL);
    interfaces_removed_sub = g_dbus_connection_signal_subscribe(system_bus,
                                                              NULL,
                                                              "org.freedesktop.DBus.ObjectManager",
                                                              "InterfacesRemoved",
                                                              NULL,
                                                              NULL,
                                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                                              interfaces_removed_cb,
                                                              NULL, NULL);
    jb_log("Subscribed to ObjectManager InterfacesAdded/Removed (bluealsa)");
    return TRUE;
}

/* Unsubscribe */
static void unsubscribe_bluealsa_signals(void) {
    if (interfaces_added_sub) {
        g_dbus_connection_signal_unsubscribe(system_bus, interfaces_added_sub);
        interfaces_added_sub = 0;
    }
    if (interfaces_removed_sub) {
        g_dbus_connection_signal_unsubscribe(system_bus, interfaces_removed_sub);
        interfaces_removed_sub = 0;
    }
}

/* Main initialization: load config, init bus, subscribe signals */
static gboolean init_services(GError **error) {
    GError *err = NULL;
    load_default_config(&config);
    if (!load_config_from_file(CONFIG_PATH, &config, &err)) {
        jb_log("Warning: failed to read config %s (%s); continuing with defaults", CONFIG_PATH, err ? err->message : "unknown");
        if (err) g_error_free(err);
    }
    apply_config(&config);

    /* write pid file */
    write_pid_file(config.pid_file);

    /* connect to system bus */
    system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
    if (!system_bus) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to connect to system bus: %s", err ? err->message : "unknown");
        if (err) g_error_free(err);
        return FALSE;
    }

    /* Subscribe to bluealsa signals */
    if (!subscribe_to_bluealsa_signals(error)) {
        jb_log("Warning: could not subscribe to bluealsa signals");
    }

    return TRUE;
}

int main(int argc, char **argv) {
    GError *err = NULL;
    /* children hash: key=pid pointer, value child_t*, use child_free to free child entries */
    children = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, child_free);

    /* Install minimal signal handlers */
    install_signal_handlers();

    /* Reopen log after parsing any command-line flags (none currently) */
    reopen_log();

    jb_log("jack-bluealsa-autobridge starting (pid=%d)", getpid());

    /* Initialize services (config, bus) */
    if (!init_services(&err)) {
        jb_log("Initialization failed: %s", err ? err->message : "unknown");
        if (err) g_error_free(err);
        return EXIT_FAILURE;
    }

    /* Install GLib periodic child reaper (every 1s) */
    g_timeout_add_seconds(1, child_reaper_cb, NULL);

    /* Main loop: exit on SIGHUP or SIGTERM; we handle reload via quitting loop and scheduling reload */
    main_loop = g_main_loop_new(NULL, FALSE);

    while (!got_term) {
        jb_log("Entering main loop");
        g_main_loop_run(main_loop);

        if (got_hup) {
            jb_log("SIGHUP received: reloading config");
            /* schedule reload on main context (not in signal handler) */
            g_main_context_invoke(NULL, handle_reload, NULL);
            got_hup = 0;
            /* continue main loop */
            continue;
        }

        if (got_term) break;
    }

    jb_log("Shutdown requested; terminating children");
    /* Terminate children gracefully */
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, children);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        child_t *c = value;
        if (c) terminate_child_graceful(c->pid, config.child_term_timeout);
    }

    /* Cleanup */
    unsubscribe_bluealsa_signals();
    if (system_bus) g_object_unref(system_bus);
    if (main_loop) g_main_loop_unref(main_loop);
    if (children) g_hash_table_destroy(children);
    remove_pid_file(config.pid_file);
    jb_log("Exiting");
    if (log_fp && log_fp != stderr) fclose(log_fp);
    return EXIT_SUCCESS;
}
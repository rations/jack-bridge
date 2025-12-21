/*
 * jack_bridge_dbus.c
 * D-Bus bridge service for qjackctl integration with jack-bridge
 *
 * Implements org.jackaudio.service D-Bus interface to bridge qjackctl's
 * expectations with jack-bridge's SysV init architecture.
 *
 * Phase A (MVP): JackControl interface (IsStarted, StartServer, StopServer)
 * Phase B: JackConfigure interface (settings management)
 * Phase C: Live updates and polish
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <gio/gio.h>

#include "jack_bridge_dbus_config.h"
#include "jack_bridge_settings_sync.h"
#include "jack_bridge_dbus.h"

/* Service configuration */
#define DBUS_SERVICE_NAME "org.jackaudio.service"
#define DBUS_OBJECT_PATH "/org/jackaudio/Controller"
#define JACKD_RT_PIDFILE "/var/run/jackd-rt.pid"
#define JACKD_RT_SERVICE "jackd-rt"

/* Global state */
static GMainLoop *main_loop = NULL;
static GDBusConnection *bus_connection = NULL;
static guint service_name_id = 0;
static guint monitor_timeout_id = 0;
static gboolean last_known_state = FALSE;
GMutex config_access_mutex;

/* Forward declarations */
static void emit_server_started(void);
static void emit_server_stopped(void);

/*
 * check_jack_running()
 * Check if jackd-rt service is running by examining pidfile
 */
static gboolean check_jack_running(void) {
    FILE *f;
    pid_t pid;
    
    f = fopen(JACKD_RT_PIDFILE, "r");
    if (!f) {
        return FALSE;
    }
    
    if (fscanf(f, "%d", &pid) != 1) {
        fclose(f);
        return FALSE;
    }
    fclose(f);
    
    /* Check if process exists */
    if (kill(pid, 0) == 0) {
        return TRUE;
    }
    
    return FALSE;
}

/*
 * monitor_jackd_state()
 * Periodic callback to monitor JACK state and emit signals
 * Runs every second (low overhead)
 */
static gboolean monitor_jackd_state(gpointer user_data) {
    (void)user_data;
    gboolean is_running = check_jack_running();
    
    /* Emit signals on state transitions */
    if (is_running && !last_known_state) {
        g_print("jack-bridge-dbus: JACK started (emitting ServerStarted)\n");
        emit_server_started();
    } else if (!is_running && last_known_state) {
        g_print("jack-bridge-dbus: JACK stopped (emitting ServerStopped)\n");
        emit_server_stopped();
    }
    
    last_known_state = is_running;
    return G_SOURCE_CONTINUE; /* Keep polling */
}

/*
 * handle_is_started()
 * D-Bus method: IsStarted() → boolean
 */
static void handle_is_started(GDBusConnection *connection,
                               const gchar *sender,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    (void)parameters;
    
    gboolean running = check_jack_running();
    g_print("jack-bridge-dbus: IsStarted() → %s\n", running ? "true" : "false");
    g_dbus_method_invocation_return_value(invocation, 
                                          g_variant_new("(b)", running));
}

/*
 * handle_start_server()
 * D-Bus method: StartServer() → void
 * Calls: service jackd-rt start
 */
static void handle_start_server(GDBusConnection *connection,
                                 const gchar *sender,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    (void)parameters;
    
    gint exit_status;
    GError *error = NULL;
    
    g_print("jack-bridge-dbus: StartServer() called\n");
    
    /* Check if already running */
    if (check_jack_running()) {
        g_print("jack-bridge-dbus: JACK already running\n");
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }
    
    /* Execute: service jackd-rt start */
    if (!g_spawn_command_line_sync("service jackd-rt start",
                                   NULL, NULL, &exit_status, &error)) {
        g_printerr("jack-bridge-dbus: Failed to start jackd-rt: %s\n",
                   error->message);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Failed to start JACK: %s",
                                              error->message);
        g_error_free(error);
        return;
    }
    
    /* Give JACK a brief moment to initialize before restarting dependent services */
    g_usleep(1500000); /* 1.5 second delay - much shorter than previous 8s attempt */
    
    /* Restart jack-bridge-ports first to spawn USB/HDMI ports */
    g_spawn_command_line_async("service jack-bridge-ports restart", NULL);
    g_print("jack-bridge-dbus: Restarted jack-bridge-ports for USB/HDMI ports\n");
    
    /* Small delay between service restarts to avoid resource contention */
    g_usleep(500000); /* 0.5 second delay */
    
    /* Restart jack-connection-manager to ensure audio routing is restored */
    g_spawn_command_line_async("service jack-connection-manager restart", NULL);
    g_print("jack-bridge-dbus: Restarted jack-connection-manager for proper audio routing\n");
    
    if (exit_status != 0) {
        g_printerr("jack-bridge-dbus: service jackd-rt start exited with %d\n",
                   exit_status);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "JACK service start failed (exit %d)",
                                              exit_status);
        return;
    }
    
    g_print("jack-bridge-dbus: JACK service started successfully\n");
    g_dbus_method_invocation_return_value(invocation, NULL);
}

/*
 * handle_stop_server()
 * D-Bus method: StopServer() → void
 * Calls: service jackd-rt stop
 */
static void handle_stop_server(GDBusConnection *connection,
                                const gchar *sender,
                                GVariant *parameters,
                                GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    (void)parameters;
    
    gint exit_status;
    GError *error = NULL;
    
    g_print("jack-bridge-dbus: StopServer() called\n");
    
    /* Check if already stopped */
    if (!check_jack_running()) {
        g_print("jack-bridge-dbus: JACK not running\n");
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }
    
    /* Execute: service jackd-rt stop */
    if (!g_spawn_command_line_sync("service jackd-rt stop",
                                   NULL, NULL, &exit_status, &error)) {
        g_printerr("jack-bridge-dbus: Failed to stop jackd-rt: %s\n",
                   error->message);
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Failed to stop JACK: %s",
                                              error->message);
        g_error_free(error);
        return;
    }
    
    if (exit_status != 0) {
        g_printerr("jack-bridge-dbus: service jackd-rt stop exited with %d\n",
                   exit_status);
        /* Don't fail - service might have already been stopped */
    }
    
    g_print("jack-bridge-dbus: JACK service stopped successfully\n");
    
    /* Stop dependent services asynchronously */
    g_print("jack-bridge-dbus: Stopping jack-connection-manager...\n");
    g_spawn_command_line_async("service jack-connection-manager stop", NULL);
    
    g_print("jack-bridge-dbus: Stopping jack-bridge-ports...\n");
    g_spawn_command_line_async("service jack-bridge-ports stop", NULL);
    
    g_dbus_method_invocation_return_value(invocation, NULL);
}

/*
 * handle_switch_master()
 * D-Bus method: SwitchMaster() → void
 * No-op for init service (qjackctl compatibility)
 */
static void handle_switch_master(GDBusConnection *connection,
                                  const gchar *sender,
                                  GVariant *parameters,
                                  GDBusMethodInvocation *invocation) {
    (void)connection;
    (void)sender;
    (void)parameters;
    
    g_print("jack-bridge-dbus: SwitchMaster() called (no-op for init service)\n");
    g_dbus_method_invocation_return_value(invocation, NULL);
}

/*
 * emit_server_started()
 * Emit ServerStarted signal
 */
static void emit_server_started(void) {
    GError *error = NULL;
    
    if (!bus_connection) return;
    
    if (!g_dbus_connection_emit_signal(bus_connection,
                                       NULL, /* destination */
                                       DBUS_OBJECT_PATH,
                                       "org.jackaudio.JackControl",
                                       "ServerStarted",
                                       NULL, /* no parameters */
                                       &error)) {
        g_printerr("jack-bridge-dbus: Failed to emit ServerStarted: %s\n",
                   error->message);
        g_error_free(error);
    }
}

/*
 * emit_server_stopped()
 * Emit ServerStopped signal
 */
static void emit_server_stopped(void) {
    GError *error = NULL;
    
    if (!bus_connection) return;
    
    if (!g_dbus_connection_emit_signal(bus_connection,
                                       NULL, /* destination */
                                       DBUS_OBJECT_PATH,
                                       "org.jackaudio.JackControl",
                                       "ServerStopped",
                                       NULL, /* no parameters */
                                       &error)) {
        g_printerr("jack-bridge-dbus: Failed to emit ServerStopped: %s\n",
                   error->message);
        g_error_free(error);
    }
}

/*
 * handle_method_call()
 * D-Bus method call dispatcher
 */
static void handle_method_call(GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *method_name,
                                GVariant *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data) {
    (void)object_path;
    (void)user_data;
    
    g_print("jack-bridge-dbus: Method call: %s.%s from %s\n",
            interface_name, method_name, sender);
    
    if (g_strcmp0(interface_name, "org.jackaudio.JackControl") == 0) {
        if (g_strcmp0(method_name, "IsStarted") == 0) {
            handle_is_started(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "StartServer") == 0) {
            handle_start_server(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "StopServer") == 0) {
            handle_stop_server(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "SwitchMaster") == 0) {
            handle_switch_master(connection, sender, parameters, invocation);
        } else {
            g_dbus_method_invocation_return_error(invocation,
                                                  G_DBUS_ERROR,
                                                  G_DBUS_ERROR_UNKNOWN_METHOD,
                                                  "Unknown method: %s",
                                                  method_name);
        }
    } else if (g_strcmp0(interface_name, "org.jackaudio.Configure") == 0) {
        if (g_strcmp0(method_name, "GetParameterValue") == 0) {
            handle_get_parameter_value(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "SetParameterValue") == 0) {
            handle_set_parameter_value(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "ResetParameterValue") == 0) {
            handle_reset_parameter_value(connection, sender, parameters, invocation);
        } else if (g_strcmp0(method_name, "GetParameterConstraint") == 0) {
            handle_get_parameter_constraint(connection, sender, parameters, invocation);
        } else {
            g_dbus_method_invocation_return_error(invocation,
                                                  G_DBUS_ERROR,
                                                  G_DBUS_ERROR_UNKNOWN_METHOD,
                                                  "Unknown method: %s",
                                                  method_name);
        }
    } else {
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_INTERFACE,
                                              "Unknown interface: %s",
                                              interface_name);
    }
}

/* D-Bus introspection data for JackControl and JackConfigure interfaces */
static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.jackaudio.JackControl'>"
    "    <method name='IsStarted'>"
    "      <arg type='b' name='started' direction='out'/>"
    "    </method>"
    "    <method name='StartServer'/>"
    "    <method name='StopServer'/>"
    "    <method name='SwitchMaster'/>"
    "    <signal name='ServerStarted'/>"
    "    <signal name='ServerStopped'/>"
    "  </interface>"
    "  <interface name='org.jackaudio.Configure'>"
    "    <method name='GetParameterValue'>"
    "      <arg type='as' name='path' direction='in'/>"
    "      <arg type='b' name='is_set' direction='out'/>"
    "      <arg type='v' name='default' direction='out'/>"
    "      <arg type='v' name='value' direction='out'/>"
    "    </method>"
    "    <method name='SetParameterValue'>"
    "      <arg type='as' name='path' direction='in'/>"
    "      <arg type='v' name='value' direction='in'/>"
    "    </method>"
    "    <method name='ResetParameterValue'>"
    "      <arg type='as' name='path' direction='in'/>"
    "    </method>"
    "    <method name='GetParameterConstraint'>"
    "      <arg type='as' name='path' direction='in'/>"
    "      <arg type='b' name='is_strict' direction='out'/>"
    "      <arg type='b' name='is_fake' direction='out'/>"
    "      <arg type='av' name='values' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL, /* get_property */
    NULL, /* set_property */
    { NULL } /* padding */
};

/*
 * on_bus_acquired()
 * Called when we acquire the bus connection
 */
static void on_bus_acquired(GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data) {
    (void)name;
    (void)user_data;
    
    GError *error = NULL;
    GDBusNodeInfo *introspection_data;
    guint registration_id;
    
    g_print("jack-bridge-dbus: Bus acquired\n");
    
    bus_connection = connection;
    
    /* Parse introspection XML */
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (!introspection_data) {
        g_printerr("jack-bridge-dbus: Failed to parse introspection XML: %s\n",
                   error->message);
        g_error_free(error);
        g_main_loop_quit(main_loop);
        return;
    }
    
    /* Register JackControl interface (interfaces[0]) */
    registration_id = g_dbus_connection_register_object(connection,
                                                        DBUS_OBJECT_PATH,
                                                        introspection_data->interfaces[0],
                                                        &interface_vtable,
                                                        NULL, /* user_data */
                                                        NULL, /* user_data_free_func */
                                                        &error);
    
    if (registration_id == 0) {
        g_printerr("jack-bridge-dbus: Failed to register JackControl interface: %s\n",
                   error->message);
        g_error_free(error);
        g_dbus_node_info_unref(introspection_data);
        g_main_loop_quit(main_loop);
        return;
    }
    
    g_print("jack-bridge-dbus: JackControl interface registered\n");
    
    /* Register JackConfigure interface (interfaces[1]) */
    registration_id = g_dbus_connection_register_object(connection,
                                                        DBUS_OBJECT_PATH,
                                                        introspection_data->interfaces[1],
                                                        &interface_vtable,
                                                        NULL, /* user_data */
                                                        NULL, /* user_data_free_func */
                                                        &error);
    
    g_dbus_node_info_unref(introspection_data);
    
    if (registration_id == 0) {
        g_printerr("jack-bridge-dbus: Failed to register JackConfigure interface: %s\n",
                   error->message);
        g_error_free(error);
        g_main_loop_quit(main_loop);
        return;
    }
    
    g_print("jack-bridge-dbus: JackConfigure interface registered\n");
    g_print("jack-bridge-dbus: Object fully registered at %s\n", DBUS_OBJECT_PATH);
    
    /* Start state monitoring */
    last_known_state = check_jack_running();
    monitor_timeout_id = g_timeout_add_seconds(1, monitor_jackd_state, NULL);
    
    g_print("jack-bridge-dbus: State monitoring started (poll interval: 1s)\n");
}

/*
 * on_name_acquired()
 * Called when we acquire the service name
 */
static void on_name_acquired(GDBusConnection *connection,
                              const gchar *name,
                              gpointer user_data) {
    (void)connection;
    (void)user_data;
    
    g_print("jack-bridge-dbus: Service name acquired: %s\n", name);
    g_print("jack-bridge-dbus: Ready to accept D-Bus calls\n");
}

/*
 * on_name_lost()
 * Called when we lose the service name
 */
static void on_name_lost(GDBusConnection *connection,
                          const gchar *name,
                          gpointer user_data) {
    (void)user_data;
    
    if (connection == NULL) {
        g_printerr("jack-bridge-dbus: Failed to acquire bus connection\n");
    } else {
        g_printerr("jack-bridge-dbus: Lost service name: %s\n", name);
    }
    
    g_main_loop_quit(main_loop);
}

/*
 * signal_handler()
 * Handle termination signals
 */
static void signal_handler(int signum) {
    (void)signum;
    
    g_print("\njack-bridge-dbus: Received termination signal, shutting down\n");
    
    if (main_loop) {
        g_main_loop_quit(main_loop);
    }
}

/*
 * main()
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    g_print("jack-bridge-dbus: Starting D-Bus bridge service\n");
    g_print("jack-bridge-dbus: Service: %s\n", DBUS_SERVICE_NAME);
    g_print("jack-bridge-dbus: Object: %s\n", DBUS_OBJECT_PATH);

    /* Initialize config access mutex */
    g_mutex_init(&config_access_mutex);
    g_print("jack-bridge-dbus: Config access mutex initialized\n");

    /* Initialize configuration cache */
    init_config_cache();
    g_print("jack-bridge-dbus: Configuration cache initialized\n");

    /* Add a small delay to ensure the configuration cache is fully initialized */
    g_usleep(500000); /* 0.5 second delay */
    g_print("jack-bridge-dbus: Configuration cache ready\n");
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Create main loop */
    main_loop = g_main_loop_new(NULL, FALSE);
    
    /* Acquire service name on system bus */
    service_name_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                     DBUS_SERVICE_NAME,
                                     G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                     on_bus_acquired,
                                     on_name_acquired,
                                     on_name_lost,
                                     NULL, /* user_data */
                                     NULL); /* user_data_free_func */
    
    /* Run main loop */
    g_main_loop_run(main_loop);
    
    /* Cleanup */
    g_print("jack-bridge-dbus: Cleaning up\n");
    
    if (monitor_timeout_id > 0) {
        g_source_remove(monitor_timeout_id);
    }
    
    if (service_name_id > 0) {
        g_bus_unown_name(service_name_id);
    }
    
    if (main_loop) {
        g_main_loop_unref(main_loop);
    }
    
    g_print("jack-bridge-dbus: Exited\n");
    return 0;
}
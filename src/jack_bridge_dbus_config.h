/*
 * jack_bridge_dbus_config.h
 * JackConfigure interface implementation
 */

#ifndef JACK_BRIDGE_DBUS_CONFIG_H
#define JACK_BRIDGE_DBUS_CONFIG_H

#include <gio/gio.h>

/* Parameter types */
typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL
} ParamType;

/* D-Bus method handlers for JackConfigure interface */
void handle_get_parameter_value(GDBusConnection *connection,
                                 const gchar *sender,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation);

void handle_set_parameter_value(GDBusConnection *connection,
                                 const gchar *sender,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation);

void handle_reset_parameter_value(GDBusConnection *connection,
                                   const gchar *sender,
                                   GVariant *parameters,
                                   GDBusMethodInvocation *invocation);

void handle_get_parameter_constraint(GDBusConnection *connection,
                                      const gchar *sender,
                                      GVariant *parameters,
                                      GDBusMethodInvocation *invocation);

#endif /* JACK_BRIDGE_DBUS_CONFIG_H */
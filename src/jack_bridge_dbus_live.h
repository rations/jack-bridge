/*
 * jack_bridge_dbus_live.h
 * Live JACK parameter updates without full restart
 */

#ifndef JACK_BRIDGE_DBUS_LIVE_H
#define JACK_BRIDGE_DBUS_LIVE_H

#include <glib.h>

/* Try to change buffer size live (without restart)
 * Returns: 0=success, 1=failed (restart needed), 2=JACK not running */
gint try_live_buffer_size_change(guint32 new_period);

/* Check if parameter change requires restart */
gboolean check_needs_restart(const gchar *param_name);

/* Get user-friendly restart message */
const gchar *get_restart_message(const gchar *param_name);

#endif /* JACK_BRIDGE_DBUS_LIVE_H */
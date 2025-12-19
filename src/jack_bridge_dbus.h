/*
 * jack_bridge_dbus.h
 * D-Bus bridge service for qjackctl integration with jack-bridge
 *
 * Implements org.jackaudio.service D-Bus interface to bridge qjackctl's
 * expectations with jack-bridge's SysV init architecture.
 *
 * Phase A (MVP): JackControl interface (IsStarted, StartServer, StopServer)
 * Phase B: JackConfigure interface (settings management)
 * Phase C: Live updates and polish
 */

#ifndef JACK_BRIDGE_DBUS_H
#define JACK_BRIDGE_DBUS_H

#include <glib.h>

/* External mutex for config access synchronization */
extern GMutex config_access_mutex;

#endif /* JACK_BRIDGE_DBUS_H */
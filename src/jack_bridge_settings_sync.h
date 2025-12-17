/*
 * jack_bridge_settings_sync.h
 * Settings file I/O for jack-bridge D-Bus service
 */

#ifndef JACK_BRIDGE_SETTINGS_SYNC_H
#define JACK_BRIDGE_SETTINGS_SYNC_H

#include <glib.h>

/* Configuration file parsing */
GHashTable *parse_config_file(const char *path);

/* Read operations */
gchar *get_config_value(const char *key);
gint get_config_int(const char *key, gint default_value);

/* Write operations (atomic) */
gboolean set_config_value(const char *key, const char *value);
gboolean set_config_int(const char *key, gint value);

/* Validation functions */
gboolean validate_sample_rate(guint rate);
gboolean validate_period(guint period);
gboolean validate_nperiods(guint nperiods);
gboolean validate_priority(guint priority);

/* Utility functions */
gdouble calculate_latency_ms(guint period, guint nperiods, guint sample_rate);

/* Initialize configuration cache (call once at startup) */
void init_config_cache(void);

/* Refresh configuration cache (call after external changes) */
void refresh_config_cache(void);

#endif /* JACK_BRIDGE_SETTINGS_SYNC_H */
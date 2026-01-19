#ifndef CONFIG_H
#define CONFIG_H

/* Define to the title of this package. */
#define PROJECT_TITLE "QjackCtl"

/* Define to the name of this package. */
#define PROJECT_NAME "qjackctl"

/* Define to the version of this package. */
#define PROJECT_VERSION "1.0.4"

/* Define to the description of this package. */
#define PROJECT_DESCRIPTION "JACK Audio Connection Kit - Qt GUI Interface"

/* Define to the homepage of this package. */
#define PROJECT_HOMEPAGE_URL "https://qjackctl.sourceforge.io"

/* Define to the copyright of this package. */
#define PROJECT_COPYRIGHT "Copyright (C) 2003-2025, rncbc aka Rui Nuno Capela. All rights reserved."

/* Define to the domain of this package. */
#define PROJECT_DOMAIN "rncbc.org"


/* Default installation prefix. */
#define CONFIG_PREFIX "/usr"

/* Define to target installation dirs. */
#define CONFIG_BINDIR "/usr/bin"
#define CONFIG_LIBDIR "/usr/lib64"
#define CONFIG_DATADIR "/usr/share"
#define CONFIG_MANDIR "/usr/share/man"

/* Define if debugging is enabled. */
/* #undef CONFIG_DEBUG */

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define if JACK library is available. */
#define CONFIG_JACK 1

/* Define if ALSA library is available. */
#define CONFIG_ALSA_SEQ 1

/* Define if PORTAUDIO library is available. */
/* #undef CONFIG_PORTAUDIO */

/* Define if jack/statistics.h is available. */
#define CONFIG_JACK_STATISTICS 1

/* Define if CoreAudio/CoreAudio.h is available (Mac OS X). */
/* #undef CONFIG_COREAUDIO */

/* Define if JACK session support is available. */
#define CONFIG_JACK_SESSION ON

/* Define if JACK metadata support is available. */
#define CONFIG_JACK_METADATA ON

/* Define if JACK MIDI support is available. */
#define CONFIG_JACK_MIDI ON

/* Define if JACK CV support is available. */
#define CONFIG_JACK_CV ON

/* Define if JACK OSC support is available. */
#define CONFIG_JACK_OSC ON

/* Define if D-Bus interface is enabled. */
#define CONFIG_DBUS ON

/* Define if unique/single instance is enabled. */
#define CONFIG_XUNIQUE ON

/* Define if debugger stack-trace is enabled. */
/* #undef CONFIG_STACKTRACE */

/* Define if system tray is enabled. */
#define CONFIG_SYSTEM_TRAY ON

/* Define if jack_tranport_query is available. */
#define CONFIG_JACK_TRANSPORT 1

/* Define if jack_is_realtime is available. */
#define CONFIG_JACK_REALTIME 1

/* Define if jack_get_xrun_delayed_usecs is available. */
#define CONFIG_JACK_XRUN_DELAY 1

/* Define if jack_get_max_delayed_usecs is available. */
#define CONFIG_JACK_MAX_DELAY 1

/* Define if jack_set_port_rename_callback is available. */
#define CONFIG_JACK_PORT_RENAME 1

/* Define if jack_port_get_aliases is available. */
#define CONFIG_JACK_PORT_ALIASES ON

/* Define if jack_get_version_string is available. */
/* #undef CONFIG_JACK_VERSION */

/* Define if jack_free is available. */
#define CONFIG_JACK_FREE 1

/* Define if Wayland is supported */
/* #undef CONFIG_WAYLAND */


#endif /* CONFIG_H */

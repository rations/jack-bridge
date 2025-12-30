# Makefile - build binaries for jack-bridge project
# Usage:
#   make        # builds mxeq (GUI), jack-connection-manager, and jack-bridge-dbus
#   make mxeq   # build GUI binary only
#   make manager # build connection manager only
#   make dbus   # build D-Bus service only
#   make clean
CC = gcc
PKG_CONFIG = pkg-config
MKDIR_P = mkdir -p

BIN_DIR = contrib/bin

# Build ALSA -> JACK LD_PRELOAD shim (libalsa-jack-redirect.so)
SHIM_TARGET = $(BIN_DIR)/libalsa-jack-redirect.so
SHIM_SRCS = src/alsa_jack_redirect.c
SHIM_LIBS = -ldl
SHIM_CFLAGS = -fPIC

# Build mxeq (GUI) - needs GTK3, GLib/GIO and ALSA
MOTR_TARGET = $(BIN_DIR)/mxeq
MOTR_SRCS = src/mxeq.c src/gui_bt.c src/bt_agent.c
MOTR_PKGS = gtk+-3.0 glib-2.0 gio-2.0 alsa
MOTR_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(MOTR_PKGS))
MOTR_LIBS   = $(shell $(PKG_CONFIG) --libs $(MOTR_PKGS))

# Build jack-connection-manager (event-driven daemon) - only needs JACK
MANAGER_TARGET = $(BIN_DIR)/jack-connection-manager
MANAGER_SRCS = src/jack_connection_manager.c
MANAGER_LIBS = -ljack
MANAGER_CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11

# Build jack-bridge-dbus (D-Bus service for qjackctl integration)
# Includes settings sync, configuration management, and live update modules
DBUS_TARGET = $(BIN_DIR)/jack-bridge-dbus
DBUS_SRCS = src/jack_bridge_dbus.c \
            src/jack_bridge_dbus_config.c \
            src/jack_bridge_settings_sync.c \
            src/jack_bridge_dbus_live.c
DBUS_PKGS = glib-2.0 gio-2.0
DBUS_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(DBUS_PKGS)) -D_POSIX_C_SOURCE=200809L
DBUS_LIBS = $(shell $(PKG_CONFIG) --libs $(DBUS_PKGS)) -ljack

CFLAGS_COMMON = -Wall -Wextra -std=c11

all: mxeq manager dbus shim

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

shim: $(BIN_DIR) $(SHIM_TARGET)

$(SHIM_TARGET): $(SHIM_SRCS) | $(BIN_DIR)
	$(CC) -shared $(CFLAGS_COMMON) $(SHIM_CFLAGS) -o $@ $(SHIM_SRCS) $(SHIM_LIBS)

mxeq: $(BIN_DIR) $(MOTR_TARGET)

$(MOTR_TARGET): $(MOTR_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(MOTR_CFLAGS) -o $@ $(MOTR_SRCS) $(MOTR_LIBS)

manager: $(BIN_DIR) $(MANAGER_TARGET)

$(MANAGER_TARGET): $(MANAGER_SRCS) | $(BIN_DIR)
	$(CC) $(MANAGER_CFLAGS) -o $@ $(MANAGER_SRCS) $(MANAGER_LIBS)

dbus: $(BIN_DIR) $(DBUS_TARGET)

$(DBUS_TARGET): $(DBUS_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(DBUS_CFLAGS) -o $@ $(DBUS_SRCS) $(DBUS_LIBS)

clean:
	rm -f $(BIN_DIR)/mxeq $(BIN_DIR)/jack-connection-manager $(BIN_DIR)/jack-bridge-dbus $(BIN_DIR)/libalsa-jack-redirect.so

.PHONY: all clean mxeq manager dbus shim

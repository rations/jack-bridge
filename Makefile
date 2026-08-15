# Makefile - build binaries for jack-bridge project
# Usage:
#   make        # builds mxeq (GUI), jack-connection-manager, and pulse-jack-bridge
#   make mxeq   # build GUI binary only
#   make manager # build connection manager only
#   make bridge # build pulse-jack-bridge only
#   make clean
CC = gcc
PKG_CONFIG = pkg-config
MKDIR_P = mkdir -p

BIN_DIR = contrib/bin

# Build mxeq (GUI) - needs GTK3, GLib/GIO and ALSA
MOTR_TARGET = $(BIN_DIR)/mxeq
MOTR_SRCS = src/mxeq.c src/gui_bt.c src/bt_agent.c
MOTR_PKGS = gtk+-3.0 glib-2.0 gio-2.0 alsa

# Cap the GLib API level so release binaries stay runnable on older distros.
# Without this, building on GLib >= 2.76 makes g_string_free(s, FALSE) expand to
# g_string_free_and_steal(), a symbol absent from GLib 2.74 (Debian 12 / Devuan
# daedalus), so the binary dies at startup with an undefined-symbol error.
GLIB_API_LEVEL = -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 \
                 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74

MOTR_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(MOTR_PKGS)) $(GLIB_API_LEVEL)
MOTR_LIBS   = $(shell $(PKG_CONFIG) --libs $(MOTR_PKGS))

# Build jack-connection-manager (event-driven daemon) - only needs JACK
MANAGER_TARGET = $(BIN_DIR)/jack-connection-manager
MANAGER_SRCS = src/jack_connection_manager.c
MANAGER_LIBS = -ljack
MANAGER_CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11

# Build pulse-jack-bridge (PA protocol server → JACK)
# Requires: libjack-jackd2-dev (build-time only)
BRIDGE_TARGET = $(BIN_DIR)/pulse-jack-bridge
BRIDGE_SRCS = src/pulse_jack_bridge.c
BRIDGE_CFLAGS = -O2 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 \
                $(shell $(PKG_CONFIG) --cflags jack)
BRIDGE_LIBS   = $(shell $(PKG_CONFIG) --libs jack)

CFLAGS_COMMON = -Wall -Wextra -std=c11

all: mxeq manager bridge

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

mxeq: $(BIN_DIR) $(MOTR_TARGET)

$(MOTR_TARGET): $(MOTR_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(MOTR_CFLAGS) -o $@ $(MOTR_SRCS) $(MOTR_LIBS)

manager: $(BIN_DIR) $(MANAGER_TARGET)

$(MANAGER_TARGET): $(MANAGER_SRCS) | $(BIN_DIR)
	$(CC) $(MANAGER_CFLAGS) -o $@ $(MANAGER_SRCS) $(MANAGER_LIBS)

bridge: $(BIN_DIR) $(BRIDGE_TARGET)

$(BRIDGE_TARGET): $(BRIDGE_SRCS) | $(BIN_DIR)
	$(CC) $(BRIDGE_CFLAGS) -o $@ $(BRIDGE_SRCS) $(BRIDGE_LIBS)

clean:
	rm -f $(BIN_DIR)/mxeq $(BIN_DIR)/jack-connection-manager $(BIN_DIR)/pulse-jack-bridge

.PHONY: all clean mxeq manager bridge

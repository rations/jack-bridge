# Makefile - build separate binaries for jack-bridge project
# Usage:
#   make            # builds both mxeq and jack-bluealsa-autobridge
#   make mxeq       # build GUI binary only
#   make autobridge # build autobridge only
#   make clean
CC = gcc
PKG_CONFIG = pkg-config
MKDIR_P = mkdir -p

BIN_DIR = contrib/bin

# Build mxeq (GUI) - needs GTK3, GLib/GIO and ALSA
MOTR_TARGET = $(BIN_DIR)/mxeq
MOTR_SRCS = src/mxeq.c src/gui_bt.c src/bt_agent.c
MOTR_PKGS = gtk+-3.0 glib-2.0 gio-2.0 alsa
MOTR_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(MOTR_PKGS))
MOTR_LIBS   = $(shell $(PKG_CONFIG) --libs $(MOTR_PKGS))

# Build jack-bluealsa-autobridge - needs GLib/GIO and ALSA
AUTOB_TARGET = $(BIN_DIR)/jack-bluealsa-autobridge
AUTOB_SRCS = src/jack-bluealsa-autobridge.c
AUTOB_PKGS = glib-2.0 gio-2.0 alsa
AUTOB_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(AUTOB_PKGS))
AUTOB_LIBS   = $(shell $(PKG_CONFIG) --libs $(AUTOB_PKGS))

CFLAGS_COMMON = -Wall -Wextra -std=c11

all: mxeq autobridge

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

mxeq: $(BIN_DIR) $(MOTR_TARGET)

$(MOTR_TARGET): $(MOTR_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(MOTR_CFLAGS) -o $@ $(MOTR_SRCS) $(MOTR_LIBS)

autobridge: $(BIN_DIR) $(AUTOB_TARGET)

$(AUTOB_TARGET): $(AUTOB_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(AUTOB_CFLAGS) -o $@ $(AUTOB_SRCS) $(AUTOB_LIBS)

clean:
	rm -f $(BIN_DIR)/mxeq $(BIN_DIR)/jack-bluealsa-autobridge

.PHONY: all clean mxeq autobridge
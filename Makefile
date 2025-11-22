# Makefile - build binaries for jack-bridge project
# Usage:
#   make        # builds mxeq (GUI)
#   make mxeq   # build GUI binary only
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

CFLAGS_COMMON = -Wall -Wextra -std=c11

all: mxeq

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

mxeq: $(BIN_DIR) $(MOTR_TARGET)

$(MOTR_TARGET): $(MOTR_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS_COMMON) $(MOTR_CFLAGS) -o $@ $(MOTR_SRCS) $(MOTR_LIBS)

clean:
	rm -f $(BIN_DIR)/mxeq

.PHONY: all clean mxeq
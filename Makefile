# jack-bridge
#
# Five binaries out of one tree:
#
#   mxeq                      the mixer / devices / recorder / Bluetooth / Steam panel
#   jack-graph                the JACK connection graph and its JACK Settings window
#   jack-connection-manager   the auto-routing daemon (no GUI, no toolkit, never had one)
#   pulse-jack-bridge         the Steam PulseAudio bridge (likewise)
#   tools/uirender            the offline layout audit -- no X server, no sound card
#
# X11 + Cairo + FreeType and nothing else. NO GTK, NO GDK, NO PANGO, NO GOBJECT, NO GIO, NO GLIB.
# C++17 for everything that draws; the two daemons stay plain C11 and their flags are untouched.
#
# THE GFX LAYER DELIBERATELY DOES NOT LINK X11. canvas, fontstack, widgets, combo, textfield,
# listview and BOTH PANELS need cairo and nothing more, which is what lets tools/uirender compose
# and audit the real layouts with no X server running. Keep it that way: an #include of Xlib.h
# under src/gfx/, src/mxeq/panel.cpp or jack-graph/src/graphpanel.cpp costs the headless audit, and
# the audit is the only thing standing between a relaid-out panel and a user discovering
# "IEC958 (S/PD..." on a machine with different fonts.
#
# ONE COPY OF THE TOOLKIT, TWO BINARIES. jack-graph has no Makefile of its own any more. Two
# copies of canvas.cpp in one repository would be a second implementation for uirender to fail to
# audit.

CC      ?= gcc
CXX     ?= g++
PKG_CONFIG ?= pkg-config
MKDIR_P = mkdir -p

BIN_DIR = contrib/bin

# The prefix is COMPILED INTO the binaries: JACKBRIDGE_RESOURCE_DIR_DEFAULT is where the bundled
# fonts are looked for first. A binary is correct at the prefix it was built for; respath.cpp falls
# back to a build tree so an uninstalled binary still finds its fonts, and to
# $JACKBRIDGE_RESOURCE_DIR for packaging.
PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
SHAREDIR    ?= $(PREFIX)/share/jack-bridge

# --- packages, each verified present with pkg-config before use ---------------------------------
# Verified on this machine: cairo 1.18.4, cairo-ft 1.18.4, cairo-xlib 1.18.4, freetype2 26.2.20,
# x11 1.8.12, dbus-1 1.16.2, alsa 1.2.14, jack 1.9.22.
GFX_PKGS  = cairo cairo-ft freetype2
X11_PKGS  = cairo-xlib x11
DBUS_PKGS = dbus-1
ALSA_PKGS = alsa
JACK_PKGS = jack

GFX_CFLAGS  := $(shell $(PKG_CONFIG) --cflags $(GFX_PKGS))
GFX_LIBS    := $(shell $(PKG_CONFIG) --libs   $(GFX_PKGS))
X11_CFLAGS  := $(shell $(PKG_CONFIG) --cflags $(X11_PKGS))
X11_LIBS    := $(shell $(PKG_CONFIG) --libs   $(X11_PKGS))
DBUS_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(DBUS_PKGS))
DBUS_LIBS   := $(shell $(PKG_CONFIG) --libs   $(DBUS_PKGS))
ALSA_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(ALSA_PKGS))
ALSA_LIBS   := $(shell $(PKG_CONFIG) --libs   $(ALSA_PKGS))
JACK_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(JACK_PKGS))
JACK_LIBS   := $(shell $(PKG_CONFIG) --libs   $(JACK_PKGS))

# --- warnings and hardening --------------------------------------------------------------------
# -Werror is not negotiable: this tree vendors no upstream source, so every warning is ours.
WARN     = -Wall -Wextra -Werror -Wformat=2 -Wformat-security
HARDEN   = -O2 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full \
           -D_FORTIFY_SOURCE=3 -fPIE
LDHARDEN = -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

DEFS = -DJACKBRIDGE_RESOURCE_DIR_DEFAULT=\"$(SHAREDIR)\"

# -MMD -MP writes a .d file beside each .o listing the headers it included, so editing a header
# rebuilds everything that includes it. Without this, `make` after a header-only edit reports
# success against stale objects -- and this tree is full of header-only geometry with
# static_asserts in it, which is exactly the kind of edit that would be silently skipped.
DEPFLAGS = -MMD -MP

COMMON     = $(WARN) $(HARDEN) $(DEFS) $(DEPFLAGS) -Isrc
CXXFLAGS  += -std=c++17 $(COMMON)

# --- objects -----------------------------------------------------------------------------------
# GFX_OBJS is everything tools/uirender can link: cairo only, no X11.
GFX_OBJS = src/gfx/canvas.o src/gfx/fontstack.o src/gfx/widgets.o src/gfx/combo.o \
           src/gfx/textfield.o src/gfx/listview.o

# The only place X11 lives.
PLAT_OBJS = src/platform/xerror.o src/platform/respath.o src/platform/fs.o \
            src/platform/proc.o src/platform/childreaper.o src/platform/x11window.o

.PHONY: all clean toolkit manager bridge mxeq graph install uninstall

# `toolkit` is the shared layer on its own, and it is what keeps this tree green between the phases
# of the port.
all: toolkit mxeq manager bridge graph

toolkit: $(GFX_OBJS) $(PLAT_OBJS)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

# gfx objects: cairo only. NO X11_CFLAGS here, deliberately -- see the header comment.
src/gfx/%.o: src/gfx/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) -c -o $@ $<

src/platform/%.o: src/platform/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(X11_CFLAGS) -c -o $@ $<

# --- the two daemons: never linked a toolkit, flags unchanged ----------------------------------
MANAGER_TARGET = $(BIN_DIR)/jack-connection-manager
MANAGER_SRCS   = src/jack_connection_manager.c
MANAGER_CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11
MANAGER_LIBS   = -ljack

BRIDGE_TARGET = $(BIN_DIR)/pulse-jack-bridge
BRIDGE_SRCS   = src/pulse_jack_bridge.c
BRIDGE_CFLAGS = -O2 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 $(JACK_CFLAGS)
BRIDGE_LIBS   = $(JACK_LIBS)

manager: $(MANAGER_TARGET)
$(MANAGER_TARGET): $(MANAGER_SRCS) | $(BIN_DIR)
	$(CC) $(MANAGER_CFLAGS) -o $@ $(MANAGER_SRCS) $(MANAGER_LIBS)

bridge: $(BRIDGE_TARGET)
$(BRIDGE_TARGET): $(BRIDGE_SRCS) | $(BIN_DIR)
	$(CC) $(BRIDGE_CFLAGS) -o $@ $(BRIDGE_SRCS) $(BRIDGE_LIBS)

# --- STILL GTK, BEING REPLACED PHASE BY PHASE --------------------------------------------------
#
# These two rules are the OLD build and they are kept deliberately, not by oversight. The port
# lands in phases -- the shared toolkit, then mxeq, then jack-graph, then the settings window --
# and each phase has to leave a tree that builds and runs. Deleting these before src/mxeq/ and
# jack-graph/src/graphpanel.cpp exist would mean a run of commits with no working panel in them,
# which is also a run of commits nobody can bisect.
#
# They are also what Phase 3 is built against: porting the BlueZ client from GDBus to libdbus-1 is
# done against the still-GTK mxeq on its own branch, so that a failed pairing cannot be ambiguous
# between the D-Bus rewrite and the toolkit swap.
#
# WHAT GOES WHEN EACH PHASE LANDS: the mxeq rule, MOTR_* and GLIB_API_LEVEL go with Phase 2; the
# graph rule and jack-graph/Makefile go with Phase 4.
MOTR_TARGET = $(BIN_DIR)/mxeq
MOTR_SRCS   = src/mxeq.c src/gui_bt.c src/bt_agent.c
MOTR_PKGS   = gtk+-3.0 glib-2.0 gio-2.0 alsa

# Cap the GLib API level so release binaries stay runnable on older distros. Without this, building
# on GLib >= 2.76 makes g_string_free(s, FALSE) expand to g_string_free_and_steal(), a symbol
# absent from GLib 2.74 (Debian 12 / Devuan daedalus), so the binary dies at startup with an
# undefined-symbol error. GOES AWAY ENTIRELY with Phase 2: there is no GLib to cap.
GLIB_API_LEVEL = -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 \
                 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74

MOTR_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(MOTR_PKGS)) $(GLIB_API_LEVEL)
MOTR_LIBS   = $(shell $(PKG_CONFIG) --libs $(MOTR_PKGS))

GRAPH_DIR    = jack-graph
GRAPH_TARGET = $(BIN_DIR)/jack-graph

mxeq: $(MOTR_TARGET)
$(MOTR_TARGET): $(MOTR_SRCS) | $(BIN_DIR)
	$(CC) -Wall -Wextra -std=c11 $(MOTR_CFLAGS) -o $@ $(MOTR_SRCS) $(MOTR_LIBS)

# Phony, and named `graph` rather than `jack-graph`: a target named after the jack-graph/ directory
# would be considered up to date the moment that directory exists, and the copy would silently never
# run. Phony also means the staging step cannot be skipped just because contrib/bin/jack-graph is
# present -- a stale binary sitting there is exactly how a release once shipped a build that would
# not run on the oldest supported distro.
graph: | $(BIN_DIR)
	$(MAKE) -C $(GRAPH_DIR)
	install -m 0755 $(GRAPH_DIR)/jack-graph $(GRAPH_TARGET)
	@echo "Staged $(GRAPH_TARGET)"

# Generated by -MMD; absent on the first build, which `-include` tolerates silently.
DEPS = $(GFX_OBJS:.o=.d) $(PLAT_OBJS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(GFX_OBJS) $(PLAT_OBJS) $(DEPS)
	rm -f $(MANAGER_TARGET) $(BRIDGE_TARGET) $(MOTR_TARGET) $(GRAPH_TARGET)
	$(MAKE) -C $(GRAPH_DIR) clean

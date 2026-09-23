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

# mxeq's model layer: the sound card, the output device, the recorder, the Steam bridge and the
# Bluetooth device list. No X11 and no drawing, so tools/uirender can drive the real models.
MXEQ_MODEL_OBJS = src/mxeq/alsamixer.o src/mxeq/devices.o src/mxeq/recorder.o \
                  src/mxeq/steam.o src/mxeq/btmodel.o

# mxeq's view: the five pages and the application that drives them. panel.o is in here rather than
# with the models because it is still cairo-only -- it is what tools/uirender links.
MXEQ_VIEW_OBJS = src/mxeq/panel.o src/mxeq/app.o

# The BlueZ client. Its own directory because it is the one part of mxeq that talks to a bus, and
# the one part that has no drawing and no ALSA in it.
BLUEZ_OBJS = src/bluez/bluez.o

# The offline layout audit. It links the GFX objects, mxeq's panel and respath -- AND NOTHING ELSE.
# No X11, no ALSA, no dbus: if this list ever has to grow, something has reached across the line the
# header comment draws.
UIRENDER_OBJS = $(GFX_OBJS) src/mxeq/panel.o src/platform/respath.o tools/uirender.o

.PHONY: all clean toolkit manager bridge mxeq uirender graph install uninstall

# `toolkit` is the shared layer on its own, and it is what keeps this tree green between the phases
# of the port.
all: toolkit mxeq uirender manager bridge graph

toolkit: $(GFX_OBJS) $(PLAT_OBJS) $(MXEQ_MODEL_OBJS)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

# gfx objects: cairo only. NO X11_CFLAGS here, deliberately -- see the header comment.
src/gfx/%.o: src/gfx/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) -c -o $@ $<

src/platform/%.o: src/platform/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(X11_CFLAGS) -c -o $@ $<

# mxeq's own objects: cairo and ALSA, NO X11. panel.cpp is in here, which is what keeps the whole
# visible surface auditable headlessly -- an #include of Xlib.h under src/mxeq/ would fail to
# compile here rather than quietly costing the audit, and that is the point of the rule.
src/mxeq/%.o: src/mxeq/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(ALSA_CFLAGS) -c -o $@ $<

# THE ONE EXCEPTION, and it is explicit rather than pattern-matched so it cannot spread: main.cpp is
# where the window and the application meet, so it is the only file under src/mxeq/ that sees an X
# header. An explicit rule beats the pattern rule above for this target.
src/mxeq/main.o: src/mxeq/main.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(X11_CFLAGS) $(ALSA_CFLAGS) -c -o $@ $<

# The BlueZ client: dbus-1 and nothing else. No cairo, no X11, no ALSA.
src/bluez/%.o: src/bluez/%.cpp
	$(CXX) $(CXXFLAGS) $(DBUS_CFLAGS) -c -o $@ $<

tools/%.o: tools/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) -c -o $@ $<

# --- mxeq ---------------------------------------------------------------------------------------
# NO -ljack, and that is deliberate: CLAUDE.md records that mxeq does not link libjack, so there is
# no `mxeq:*` client in the JACK graph. Every JACK question it asks is a jack_lsp subprocess through
# platform/proc. `ldd contrib/bin/mxeq` is the check, and it is item 2 of the port's verification.
MXEQ_TARGET = $(BIN_DIR)/mxeq
MXEQ_OBJS   = $(GFX_OBJS) $(PLAT_OBJS) $(MXEQ_MODEL_OBJS) $(MXEQ_VIEW_OBJS) $(BLUEZ_OBJS) \
              src/mxeq/main.o

mxeq: $(MXEQ_TARGET)
$(MXEQ_TARGET): $(MXEQ_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDHARDEN) -o $@ $(MXEQ_OBJS) \
	    $(GFX_LIBS) $(X11_LIBS) $(ALSA_LIBS) $(DBUS_LIBS)

# --- the offline layout audit -------------------------------------------------------------------
# Not a test of behaviour: a test that every string the panel draws fits the slot it lands in, with
# the bundled faces, in BOTH mixer curation modes -- only one of which is on screen on any given
# machine. Run it before every commit that moves a rectangle.
UIRENDER_TARGET = tools/uirender

uirender: $(UIRENDER_TARGET)
$(UIRENDER_TARGET): $(UIRENDER_OBJS)
	$(CXX) $(CXXFLAGS) $(LDHARDEN) -o $@ $(UIRENDER_OBJS) $(GFX_LIBS)

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

# --- STILL GTK: jack-graph, BEING REPLACED IN PHASE 4 -------------------------------------------
#
# This rule is the OLD build and it is kept deliberately, not by oversight. The port lands in phases
# -- the shared toolkit, then mxeq, then jack-graph, then the settings window -- and each phase has
# to leave a tree that builds and runs. Deleting this before jack-graph/src/graphpanel.cpp exists
# would mean a run of commits with no working graph in them, which is also a run of commits nobody
# can bisect.
#
# WHAT GOES WHEN: this rule and jack-graph/Makefile go with Phase 4.
#
# THE GTK mxeq IS GONE as of this phase, and with it MOTR_*, GLIB_API_LEVEL and src/mxeq.c.
# src/gui_bt.c and src/bt_agent.c are NO LONGER BUILT but are still in the tree on purpose: they are
# the GDBus original the libdbus-1 client in src/bluez/ is being written from, member by member, and
# reading them beside the port is the whole reason the plan says that port must not be guessed at.
# They go when Phase 3 lands and src/bluez/bluez.cpp stops being a placeholder.
GRAPH_DIR    = jack-graph
GRAPH_TARGET = $(BIN_DIR)/jack-graph

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
DEPS = $(GFX_OBJS:.o=.d) $(PLAT_OBJS:.o=.d) $(MXEQ_MODEL_OBJS:.o=.d) \
       $(MXEQ_VIEW_OBJS:.o=.d) $(BLUEZ_OBJS:.o=.d) src/mxeq/main.d tools/uirender.d
-include $(DEPS)

clean:
	rm -f $(GFX_OBJS) $(PLAT_OBJS) $(MXEQ_MODEL_OBJS) $(MXEQ_VIEW_OBJS) $(BLUEZ_OBJS) \
	      src/mxeq/main.o tools/uirender.o $(DEPS)
	rm -f $(MANAGER_TARGET) $(BRIDGE_TARGET) $(MXEQ_TARGET) $(GRAPH_TARGET) $(UIRENDER_TARGET)
	$(MAKE) -C $(GRAPH_DIR) clean

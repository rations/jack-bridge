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
            src/platform/proc.o src/platform/wakepipe.o src/platform/x11window.o

# mxeq's model layer: the sound card, the output device, the recorder, the Steam bridge and the
# Bluetooth device list. No X11 and no drawing, so tools/uirender can drive the real models.
MXEQ_MODEL_OBJS = src/mxeq/alsamixer.o src/mxeq/devices.o src/mxeq/recorder.o \
                  src/mxeq/steam.o src/mxeq/btmodel.o

# mxeq's view: the five pages and the application that drives them. panel.o is in here rather than
# with the models because it is still cairo-only -- it is what tools/uirender links.
MXEQ_VIEW_OBJS = src/mxeq/panel.o src/mxeq/app.o

# The BlueZ client. Its own directory because it is the one part of mxeq that talks to a bus, and
# the one part that has no drawing and no ALSA in it. bus.cpp is the ONLY file in the tree that
# includes <dbus/dbus.h>.
BLUEZ_OBJS = src/bluez/bus.o src/bluez/agent.o src/bluez/bluez.o

# jack-graph's model layer, all of it untouched by the port: it never linked a toolkit. JackClient
# and AlsaClient are the only two that talk to a server; the other five are pure data.
GRAPH_MODEL_OBJS = jack-graph/src/JackClient.o jack-graph/src/AlsaClient.o \
                   jack-graph/src/Config.o jack-graph/src/JackServerControl.o \
                   jack-graph/src/Node.o jack-graph/src/Connection.o jack-graph/src/ClientBox.o

# jack-graph's view: the graph itself, the JACK Settings window, the toolbar and status line, and
# the application that drives them. THE THREE PANELS ARE CAIRO ONLY, like src/mxeq/panel.o, which is
# what lets tools/uirender audit them; app.o is where libjack appears.
GRAPH_VIEW_OBJS = jack-graph/src/graphpanel.o jack-graph/src/settingspanel.o \
                  jack-graph/src/chrome.o jack-graph/src/app.o

# The offline layout audit. It links the GFX objects, BOTH BINARIES' CAIRO-ONLY PANELS, the
# pure-data model types the graph panel lays out, and respath -- AND NOTHING ELSE. No X11, no ALSA,
# no dbus, no jack: if this list ever has to grow past that, something has reached across the line
# the header comment draws.
#
# jack-graph's settingspanel.o is NOT here, and that is the one deliberate gap: it takes a
# JackServerControl&, which links libjack and libasound. uirender.cpp's header says what it measures
# instead.
UIRENDER_OBJS = $(GFX_OBJS) src/mxeq/panel.o \
                jack-graph/src/graphpanel.o jack-graph/src/chrome.o \
                jack-graph/src/Node.o jack-graph/src/Connection.o jack-graph/src/ClientBox.o \
                src/platform/respath.o tools/uirender.o

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

# jack-graph's objects. THE SAME RULE AS src/mxeq/: cairo, plus the two servers its model layer
# talks to, and NO X11 -- graphpanel.cpp, settingspanel.cpp and chrome.cpp draw through Canvas and
# nothing else, which is what keeps them auditable headlessly.
jack-graph/src/%.o: jack-graph/src/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(JACK_CFLAGS) $(ALSA_CFLAGS) -c -o $@ $<

# THE ONE EXCEPTION, explicit so it cannot spread, exactly as src/mxeq/main.o is: main.cpp is where
# the windows and the application meet, so it is the only file under jack-graph/src/ that sees an X
# header.
jack-graph/src/main.o: jack-graph/src/main.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) $(X11_CFLAGS) $(JACK_CFLAGS) $(ALSA_CFLAGS) -c -o $@ $<

# The audit sees both binaries' geometry headers, so it needs jack-graph/src on the include path.
# Still no X11, ALSA, jack or dbus cflags: it composes the panels and nothing else.
tools/%.o: tools/%.cpp
	$(CXX) $(CXXFLAGS) $(GFX_CFLAGS) -Ijack-graph/src -c -o $@ $<

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

# --- jack-graph -------------------------------------------------------------------------------
# X11 + Cairo + FreeType, like mxeq, out of the same src/gfx and src/platform objects. jack-graph
# has no Makefile of its own any more: two copies of canvas.cpp in one repository would be a second
# implementation for uirender to fail to audit.
#
# THIS ONE DOES LINK libjack, and mxeq deliberately does not -- see the note on the mxeq target.
# jack-graph is a JACK client by definition: it is the graph.
GRAPH_TARGET = $(BIN_DIR)/jack-graph
GRAPH_OBJS   = $(GFX_OBJS) $(PLAT_OBJS) $(GRAPH_MODEL_OBJS) $(GRAPH_VIEW_OBJS) \
               jack-graph/src/main.o

# Named `graph` rather than `jack-graph`: a phony target named after the jack-graph/ directory would
# be considered up to date the moment that directory exists, and the build would silently never run.
graph: $(GRAPH_TARGET)
$(GRAPH_TARGET): $(GRAPH_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDHARDEN) -o $@ $(GRAPH_OBJS) \
	    $(GFX_LIBS) $(X11_LIBS) $(JACK_LIBS) $(ALSA_LIBS)

# Generated by -MMD; absent on the first build, which `-include` tolerates silently.
DEPS = $(GFX_OBJS:.o=.d) $(PLAT_OBJS:.o=.d) $(MXEQ_MODEL_OBJS:.o=.d) \
       $(MXEQ_VIEW_OBJS:.o=.d) $(BLUEZ_OBJS:.o=.d) src/mxeq/main.d \
       $(GRAPH_MODEL_OBJS:.o=.d) $(GRAPH_VIEW_OBJS:.o=.d) jack-graph/src/main.d \
       tools/uirender.d
-include $(DEPS)

clean:
	rm -f $(GFX_OBJS) $(PLAT_OBJS) $(MXEQ_MODEL_OBJS) $(MXEQ_VIEW_OBJS) $(BLUEZ_OBJS) \
	      src/mxeq/main.o $(GRAPH_MODEL_OBJS) $(GRAPH_VIEW_OBJS) jack-graph/src/main.o \
	      tools/uirender.o $(DEPS)
	rm -f $(MANAGER_TARGET) $(BRIDGE_TARGET) $(MXEQ_TARGET) $(GRAPH_TARGET) $(UIRENDER_TARGET)

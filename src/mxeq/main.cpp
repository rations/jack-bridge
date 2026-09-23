// mxeq -- the ALSA mixer, output device, recorder, Bluetooth and Steam panel.
//
// THE ONLY PLACE THE WINDOW AND THE APPLICATION MEET. App includes no X11 header and the window
// knows nothing about ALSA, BlueZ or subprocesses; everything below is the wiring between them,
// following Audio-Gui's main.cpp: handlers never paint, they set a dirty flag, and one
// invalidate() per handler turns into one repaint per pass round the loop.
//
// mxeq DOES NOT LINK libjack, deliberately -- CLAUDE.md records that there is therefore no `mxeq:*`
// client in the graph. Every JACK question it asks is a `jack_lsp` subprocess. Check with
// `ldd contrib/bin/mxeq`.

#include "mxeq/app.h"
#include "mxeq/geometry.h"
#include "platform/childreaper.h"
#include "platform/x11window.h"

#include <X11/Xlib.h>

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace jackbridge;

namespace
{

float scaleFromArgs(int argc, char **argv)
{
    // --scale, then $JACK_BRIDGE_SCALE, then 1. There is no automatic DPI detection here, for the
    // same reason the sibling projects have none: Xft.dpi, RandR's physical size and the
    // toolkit-specific overrides disagree with each other often enough that guessing produces a
    // window that is wrong in a way the user cannot correct.
    const char *v = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--scale") == 0 && i + 1 < argc)
            v = argv[i + 1];
    }
    if (!v)
        v = getenv("JACK_BRIDGE_SCALE");
    if (!v || !*v)
        return 1.0f;

    char *end = nullptr;
    const double d = strtod(v, &end);
    if (end == v || d <= 0.0)
        return 1.0f;
    if (d < geo::kScaleMin)
        return geo::kScaleMin;
    if (d > geo::kScaleMax)
        return geo::kScaleMax;
    return static_cast<float>(d);
}

} // namespace

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printf("usage: mxeq [--scale N]\n"
                   "  --scale N   render at N times the logical size (%.2f..%.2f)\n",
                   static_cast<double>(geo::kScaleMin), static_cast<double>(geo::kScaleMax));
            return 0;
        }
    }

    // BEFORE THE WINDOW OPENS ITS INPUT METHOD. XOpenIM reads the locale, and without these two
    // calls it gets "C" and the recorder's filename field cannot type an accented character --
    // which is the whole reason there is an input method. simple-login-gui does the same, for a
    // password rather than a filename.
    setlocale(LC_ALL, "");
    if (!XSetLocaleModifiers(""))
        fprintf(stderr, "jack-bridge: could not set the X locale modifiers\n");

    // The SIGCHLD self-pipe, before anything can fork. arecord, pulse-jack-bridge and
    // jack-route-select are all watched through it.
    childreaper::install();

    App app;

    X11Window win;

    // The window opens at the height the content needs. THE WIDTH IS PINNED and the height is
    // bounded: Geometry's minW/maxW are left at zero, which pins that axis, so mxeq keeps exactly
    // the fixed-width behaviour it has today while jack-graph uses the same window class freely
    // resizable. geometry.h asserts every page against kWinMaxH.
    X11Window::Geometry g;
    g.logicalW = geo::kWinW;
    g.logicalH = app.panel().layout();
    g.minH = geo::kWinMinH;
    g.maxH = geo::kWinMaxH;
    g.scale = scaleFromArgs(argc, argv);

    if (!win.open("Alsa Sound Connect", "mxeq", g))
        return 1;

    app.onHeightChanged = [&win](float h) { win.resize(geo::kWinW, h); };
    app.onNeedsRepaint = [&win] { win.invalidate(); };

    // The child-exit pipe joins the same select() the X connection is in.
    win.addFd(childreaper::readFd(), [] { childreaper::drain(); });

    // THE MIXER'S POLL DESCRIPTORS DO NOT SURVIVE A REOPEN -- snd_mixer_close() invalidates them --
    // so they are torn down and re-registered every time the output device changes and the mixer
    // follows it onto another card. Leaving a closed card's descriptors in the set is a loop that
    // wakes for ever on a bad fd.
    std::vector<int> mixerTokens;
    auto registerMixerFds = [&] {
        for (int t : mixerTokens)
            win.removeFd(t);
        mixerTokens.clear();
        for (int fd : app.mixer().pollDescriptors())
            mixerTokens.push_back(win.addFd(fd, [&app] { app.onMixerReadable(); }));
    };
    app.onMixerDescriptorsChanged = registerMixerFds;

    // The same for libdbus: it adds a watch during authentication, so the set changes at runtime and
    // registering once would miss it. bus.h explains why the one-descriptor shortcut is wrong.
    std::vector<int> busTokens;
    auto registerBusFds = [&] {
        for (int t : busTokens)
            win.removeFd(t);
        busTokens.clear();
        for (int fd : app.bluez().watchDescriptors())
            busTokens.push_back(win.addFd(fd, [&app] { app.bluez().handleWatches(); }));
    };
    app.bluez().onWatchesChanged = registerBusFds;

    // Devices gets the window's timer, for the Bluetooth port poll.
    app.devices().addTimer = [&win](int ms, std::function<void()> fn) {
        return win.addTimer(ms, std::move(fn));
    };
    app.devices().removeTimer = [&win](int token) { win.removeTimer(token); };

    if (!app.start())
        return 1;
    registerMixerFds();
    registerBusFds();

    // Two clocks. Both carry absolute deadlines, so dragging a slider cannot starve either -- which
    // is the bug Audio-Gui's window comment records from the panel it was ported from.
    win.addTimer(2000, [&app] { app.tickDevices(); });
    win.addTimer(1000, [&app] { app.tickRecorder(); });
    // libdbus's own timeouts: a call that times out with no traffic on the socket needs this, and
    // it is the second half of why the one-descriptor shortcut does not work.
    win.addTimer(250, [&app] { app.bluez().handleTimeouts(); });

    X11Window::Callbacks cb;

    cb.draw = [&app](Canvas &c) { app.panel().draw(c); };

    cb.button = [&](float x, float y, int button, bool pressed) {
        if (pressed)
            app.panel().press(x, y, button);
        else
            app.panel().release(x, y, button);
    };

    // Motion does NOT invalidate unconditionally: Panel::motion calls onNeedsRepaint itself, and
    // only when a hover actually moved or a drag changed a value. Marking every pixel dirty would
    // recompose the whole window for a pointer crossing it.
    cb.motion = [&](float x, float y) { app.panel().motion(x, y); };

    cb.scroll = [&](float x, float y, int dir) { app.panel().scroll(x, y, dir); };

    cb.key = [&](Key k, const char *text, int len, unsigned state) {
        return app.panel().key(k, text, len, state);
    };

    win.run(cb);
    return 0;
}

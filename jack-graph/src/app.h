// Everything jack-graph is, minus the windows.
//
// NO X11 HEADER HERE, and none in app.cpp -- the same rule mxeq's app.h states, for the same
// reason. This is JackGraph.cpp with Gtk::Window taken out from under it: it owns the JACK client,
// the ALSA sequencer client, the config, the server control, the graph panel, the window chrome and
// the settings panel, and it turns the chrome's semantic events into model calls and the models'
// changes back into panel content.
//
// IT OWNS NO DESCRIPTOR AND NO LOOP. It does own two timers, but only through the addTimer /
// removeTimer functions main.cpp gives it -- the same arrangement mxeq's Devices has for its
// Bluetooth port poll, and it is what lets the whole of this file be driven with no event loop at
// all.
//
//------------------------------------------------------------------------------------------------
// THE THREE JACK CALLBACKS ARRIVE ON JACK'S OWN THREADS AND NONE OF THEM MAY TOUCH ANYTHING HERE.
//
// JackClient's header is explicit about the shutdown one: "the handler must not touch the client or
// any widget; marshal to the main loop first". The GTK build marshalled it with Glib::Dispatcher,
// which is a self-pipe with a C++ wrapper on it, and platform/wakepipe.h is the same self-pipe
// -- already in the same select(), so the hop costs no second descriptor and no second mechanism.
//
// The GTK build did NOT marshal the other two, and that was a latent bug rather than a design: the
// port callback called Glib::signal_timeout().connect() and the xrun callback called
// Glib::signal_idle().connect_once() straight from JACK's notification thread. Both go through the
// same self-pipe now. So all three handlers do nothing but set an atomic flag and write one byte,
// and onPostFromJackThread() -- which runs on the main loop -- is the only place that acts.
//
// The 100 ms coalescer and the std::atomic guards are otherwise unchanged. A JACK restart registers
// dozens of ports in a burst and rebuilding the graph once per port would rebuild it dozens of
// times; the flag plus a one-shot timer collapses the burst into one refresh.

#pragma once

#include "AlsaClient.hpp"
#include "Config.hpp"
#include "JackClient.hpp"
#include "JackServerControl.hpp"
#include "chrome.h"
#include "gfx/canvas.h"
#include "graphpanel.h"
#include "settingspanel.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace jackbridge
{

class App
{
public:
    App();

    // Load the config, connect to JACK and to the ALSA sequencer, read the graph and fit it to the
    // window. Returns false only if something the program cannot run without failed -- and a
    // stopped JACK server is NOT that, it is the case Settings -> Start exists for.
    bool start();

    // The other half of start(), and not optional tidying: the callbacks below point into main()'s
    // locals, which are destroyed before this object is. mxeq/app.h records the AddressSanitizer
    // report that made this a rule. Severs the outward callbacks, drops the JACK client and saves
    // the config -- which is what ~JackGraph and on_delete_event did between them.
    void shutdown();

    //--- what main.cpp gives us -----------------------------------------
    // The window's timer, for the port-refresh coalescer, the reconnect poll and the settings
    // window's 1 Hz tick. Both must be set before start().
    std::function<int(int intervalMs, std::function<void()> fn)> addTimer;
    std::function<void(int token)> removeTimer;

    std::function<void()> onNeedsRepaint;

    // The Settings and About pills. main.cpp owns the two dialog windows, because opening one is
    // the one thing in this file that needs X11.
    std::function<void()> onOpenSettings;
    std::function<void()> onOpenAbout;
    // The settings window should go away. Deferred, always: this fires from inside the panel's own
    // release handler, so the window may only be asked to close, never closed here.
    std::function<void()> onCloseSettings;

    //--- the settings window --------------------------------------------
    // Builds the panel, wires the four callbacks the graph owes it and reloads. Returns null if one
    // is already open, which is what stops the pill opening a second window.
    //
    // THE RELOAD IS NOT REDUNDANT. The constructor reads the server before these callbacks exist,
    // so its frames/period query has nothing to ask; the GTK build called reload() here for
    // exactly this reason and the comment there says so.
    SettingsPanel *openSettings();
    void closeSettings();
    SettingsPanel *settings()
    {
        return mSettingsOpen ? mSettings.get() : nullptr;
    }
    // The 1 Hz refresh. A no-op when the window is not open.
    void tickSettings();

    //--- layout, paint, input -------------------------------------------
    // The window's logical size, at start-up and on every resize.
    void setSize(float w, float h);

    void draw(Canvas &c);
    void button(float x, float y, int button, bool pressed);
    void motion(float x, float y);
    void scroll(float x, float y, int dir);

    //--- the hop off JACK's threads -------------------------------------
    // Registered with wakepipe::setPostHandler. Runs on the main loop; the three flags are the
    // only thing JACK's threads touched.
    void onPostFromJackThread();

private:
    void attachJackCallbacks();
    // forgetPositions drops the saved box positions first, so the rebuild lays the graph out by
    // the automatic rule instead of restoring the user's arrangement. Only Tool::Refresh passes
    // true; every automatic caller leaves the arrangement alone.
    void refreshPorts(bool forgetPositions = false);
    void schedulePortRefresh();
    void startReconnectPoll();
    bool tryReconnectJack();
    void handleServerGone();
    void updateStatus();
    void onTool(Tool t);
    void repaint() const
    {
        if (onNeedsRepaint)
            onNeedsRepaint();
    }

    JackClient mJack;
    AlsaClient mAlsa;
    Config mConfig;
    JackServerControl mServer;

    GraphPanel mGraph;
    Chrome mChrome;
    std::unique_ptr<SettingsPanel> mSettings;

    bool mJackConnected = false;
    bool mAlsaConnected = false;

    // Written on JACK's notification threads, read and cleared on the main loop. Nothing else
    // crosses the boundary.
    std::atomic<bool> mShutdownPosted{false};
    std::atomic<bool> mPortsPosted{false};
    std::atomic<bool> mXrunPosted{false};

    // Main loop only, from here down.
    bool mRefreshPending = false;
    int mRefreshToken = -1;

    int mReconnectToken = -1;
    int mReconnectAttempts = 0;

    bool mSettingsOpen = false;
    int mSettingsTickToken = -1;
};

} // namespace jackbridge

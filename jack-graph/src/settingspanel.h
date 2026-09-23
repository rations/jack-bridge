// The JACK Settings window: start and stop the server, and choose what it starts with.
//
// This is SettingsDialog with Gtk::Dialog taken out from under it. The LOGIC MOVES ACROSS
// UNCHANGED -- populate_devices, load_current_settings, running_cmdline, arg_from, running_device,
// select_interface, persist_period, respawn_bridges, preferred_output, sync_preferred_output,
// refresh_running_state and the 1 Hz tick are the same functions doing the same things in the same
// order. Two of them carry comments recording bugs that were paid for, and both rules survive:
//
//   * populate_devices() RUNS BEFORE load_current_settings(). The other order silently no-ops,
//     because set_active_id on an empty list does nothing -- and jackd has started on the wrong
//     device because of it. /var/log/jackd-rt.log has sessions that began on "default" and on
//     "hw:0,0" for exactly that reason.
//
//   * m_last_running_device is what protects a selection the user has made but not yet started.
//     The periodic refresh only re-syncs the fields on a tick where the running device genuinely
//     moved, so a refresh cannot stomp on a choice in progress.
//
// WHAT CHANGED is the surface, and one behaviour. The surface: five gfx::Combos with the same
// items and the same stable value tokens, three pills, drawn group boxes. The behaviour: IT IS NO
// LONGER MODAL. That modality was Gtk::Dialog::run()'s doing -- a nested main loop -- and there is
// no nested loop in this architecture. Nothing is lost: the graph behind it is read-only while it
// is open, and the 1 Hz refresh already assumed the world changes underneath it.
//
// The set_tooltip_text() on Apply Live becomes drawn text under the field. There are no tooltips
// here, and a sentence explaining what a button does is better under the button than behind a
// hover nobody performs before pressing it.

#pragma once

#include "Config.hpp"
#include "JackServerControl.hpp"
#include "gfx/canvas.h"
#include "gfx/combo.h"
#include "gfx/keys.h"
#include "gfx/widgets.h"

#include <functional>
#include <string>

namespace jackbridge
{

class SettingsPanel
{
public:
    SettingsPanel(JackServerControl &server, Config &config);

    //--- what the graph window does for us ------------------------------
    // Start finished: drop the old JACK client and open one on the new server.
    std::function<void()> onApply;
    // Stop pressed: drop the client WITHOUT reconnecting, before the server goes away.
    std::function<void()> onDisconnect;
    // A live frames/period change. False means the running server refused it.
    std::function<bool(unsigned int nframes)> onBufferSize;
    // What the running server says its frames/period is. Only JACK knows this after a live change:
    // the command line still holds the size jackd was launched with. 0 when there is no server.
    std::function<unsigned int()> bufferSizeQuery;
    // Close was pressed, or the window manager closed us.
    std::function<void()> onClose;

    std::function<void()> onNeedsRepaint;

    // Read everything again. The graph calls this once after wiring the callbacks above, because
    // the constructor ran before they existed and the frames/period query had nothing to ask.
    void reload();

    // The 1 Hz tick. The device is not only ours to change -- mxeq is a separate process, and a
    // USB hotplug moves it with no GUI involved at all.
    void tick();

    //--- layout, paint, input -------------------------------------------
    float layout();
    float height() const
    {
        return mHeight;
    }
    void draw(Canvas &c) const;

    void motion(float x, float y);
    void press(float x, float y, int button);
    void release(float x, float y, int button);
    bool key(Key k, const char *text, int len, unsigned state);

private:
    enum class Target { Nothing, Start, Stop, ApplyLive, CloseButton, Interface, Rate, Frames,
                        Periods, Midi };
    Target targetAt(float x, float y) const;
    void clearHover();
    void repaint() const
    {
        if (onNeedsRepaint)
            onNeedsRepaint();
    }

    void populateDevices();
    void loadCurrentSettings();
    void updateServerStatus(bool running);
    void refreshRunningState();
    void onApplyLive();
    void onStart();
    void onStop();
    void selectInterface(const std::string &id);

    static std::string runningCmdline();
    static std::string argFrom(const std::string &cmd, const char *flag);
    static std::string runningDevice();
    static std::string preferredOutput();
    static bool persistPeriod(int frames);
    static void respawnBridges();
    static void syncPreferredOutput(const std::string &iface);

    JackServerControl &mServer;
    Config &mConfig;

    Combo mInterface;
    Combo mRate;
    Combo mFrames;
    Combo mPeriods;
    Combo mMidi;

    Pill mStart;
    Pill mStop;
    Pill mApplyLive;
    Pill mClose;

    std::string mServerStatus = "Status: Stopped";
    std::string mLiveStatus;

    // Last device the refresh saw, so a tick where nothing changed costs one process read and
    // stops there. See the header comment: it is also what protects an unstarted selection.
    std::string mLastRunningDevice;
    bool mRunningStateValid = false;

    float mHeight = 0.0f;
    Target mPressTarget = Target::Nothing;
    Target mHoverTarget = Target::Nothing;
};

} // namespace jackbridge

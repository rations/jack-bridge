// See app.h.

#include "app.h"

#include "graphgeometry.h"
#include "gfx/palette.h"
#include "platform/wakepipe.h"

#include <cstdio>
#include <memory>
#include <string>

namespace jackbridge
{

App::App() = default;

//------------------------------------------------------------------------
bool App::start()
{
    mConfig.load();

    mChrome.onNeedsRepaint = [this] { repaint(); };
    mChrome.onTool = [this](Tool t) { onTool(t); };
    mGraph.onNeedsRepaint = [this] { repaint(); };

    mGraph.onConnect = [this](const std::string &source, const std::string &dest) {
        if (mJackConnected)
            mJack.connect_ports(source, dest);
    };
    mGraph.onDisconnect = [this](const std::string &source, const std::string &dest) {
        if (mJackConnected)
            mJack.disconnect_ports(source, dest);
    };

    // Connect to a JACK server that is already running -- it usually is, started by jackd-rt at
    // boot. A server that is not running is not an error: that is what Settings -> Start is for.
    mJackConnected = mJack.connect("jack-graph");
    if (mJackConnected) {
        fprintf(stderr, "jack-graph: connected to the running JACK server\n");
        attachJackCallbacks();
    } else {
        fprintf(stderr, "jack-graph: JACK is not running; use JACK Settings to start it\n");
    }

    mAlsaConnected = mAlsa.connect("jack-graph");

    refreshPorts();

    // FIT ON FIRST SHOW. The GTK build deferred this to a Glib::signal_idle because it had to wait
    // for GTK to allocate the widget; here the window's size is known as soon as it is open and
    // setSize() has already given the panel its rect, so there is nothing to wait for.
    mGraph.fitToWindow();
    return true;
}

void App::shutdown()
{
    // Sever everything pointing outward FIRST -- see the header. Closing the settings window would
    // otherwise call into main()'s locals on the way out.
    onNeedsRepaint = nullptr;
    onOpenSettings = nullptr;
    onOpenAbout = nullptr;
    onCloseSettings = nullptr;
    mChrome.onNeedsRepaint = nullptr;
    mChrome.onTool = nullptr;
    mGraph.onNeedsRepaint = nullptr;
    if (mSettings)
        mSettings->onNeedsRepaint = nullptr;

    // Then the timers, before addTimer/removeTimer stop being callable.
    if (removeTimer) {
        if (mRefreshToken >= 0)
            removeTimer(mRefreshToken);
        if (mReconnectToken >= 0)
            removeTimer(mReconnectToken);
        if (mSettingsTickToken >= 0)
            removeTimer(mSettingsTickToken);
    }
    mRefreshToken = mReconnectToken = mSettingsTickToken = -1;
    addTimer = nullptr;
    removeTimer = nullptr;

    // And only then the client, whose shutdown callback would otherwise be able to post one more
    // byte into a handler that no longer has anything to do.
    wakepipe::setPostHandler(nullptr);
    mJack.disconnect();
    mJackConnected = false;
    mSettingsOpen = false;
    mSettings.reset();

    mConfig.save();
}

//------------------------------------------------------------------------
void App::attachJackCallbacks()
{
    // ONE PLACE THAT WIRES THESE, called from three -- the first connect, the reconnect after
    // Settings -> Start, and the reconnect after the server went away -- and every one of them
    // must register the same set. That was true of attach_jack_callbacks() and it is still true.
    //
    // ALL THREE RUN ON A JACK THREAD. They set a flag and write one byte; see the header.
    mJack.set_port_callback([this] {
        mPortsPosted.store(true);
        wakepipe::post();
    });
    mJack.set_xrun_callback([this] {
        mXrunPosted.store(true);
        wakepipe::post();
    });
    mJack.set_shutdown_callback([this] {
        mShutdownPosted.store(true);
        wakepipe::post();
    });
}

void App::onPostFromJackThread()
{
    // Shutdown first: there is no point refreshing a graph off a client that has been orphaned, and
    // handleServerGone() refreshes it itself once the client is gone.
    if (mShutdownPosted.exchange(false))
        handleServerGone();
    if (mPortsPosted.exchange(false))
        schedulePortRefresh();
    if (mXrunPosted.exchange(false))
        updateStatus();
}

// A burst of port registrations -- which is what a JACK restart or a device switch is -- collapses
// into one refresh 100 ms later. The flag is what makes it one timer rather than one per port.
void App::schedulePortRefresh()
{
    if (mRefreshPending)
        return;
    if (!addTimer) {
        refreshPorts();
        return;
    }
    mRefreshPending = true;
    mRefreshToken = addTimer(100, [this] {
        // ONE SHOT: the timer removes itself. X11Window dispatches timers from a copy of its list
        // and re-checks that a timer is still live before calling it, so removing this one from
        // inside its own handler is safe.
        const int token = mRefreshToken;
        mRefreshToken = -1;
        mRefreshPending = false;
        if (removeTimer && token >= 0)
            removeTimer(token);
        refreshPorts();
    });
}

//------------------------------------------------------------------------
// The server went away without this window asking it to.
//
// That is a routine event rather than a crash: selecting a USB interface restarts jackd on the
// interface, and so does unplugging or replugging one. Before this handler existed the canvas froze
// on a dead client until the user noticed and pressed Refresh.
void App::handleServerGone()
{
    if (!mJack.is_connected())
        return; // already torn down

    // The hop means this can arrive LATE -- after something else has already dealt with the restart
    // and opened a fresh client. Apply Live on a USB interface does exactly that: it restarts the
    // server and reconnects before the main loop gets round to this handler. Tearing down a client
    // that is alive and well would turn a working restart into a dropout.
    if (!mJack.server_gone())
        return;

    fprintf(stderr, "jack-graph: the JACK server went away; waiting for it to return\n");
    mJack.disconnect();
    mJackConnected = false;
    refreshPorts();

    mReconnectAttempts = 0;
    startReconnectPoll();
}

void App::startReconnectPoll()
{
    if (!addTimer)
        return;
    if (mReconnectToken >= 0 && removeTimer)
        removeTimer(mReconnectToken);
    mReconnectToken = addTimer(500, [this] {
        if (tryReconnectJack())
            return;
        const int token = mReconnectToken;
        mReconnectToken = -1;
        if (removeTimer && token >= 0)
            removeTimer(token);
    });
}

// True to keep polling.
bool App::tryReconnectJack()
{
    if (mJackConnected)
        return false;

    if (mJack.connect("jack-graph")) {
        mJackConnected = true;
        attachJackCallbacks();
        refreshPorts();
        fprintf(stderr, "jack-graph: reconnected to JACK\n");
        return false;
    }

    // 60 * 500ms = 30s, which comfortably covers a device switch (a jackd-rt restart plus the
    // interface's own warm-up). A server still absent after that was stopped deliberately, not
    // restarted, so stop polling and leave it to Settings -> Start.
    if (++mReconnectAttempts >= 60) {
        fprintf(stderr, "jack-graph: JACK did not return within 30s; no longer polling\n");
        updateStatus();
        return false;
    }
    return true;
}

//------------------------------------------------------------------------
void App::refreshPorts()
{
    mJack.scan_ports();
    mGraph.removeAll();

    if (mJackConnected) {
        const std::string ourClient = mJack.get_actual_client_name();
        for (const JackClient::PortInfo &p : mJack.get_ports()) {
            if (p.client == ourClient)
                continue;
            mGraph.addNode(std::make_shared<Node>(
                p.name, p.is_audio ? PortType::AUDIO : PortType::MIDI,
                p.is_output ? PortDirection::OUTPUT : PortDirection::INPUT));
        }

        const std::vector<std::shared_ptr<Node>> &all = mGraph.nodes();
        for (const JackClient::ConnectionInfo &c : mJack.get_connections()) {
            std::shared_ptr<Node> src, dst;
            for (const std::shared_ptr<Node> &n : all) {
                if (n->full_name() == c.source)
                    src = n;
                if (n->full_name() == c.destination)
                    dst = n;
            }
            if (src && dst)
                mGraph.addConnection(std::make_shared<Connection>(src, dst, src->type));
        }
    }

    // ALSA MIDI ports ONLY WHEN JACK IS NOT CONNECTED. With JACK running, jack_get_ports above
    // already reports every ALSA MIDI device the server bridged; adding them again from the
    // sequencer produces phantom duplicates (Midi-Through shows three ports instead of two).
    if (mAlsaConnected && !mJackConnected) {
        for (const AlsaClient::PortInfo &p : mAlsa.get_ports()) {
            mGraph.addNode(std::make_shared<Node>(
                p.client + ":" + p.name, PortType::MIDI,
                p.is_output ? PortDirection::OUTPUT : PortDirection::INPUT, true));
        }
    }

    mGraph.layout(true);
    updateStatus();
}

void App::updateStatus()
{
    std::string status;

    if (mJackConnected) {
        status += "JACK: connected";
        status += " | Buffer: " + std::to_string(mJack.get_buffer_size()) + " frames";
        status += " | Rate: " + std::to_string(mJack.get_sample_rate()) + " Hz";
        status += " | Xruns: " + std::to_string(mJack.get_xrun_count());
    } else {
        status += "JACK: not connected";
    }

    status += " | Server: " + mServer.get_status();

    if (mAlsaConnected)
        status += " | ALSA MIDI: connected";

    mChrome.setStatus(status);
}

//------------------------------------------------------------------------
void App::onTool(Tool t)
{
    switch (t) {
    case Tool::Refresh:
        refreshPorts();
        mGraph.fitToWindow();
        break;
    case Tool::ZoomOut:
        mGraph.setZoom(mGraph.zoom() / geo::kZoomStepButton);
        break;
    case Tool::ZoomIn:
        mGraph.setZoom(mGraph.zoom() * geo::kZoomStepButton);
        break;
    case Tool::ZoomNormal:
        mGraph.setZoom(1.0);
        break;
    case Tool::Fit:
        mGraph.fitToWindow();
        break;
    case Tool::Settings:
        if (onOpenSettings)
            onOpenSettings();
        break;
    case Tool::About:
        if (onOpenAbout)
            onOpenAbout();
        break;
    case Tool::None:
        break;
    }
}

//------------------------------------------------------------------------
SettingsPanel *App::openSettings()
{
    if (mSettingsOpen)
        return nullptr;

    // A panel left over from a window that was closed earlier. Dropped HERE rather than at close
    // time, because closing is asked for from inside the panel's own release handler.
    mSettings.reset(new SettingsPanel(mServer, mConfig));

    // Reconnect after Start: the old client is dropped and a new one opened on the new server.
    mSettings->onApply = [this] {
        if (mJack.is_connected()) {
            mJack.disconnect();
            mJackConnected = false;
        }
        if (mServer.is_running()) {
            mJackConnected = mJack.connect("jack-graph");
            if (mJackConnected) {
                attachJackCallbacks();
                refreshPorts();
            }
        }
        updateStatus();
    };

    // Stop: drop the client and do NOT reconnect.
    mSettings->onDisconnect = [this] {
        if (mJack.is_connected()) {
            mJack.disconnect();
            mJackConnected = false;
        }
        refreshPorts();
    };

    // A live frames/period change, with no server restart. False means the running server refused
    // it, which the panel reports rather than pretending it worked.
    mSettings->onBufferSize = [this](unsigned int nframes) -> bool {
        if (!mJack.is_connected())
            return false;
        if (!mJack.set_buffer_size(static_cast<jack_nframes_t>(nframes)))
            return false;
        updateStatus();
        return true;
    };

    // Only the running server knows this after a live change: the command line still holds the size
    // jackd was launched with.
    mSettings->bufferSizeQuery = [this]() -> unsigned int {
        return mJack.is_connected() ? static_cast<unsigned int>(mJack.get_buffer_size()) : 0u;
    };

    mSettings->onClose = [this] { closeSettings(); };

    // onNeedsRepaint is NOT set here: it has to invalidate the settings WINDOW, and main.cpp is
    // what owns that. It sets it as soon as the window is open.

    // See the header: the constructor ran before bufferSizeQuery existed.
    mSettings->reload();

    mSettingsOpen = true;
    mChrome.setActive(Tool::Settings, true);

    if (addTimer && mSettingsTickToken < 0) {
        // The device is not only ours to change: mxeq is a separate process and a USB hotplug moves
        // it with no GUI involved at all.
        mSettingsTickToken = addTimer(1000, [this] { tickSettings(); });
    }
    return mSettings.get();
}

void App::closeSettings()
{
    if (!mSettingsOpen)
        return;
    mSettingsOpen = false;
    mChrome.setActive(Tool::Settings, false);

    if (mSettingsTickToken >= 0 && removeTimer)
        removeTimer(mSettingsTickToken);
    mSettingsTickToken = -1;

    // DEFERRED. This is reached from inside SettingsPanel::release() and SettingsPanel::key(), so
    // the panel and its window are only asked to go away; the panel object itself is dropped at the
    // next openSettings() or at shutdown().
    if (onCloseSettings)
        onCloseSettings();
}

void App::tickSettings()
{
    if (mSettingsOpen && mSettings)
        mSettings->tick();
}

//------------------------------------------------------------------------
void App::setSize(float w, float h)
{
    mChrome.setWindow(Rect(0.0f, 0.0f, w, h));
    mGraph.setRect(mChrome.canvasRect());
    repaint();
}

void App::draw(Canvas &c)
{
    c.setColor(pal::kBgColor);
    c.fillRect(c.bounds());

    // Measured once, from the labels themselves; only a Canvas can measure a string.
    mChrome.layout(c);

    mGraph.draw(c);
    mChrome.draw(c);
}

void App::button(float x, float y, int button, bool pressed)
{
    if (pressed) {
        if (mChrome.press(x, y, button))
            return;
        mGraph.press(x, y, button);
        return;
    }
    if (mChrome.release(x, y, button))
        return;
    mGraph.release(x, y, button);
}

void App::motion(float x, float y)
{
    // The graph first, and only when it is mid-drag: a cable being dragged to a port near the top
    // of the canvas passes under the toolbar, and losing the drag there would be maddening.
    if (mGraph.motion(x, y))
        return;
    mChrome.motion(x, y);
}

void App::scroll(float x, float y, int dir)
{
    mGraph.scroll(x, y, dir);
}

} // namespace jackbridge

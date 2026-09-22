// See steam.h.

#include "steam.h"

#include "platform/childreaper.h"
#include "platform/proc.h"

#include <signal.h>

namespace jackbridge
{

Steam::~Steam()
{
    if (mPid > 0) {
        // Forget before signalling: the callback captures `this`, and a drain() between the two
        // would call into a destroyed object.
        //
        // The bridge is NOT killed here. It carries audio for a running game, and closing the mixer
        // window is not a reason to mute Steam -- which is the behaviour the GTK build had, because
        // a GLib child watch on a process that outlives its parent simply stops being watched.
        childreaper::forget(mPid);
    }
}

std::string Steam::statusText() const
{
    return mPid > 0 ? "Bridge active (pulse_bridge:playback_1/2)" : "Bridge inactive";
}

void Steam::toggle()
{
    if (mPid > 0) {
        // Ask it to exit. The state changes in handleExit(), not here, so the label cannot say
        // "inactive" while the bridge still holds the PulseAudio socket -- which matters because a
        // second start would then fail to bind it.
        kill(mPid, SIGTERM);
        if (onMessage)
            onMessage("Stopping the Steam bridge...", false);
        return;
    }

    // JACK first. `jack_lsp` succeeding is the whole test: the bridge cannot open a client without
    // a server, and it exits at once if it tries.
    if (proc::run({"jack_lsp"}) != 0) {
        if (onMessage) {
            onMessage("JACK is not running. Start it from jack-graph's JACK Settings window first.",
                      true);
        }
        return;
    }

    const pid_t pid = proc::spawnAsync({"pulse-jack-bridge"});
    if (pid <= 0) {
        if (onMessage) {
            onMessage("Could not start pulse-jack-bridge. Check that it is installed: "
                      "sudo ./contrib/install.sh",
                      true);
        }
        return;
    }

    mPid = pid;
    childreaper::watch(pid, [this](int status) { handleExit(status); });

    if (onMessage)
        onMessage("Steam bridge started.", false);
    if (onChanged)
        onChanged();
}

void Steam::handleExit(int status)
{
    (void)status;
    mPid = 0;
    if (onChanged)
        onChanged();
}

} // namespace jackbridge

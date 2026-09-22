// The Steam bridge: pulse-jack-bridge's lifecycle.
//
// Steam does not work with apulse, and this bridge is the workaround -- a minimal PulseAudio
// protocol server that creates one JACK client, pulse_bridge:playback_1/2. See CLAUDE.md.
//
// Two things carry across from mxeq.c:
//
//   * JACK IS CHECKED BEFORE SPAWNING. `jack_lsp` has to succeed first, because the bridge exits
//     immediately if it cannot open a JACK client and the user would see a toggle that flicked on
//     and straight back off with no explanation.
//
//   * STOP IS SIGTERM AND THE STATE FOLLOWS THE CHILD. The GTK build's comment is worth keeping:
//     do not close the pid here, because that would drop the watch's handle. The equivalent here is
//     that childreaper owns the watch and `onChanged` fires when the child has actually gone.
//
// The GTK build replaced a 1 Hz waitpid(WNOHANG) poll with g_child_watch_add for this, noting that
// needless wakeups are wrong in a stack whose routing daemon is explicitly event-driven and idle at
// zero CPU. platform/childreaper keeps that property: the exit arrives as a descriptor becoming
// readable in the select() the window is already in.

#pragma once

#include <functional>
#include <string>

namespace jackbridge
{

class Steam
{
public:
    Steam() = default;
    ~Steam();

    Steam(const Steam &) = delete;
    Steam &operator=(const Steam &) = delete;

    // The bridge started or stopped. The page re-reads isActive().
    std::function<void()> onChanged;
    std::function<void(const std::string &message, bool isError)> onMessage;

    // Starts the bridge if it is stopped, SIGTERMs it if it is running.
    void toggle();

    bool isActive() const
    {
        return mPid > 0;
    }

    // The line under the toggle: what the GTK build put in bridge_status_lbl.
    std::string statusText() const;

private:
    void handleExit(int status);

    int mPid = 0;
};

} // namespace jackbridge

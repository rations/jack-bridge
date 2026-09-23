// The pairing agent: we EXPORT org.bluez.Agent1 at /org/bluez/JackBridgeAgent, and BlueZ calls US.
//
// This replaces src/bt_agent.c, which was GDBus. Everything about what the agent answers is kept:
// it auto-accepts, non-interactively, because that is what the GTK build did and changing it would
// change which pairings succeed.
//
// THREE THINGS ARE DELIBERATELY DIFFERENT AND EACH IS A FIX, not a preference:
//
//  1. RequestAuthorization IS IMPLEMENTED. The GDBus build's introspection XML (bt_agent.c:26-58)
//     omits it, and that is a live bug rather than a choice. The capability registered is
//     "KeyboardDisplay", and BlueZ calls RequestAuthorization on a KeyboardDisplay agent for an
//     incoming just-works pairing -- src/device.c:7125 in the bluez 5.82 source, where
//     `confirm_hint && device->bonding == NULL` selects it. That is exactly the case the
//     Discoverable toggle invites. An agent that does not implement a method BlueZ calls fails the
//     pairing with org.freedesktop.DBus.Error.UnknownMethod, which the user sees as "it just does
//     not pair". doc/org.bluez.Agent.rst gives the signature: void RequestAuthorization(object).
//
//  2. THE RegisterDefault FALLBACK IS GONE. bt_agent.c calls RequestDefaultAgent and, if that
//     fails, RegisterDefault "(older BlueZ)". There is no RegisterDefault on org.bluez
//     .AgentManager1 -- not in 5.82's introspection on this machine, and not in
//     doc/org.bluez.AgentManager.rst. It was dead code that could only ever add a second failed
//     call to a failure path.
//
//  3. THE PROMPTS CAN REACH THE PANEL. RequestConfirmation and DisplayPasskey now go through
//     onPrompt if anything is listening, because there is a UI to show a passkey in now and there
//     was not before. THE AUTO-ACCEPT IS UNCHANGED: onPrompt is told, and the reply is sent
//     immediately either way. Making the reply wait for a human would change pairing from
//     something that works to something that needs a click, and bt_agent.c's ordering registers
//     the agent before the window exists.
//
// WHAT IS KEPT EXACTLY: the object path, the "KeyboardDisplay" capability (it decides which of the
// nine methods BlueZ calls at all, so it is not a free choice), the "0000" default PIN, the 0
// default passkey, and UnregisterAgent-before-RegisterAgent -- which exists because a crashed
// previous run leaves an agent entry behind and RegisterAgent then fails with "already
// registered" (bt_agent.c:185).

#pragma once

#include <functional>
#include <string>

namespace jackbridge
{

class Bus;

class Agent
{
public:
    static const char *const kPath;
    static const char *const kCapability;

    ~Agent();

    Agent() = default;
    Agent(const Agent &) = delete;
    Agent &operator=(const Agent &) = delete;

    // Exports the object and registers it with org.bluez.AgentManager1 as the default agent.
    // Returns false having warned; NOT fatal to the caller, because a Bluetooth page that cannot
    // pair is still a page that lists devices and can connect an already-paired one.
    bool install(Bus &bus);
    // UnregisterAgent, then drop the exported object. Safe to call twice.
    void remove();

    // A passkey or PIN worth showing a human. Optional: with nothing listening the agent behaves
    // exactly as the GDBus build did.
    std::function<void(const std::string &prompt)> onPrompt;

private:
    Bus *mBus = nullptr;
    bool mExported = false;
    bool mRegistered = false;
};

} // namespace jackbridge

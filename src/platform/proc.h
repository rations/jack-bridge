// Starting other programs, replacing g_spawn_async, g_spawn_command_line_sync,
// g_spawn_command_line_async and four popen() shell pipelines.
//
// EVERYTHING HERE TAKES AN ARGV ARRAY AND CALLS execvp DIRECTLY. There is no shell anywhere in
// this file: no system(), no popen(), no /bin/sh -c. That is a real change from what is being
// replaced and it is worth being precise about why, because the GTK build's shell use was not
// gratuitous -- it was convenient, and in one place it was a pipeline:
//
//     popen("jack_lsp 2>/dev/null | grep -q '^hdmi_out:'", "r")     (mxeq.c:2432)
//     popen("ps -o args= -C jackd 2>/dev/null", "r")                (mxeq.c:1604)
//     popen("aplay -l 2>/dev/null", "r")                            (mxeq.c:1321, :1549)
//     g_spawn_command_line_sync("pidof bluealsad", ...)              (mxeq.c:1635)
//
// The pipeline becomes runCapture({"jack_lsp"}) and a strstr, which is strictly less machinery for
// the same answer. The rest become argv arrays. What this buys:
//
//   * A DEVICE OR FILE NAME CAN CONTAIN A SHELL METACHARACTER. The names that flow through here
//     come from ALSA, from JACK and -- in the Bluetooth path -- from a remote device that chose
//     its own name. `g_spawn_command_line_async` on a string built by concatenation is one
//     apostrophe away from running something else, and BlueZ device names are attacker-controlled
//     in the plainest sense: anything in radio range picks them.
//   * NO SHELL PROCESS PER QUESTION. The device poll runs every two seconds.
//   * THE EXIT STATUS IS THE PROGRAM'S. popen() returns the shell's, so a missing binary and a
//     program that ran and said no are indistinguishable -- which is exactly the distinction
//     `jackd_is_on_usb_card` and the HDMI probe need.
//
// mxeq MUST NOT LINK libjack -- CLAUDE.md states it deliberately does not, so there is no `mxeq:*`
// client in the graph, and `jack_lsp` answers every JACK question it has through runCapture()
// instead. Do not "improve" that into a jack_get_ports() call.

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace jackbridge
{
namespace proc
{

using Argv = std::vector<std::string>;

// Environment entries to add to, or replace in, the inherited environment.
using EnvPairs = std::vector<std::pair<std::string, std::string>>;

// Start `argv` and return its pid, or -1 having warned. The caller is responsible for the child:
// register it with childreaper::watch() if it wants to know when it exits, which is what every
// current caller does.
//
// Replaces g_spawn_async with G_SPAWN_DO_NOT_REAP_CHILD. stdout and stderr are INHERITED, not
// captured: `arecord` and `pulse-jack-bridge` write diagnostics worth having in the terminal or
// the journal, and that is where the GTK build left them.
pid_t spawnAsync(const Argv &argv);

// Run `argv` to completion, collecting its standard output into `out`. Returns the exit status, or
// -1 if it could not be started. Replaces g_spawn_command_line_sync and every popen().
//
// SYNCHRONOUS, AND THAT IS THE POINT: `jack_lsp`, `aplay -l`, `pidof` and `ps` answer a question
// the caller needs the answer to before it can lay out a panel. Each returns a few hundred bytes
// and exits immediately.
//
// It reads to EOF BEFORE waiting, which is the ordering that matters: waiting first deadlocks as
// soon as a child writes more than a pipe buffer, and the fact that today's outputs are small is
// not a reason to write it the other way round.
//
// Standard error goes to /dev/null, matching the `2>/dev/null` on every pipeline being replaced.
int runCapture(const Argv &argv, std::string &out);

// runCapture without the output, for a command asked only whether it succeeded.
int run(const Argv &argv);

// Start a process that OUTLIVES THIS ONE, and return its pid or -1.
//
// "DETACHED" HAS THREE SEPARATE REQUIREMENTS, and it is worth naming them because meeting two and
// not the third is the bug that shows up a week later:
//
//   1. It must SURVIVE THE GUI. The Steam bridge carries audio for a running game, and closing the
//      mixer window must not mute it.
//   2. It must NOT BECOME A ZOMBIE while the GUI runs. The GUI would otherwise leave an entry in
//      the process table until it quit -- and a zombie is indistinguishable from a live process to
//      kill(pid, 0), which is how the Steam toggle decides whether the bridge is still up.
//   3. It must be in its OWN SESSION, so a Ctrl-C in the terminal the GUI was started from does
//      not take the audio down with it.
//
// The double fork satisfies all three: the intermediate child calls setsid() and forks again, then
// exits at once, which orphans the grandchild onto init (no zombie, survives the GUI) while the GUI
// reaps only the short-lived intermediate. The grandchild's pid comes back through a pipe.
//
// Ported from Audio-Gui's spawn.cpp, generalised from one program name to an argv.
pid_t spawnDetached(const Argv &argv, const std::string &cwd, const EnvPairs &extraEnv);

// True if a process with this pid exists. kill(pid, 0) cannot tell a zombie from a live process,
// which is why spawnDetached() above exists; for a pid this program did not fork, it is the answer.
bool pidAlive(pid_t pid);

// Full path of `program` as found on PATH, or an empty string. For the one thing the GTK build
// checked before spawning: whether the `jack-route-select` helper is installed at all, so the
// panel can say so instead of reporting a failed spawn.
std::string findOnPath(const std::string &program);

} // namespace proc
} // namespace jackbridge

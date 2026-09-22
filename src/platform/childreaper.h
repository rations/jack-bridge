// Child processes that exit, noticed on the main loop -- replacing g_child_watch_add.
//
// mxeq starts four kinds of child and needs to know when each one dies: `arecord` (the recorder's
// Stop label and duration timer depend on it), `pulse-jack-bridge` (the Steam toggle resets when
// it exits), `jack-route-select` (the Bluetooth output flow waits for it before polling for
// bluealsa's ports), and `bluetooth-enable.sh`. jack-graph needs the same mechanism for a
// different reason -- see the note on postByte() below.
//
// THE MECHANISM IS A SELF-PIPE. A SIGCHLD handler writes one byte to a pipe; the pipe's read end
// is registered with X11Window::addFd; the handler on the main thread calls drain(), which reaps
// and dispatches. This is what simple-login-gui already does for its session child, and it is the
// standard answer: almost nothing is legal inside a signal handler, but write() to a pipe is
// async-signal-safe, and it turns a signal into a file descriptor the same select() is already
// waiting on.
//
//------------------------------------------------------------------------------------------------
// THE ONE DESIGN DECISION THAT MATTERS: THIS ONLY EVER REAPS PIDS IT WAS ASKED TO WATCH.
//
// The obvious implementation of drain() is `while (waitpid(-1, &st, WNOHANG) > 0)`, and it is
// wrong here. proc::runCapture() forks a child, reads its output and waits for it SYNCHRONOUSLY --
// that is what makes `jack_lsp` and `aplay -l` answer a question inline. If the SIGCHLD handler
// reaped every child it could find, it would sometimes reap runCapture's child first, and
// runCapture's own waitpid would then fail with ECHILD and report a failure for a command that
// worked. It is a race, so it would not fail every time, which is worse.
//
// So drain() loops over the WATCH LIST and calls waitpid(pid, &st, WNOHANG) for each. A child
// nobody registered is nobody's business here, and runCapture keeps its own.
//
// The pipe is also why this is not just SIGCHLD with SA_RESTART: the point is not to survive the
// signal, it is to handle the exit on the main thread, at a defined moment, where a callback may
// safely touch the panel and repaint.
//------------------------------------------------------------------------------------------------

#pragma once

#include <sys/types.h>

#include <functional>

namespace jackbridge
{
namespace childreaper
{

// Installs the SIGCHLD handler and creates the self-pipe. Idempotent; returns false having warned
// if the pipe could not be created, in which case watch() still records callbacks and drain() can
// still be called from a timer -- a degradation, not a failure.
//
// Uses sigaction with SA_RESTART and SA_NOCLDSTOP: SA_RESTART because an unrestarted read() on the
// ALSA mixer's descriptors would start returning EINTR for no reason the caller can act on, and
// SA_NOCLDSTOP because a child that was merely stopped has not exited and waking the loop for it
// is noise.
bool install();

// The read end of the self-pipe, for X11Window::addFd, or -1 if install() failed.
int readFd();

// Call `onExit` on the main loop when `pid` exits, with the raw wait status. Replaces one
// g_child_watch_add. Watching a pid twice replaces the callback rather than adding a second.
void watch(pid_t pid, std::function<void(int status)> onExit);

// Stop watching, without waiting. For a child that has been handed to somebody else, or whose
// callback would outlive the object it captured -- which is the case this exists for: a panel that
// is being torn down must forget its children before its callbacks dangle.
void forget(pid_t pid);

// Drain the pipe and reap. Register this with X11Window::addFd(readFd(), drain).
//
// Reads until the pipe is empty, because several children can exit between two passes and SIGCHLD
// is not queued -- one byte may stand for any number of exits. Then walks the watch list, so the
// number of waitpid calls is the number of children we care about and not the number of bytes.
void drain();

// Turn one byte of the pipe into a callback on the main loop, with no child involved.
//
// THIS IS THE JACK SHUTDOWN HOP, and it is why this file is not called "children". JACK calls the
// shutdown callback registered with jack_on_shutdown FROM ITS OWN THREAD, and jack-graph's
// existing comment is explicit that the handler must not touch the client or any widget and must
// marshal to the main loop first. The GTK build used Glib::Dispatcher, which is a self-pipe with a
// C++ wrapper on it. This is the same self-pipe, already in the same select(), so the hop costs no
// second descriptor and no second mechanism.
//
// ASYNC-SIGNAL-SAFE AND THREAD-SAFE BY THE SAME ARGUMENT: it does nothing but write() one byte.
// The callback runs on the main thread from drain().
void postFromOtherThread();

// The callback postFromOtherThread() will run, on the main loop. Set once at start-up.
void setPostHandler(std::function<void()> fn);

} // namespace childreaper
} // namespace jackbridge

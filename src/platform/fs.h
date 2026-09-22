// Filesystem and path helpers, replacing the QFile / QDir / QSaveFile / QStandardPaths /
// QCoreApplication::applicationDirPath surface the Qt build used.
//
// THE ATOMIC WRITE IS THE LOAD-BEARING ONE. QSaveFile::commit() wrote to a temporary and renamed
// it into place, and two of its users here genuinely need that: ~/.asoundrc, which ALSA may open
// at any moment from any process on the machine, and the bridge control file, which the running
// bridge reads from its SIGUSR1 path. A plain truncate-and-write leaves a window in which either
// reader sees an empty or half-written file, and for the control file that window is exactly when
// it is being read, because the signal is sent immediately afterwards.

#pragma once

#include <string>

namespace jackbridge
{
namespace fs
{

bool exists(const std::string &path);

// Whole-file read. Returns false if the file could not be opened.
bool readFile(const std::string &path, std::string &out);

// Write via a temporary in the same directory, fsync, then rename over the target. The rename is
// what makes a reader see either the old contents or the new ones and never a partial file.
// Creates parent directories as needed. Returns false having left the target untouched.
bool writeFileAtomic(const std::string &path, const std::string &body);

bool removeFile(const std::string &path);

// mkdir -p. Returns true if the directory exists afterwards.
bool makeDirs(const std::string &path);

// $HOME, or the passwd entry if it is unset.
const std::string &homeDir();

// $XDG_CONFIG_HOME, else ~/.config. jack-bridge's own devices.conf lives at
// $configHome/jack-bridge/devices.conf, and jack-route-select writes the same path from shell --
// see CLAUDE.md. The two must agree.
const std::string &configHome();

// $XDG_RUNTIME_DIR, else /tmp -- the same fallback QStandardPaths::RuntimeLocation had. For state
// that should not survive a reboot.
const std::string &runtimeDir();

// The user's Music directory, which is where the recorder writes.
//
// Replaces g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) and does what GLib does: read
// XDG_MUSIC_DIR out of $XDG_CONFIG_HOME/user-dirs.dirs, expanding a leading $HOME, and fall back
// to ~/Music. Reading the file matters rather than assuming ~/Music -- a user whose Music
// directory is on another filesystem set it there for a reason, and a recorder that ignores it
// writes an hour of audio to the wrong disc.
//
// Always returns a path; it is not guaranteed to exist, and the recorder creates it.
const std::string &musicDir();

// The running executable, from /proc/self/exe. respath.cpp looks for the bundled fonts relative to
// this, which is what lets an uninstalled build find its resources.
const std::string &exePath();
const std::string &exeDir();

} // namespace fs
} // namespace jackbridge

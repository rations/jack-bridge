// See recorder.h.

#include "recorder.h"

#include "platform/wakepipe.h"
#include "platform/fs.h"
#include "platform/proc.h"

#include <signal.h>
#include <sys/wait.h>

#include <cstdio>
#include <ctime>

namespace jackbridge
{

const char *const Recorder::kChannelItems[] = {"Mono", "Stereo", nullptr};
const char *const Recorder::kRateItems[] = {"44100", "48000", nullptr};

namespace
{

long monotonicNs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<long>(ts.tv_sec) * 1000000000L + ts.tv_nsec;
}

} // namespace

//------------------------------------------------------------------------
Recorder::~Recorder()
{
    if (mPid > 0) {
        // The callback captures `this`, and `this` is going away. Forget it BEFORE signalling, or a
        // drain() between the two would call into a destroyed object.
        wakepipe::forget(mPid);
        kill(mPid, SIGINT);
    }
}

//------------------------------------------------------------------------
std::string Recorder::defaultFilename()
{
    const time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[128];
    strftime(buf, sizeof(buf), "Alsa Sound Connect-%Y%m%d-%H%M%S.wav", &tmv);
    return std::string(buf);
}

std::string Recorder::ensureWavExtension(const std::string &name)
{
    const std::string ext = ".wav";
    if (name.size() >= ext.size() && name.compare(name.size() - ext.size(), ext.size(), ext) == 0)
        return name;
    return name + ext;
}

std::string Recorder::sanitizeBasename(const std::string &name)
{
    // Everything after the last '/'. Replaces g_path_get_basename, and keeps its two edge cases:
    // a trailing slash and a name that is nothing but slashes both have to produce something that
    // is not a directory traversal.
    size_t end = name.find_last_not_of('/');
    if (end == std::string::npos)
        return std::string(); // all slashes
    const size_t slash = name.find_last_of('/', end);
    const size_t begin = slash == std::string::npos ? 0 : slash + 1;
    return name.substr(begin, end - begin + 1);
}

//------------------------------------------------------------------------
int Recorder::elapsedSeconds() const
{
    if (mPid <= 0)
        return 0;
    return static_cast<int>((monotonicNs() - mStartNs) / 1000000000L);
}

//------------------------------------------------------------------------
bool Recorder::start(const Settings &s)
{
    if (mPid > 0)
        return true; // already recording

    const std::string base = sanitizeBasename(s.filename);
    if (base.empty()) {
        if (onMessage)
            onMessage("Enter a filename to record to.", true);
        return false;
    }

    const std::string dir = fs::musicDir();
    if (!fs::makeDirs(dir)) {
        if (onMessage)
            onMessage("Cannot create " + dir, true);
        return false;
    }

    mPath = dir + "/" + ensureWavExtension(base);

    // See the header for why the device is `jack` and the format is FLOAT_LE. Neither is a default
    // worth revisiting: the first is the only capture path that exists while jackd holds the card,
    // and the second is what the ALSA JACK plugin accepts.
    const proc::Argv argv = {
        "arecord",
        "-D", "jack",
        "-r", std::to_string(s.rate),
        "-c", std::to_string(s.channels),
        "-f", "FLOAT_LE",
        "-t", "wav",
        mPath,
    };

    const pid_t pid = proc::spawnAsync(argv);
    if (pid <= 0) {
        if (onMessage) {
            onMessage("Could not start arecord. Check that it is installed and that jackd is "
                      "running.",
                      true);
        }
        mPath.clear();
        return false;
    }

    mPid = pid;
    mStartNs = monotonicNs();
    wakepipe::watch(pid, [this](int status) { handleExit(status); });

    fprintf(stderr, "jack-bridge: recording to %s (pid %d)\n", mPath.c_str(), pid);
    return true;
}

//------------------------------------------------------------------------
void Recorder::stop()
{
    if (mPid <= 0)
        return;

    // SIGINT, so arecord goes back and writes the real length into the WAV header it wrote at the
    // start. SIGTERM would leave a file claiming zero bytes. See the header.
    if (kill(mPid, SIGINT) != 0)
        kill(mPid, SIGTERM);

    // The state stays as it is. It changes in handleExit(), when the child has actually gone, so
    // the buttons cannot say "idle" while arecord is still finalising the file.
}

//------------------------------------------------------------------------
void Recorder::handleExit(int status)
{
    const std::string path = mPath;
    mPid = 0;
    mStartNs = 0;

    // A clean exit, or the SIGINT we sent. arecord reports the signal it died from, and the one we
    // asked for is not a failure to report.
    const bool ok = status == -1 || (WIFEXITED(status) && WEXITSTATUS(status) == 0) ||
                    (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT);

    if (onMessage) {
        if (ok && !path.empty())
            onMessage("Saved " + path, false);
        else if (!ok)
            onMessage("Recording failed. Check that jackd is running and that the ALSA `jack` "
                      "PCM is configured.",
                      true);
    }

    mPath.clear();
    if (onFinished)
        onFinished();
}

} // namespace jackbridge

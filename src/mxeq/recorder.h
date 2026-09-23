// The recorder: arecord's lifecycle and the filename rules, with no UI in it.
//
// Ported from mxeq.c's recorder section. Three things carry across exactly, because each is a
// decision somebody had to make once:
//
//   * IT RECORDS THROUGH THE ALSA `jack` PCM, not through the hardware. That PCM is defined in
//     50-jack.conf (installed to /etc/alsa/conf.d/ or /usr/share/alsa/alsa.conf.d/) and its
//     capture_ports point at system:capture_1/2, which jackd creates from its -C device. So arecord
//     captures mic and line-in THROUGH JACK. `plughw:N` is unavailable, because jackd holds the
//     hardware exclusively, and `default` is playback-only here. This is not a preference and
//     changing it breaks recording outright.
//
//   * THE FORMAT IS FLOAT_LE. The ALSA JACK plugin requires it. S16_LE looks more normal and does
//     not work.
//
//   * STOP IS SIGINT, NOT SIGTERM. arecord finalises the WAV header on SIGINT -- it goes back and
//     writes the real data length into the RIFF chunk it wrote at the start. SIGTERM kills it
//     first, leaving a header that claims zero bytes, which most players read as an empty file. A
//     SIGTERM follows only if the SIGINT could not be delivered at all.
//
// The child is watched through platform/wakepipe rather than g_child_watch_add, so `onFinished`
// arrives on the main loop exactly as the GLib callback did.

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

class Recorder
{
public:
    Recorder() = default;
    ~Recorder();

    Recorder(const Recorder &) = delete;
    Recorder &operator=(const Recorder &) = delete;

    // arecord exited, for whatever reason: Stop was pressed, it failed to start, or it died. The
    // panel returns to its idle state from here and nowhere else, so there is one path back.
    std::function<void()> onFinished;

    // Something the user needs to read. Goes to the page's message strip, which is what replaces
    // this path's two gtk_message_dialog_new + gtk_dialog_run sites.
    std::function<void(const std::string &message, bool isError)> onMessage;

    struct Settings {
        std::string filename; // as typed; sanitised and suffixed before use
        int channels = 2;     // 1 = Mono, 2 = Stereo
        int rate = 48000;     // 44100 or 48000
    };

    // Starts arecord. Returns false having already called onMessage. Does nothing and returns true
    // if a recording is already running.
    bool start(const Settings &s);

    // Sends SIGINT. The state does not change here: it changes when the child actually exits and
    // wakepipe calls back, which is what keeps the button state and the process in step even
    // when arecord takes a moment to finalise the file.
    void stop();

    bool isRecording() const
    {
        return mPid > 0;
    }

    // Seconds since the recording started, for the 1 Hz duration label. Zero when idle.
    int elapsedSeconds() const;

    // The file being written, or empty. Shown after a successful stop so the user knows where it
    // went -- something the GTK build never said.
    const std::string &outputPath() const
    {
        return mPath;
    }

    //--- the filename rules, static so tools/uirender can pin the strings they produce ---
    // "Alsa Sound Connect-YYYYmmdd-HHMMSS.wav" -- the prefix matches the application name.
    static std::string defaultFilename();
    // Appends .wav unless it is already there.
    static std::string ensureWavExtension(const std::string &name);
    // Strips any directory part. A filename field is a filename, not a path: the recording goes to
    // the Music directory and a typed "../../etc/x" must not escape it.
    static std::string sanitizeBasename(const std::string &name);

    // The two combo item lists, so the panel and the audit agree on them.
    static const char *const kChannelItems[]; // "Mono", "Stereo"
    static const char *const kRateItems[];    // "44100", "48000"

private:
    void handleExit(int status);

    int mPid = 0;
    long mStartNs = 0;
    std::string mPath;

    // Set by stop(), read by handleExit(). arecord CATCHES SIGINT rather than dying from it: it
    // finalises the WAV header and then exits(1), so the exit status of a perfectly good recording
    // is indistinguishable from a real failure by status alone. What distinguishes them is whether
    // we asked. See handleExit().
    bool mStopRequested = false;
};

} // namespace jackbridge

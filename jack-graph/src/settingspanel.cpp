// See settingspanel.h.

#include "settingspanel.h"

#include "graphgeometry.h"
#include "gfx/ink.h"
#include "gfx/palette.h"
#include "platform/proc.h"

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace jackbridge
{

namespace
{

const char *const kRouteHelper = "/usr/local/lib/jack-bridge/jack-route-select";
const char *const kServiceHelper = "/usr/local/lib/jack-bridge/jack-bridge-service-helper";
const char *const kJackdDefaults = "/etc/default/jackd-rt";

// Select the item whose stable VALUE token matches, never its visible label. combo.h records why:
// matching on the visible text is how "48000" and "48000 Hz" become the same bug twice. This is
// Gtk::ComboBoxText::set_active_id, and like it, it reports whether anything matched.
bool selectByValue(Combo &combo, const std::string &value)
{
    if (value.empty())
        return false;
    const std::vector<ComboItem> &items = combo.items();
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].value == value) {
            combo.setIndex(static_cast<int>(i));
            return true;
        }
    }
    return false;
}

void label(Canvas &c, float x, float cy, const char *text, float maxW)
{
    c.setFont(Font::Body);
    c.setFontSize(geo::kBodySize);
    c.setColor(pal::kDimColor);
    const std::string t = c.clipToWidth(text, maxW);
    c.drawString(t.c_str(), x, cy + geo::kBodySize * geo::kLabelBaselineBias);
}

} // namespace

//------------------------------------------------------------------------
SettingsPanel::SettingsPanel(JackServerControl &server, Config &config)
    : mServer(server), mConfig(config)
{
    mStart.label = "Start";
    mStop.label = "Stop";
    mApplyLive.label = "Apply Live";
    mClose.label = "Close";

    // The same item lists the Gtk::ComboBoxTexts held, with the same value tokens.
    mRate.setItems({{"44100", "44100"},
                    {"48000", "48000"},
                    {"88200", "88200"},
                    {"96000", "96000"},
                    {"192000", "192000"}});
    mFrames.setItems({{"64", "64"},
                      {"128", "128"},
                      {"256", "256"},
                      {"512", "512"},
                      {"1024", "1024"},
                      {"2048", "2048"}});
    mPeriods.setItems(
        {{"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"7", "7"}, {"8", "8"}});
    // The LABEL is what the user reads and the VALUE is what jackd is started with: "-X seq", or
    // the flag omitted entirely for none.
    mMidi.setItems({{"None", "none"}, {"ALSA SEQ", "seq"}});

    // DEVICES FIRST. loadCurrentSettings() selects by value, which silently does nothing on an
    // empty list -- see the header: that ordering bug started jackd on the wrong device.
    populateDevices();
    loadCurrentSettings();
    refreshRunningState();
}

//------------------------------------------------------------------------
void SettingsPanel::reload()
{
    loadCurrentSettings();
    repaint();
}

void SettingsPanel::tick()
{
    refreshRunningState();
}

//------------------------------------------------------------------------
void SettingsPanel::populateDevices()
{
    std::vector<ComboItem> items;
    items.push_back({"default", "default"});

    const std::string devices = mServer.list_audio_devices();
    std::istringstream stream(devices);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty())
            continue;
        // list_audio_devices() returns "hw:CARD=id,DEV=n|Card Name - Device". The id is what jackd
        // is started with and what gets persisted; the label is the only half the user should have
        // to read.
        const size_t pipe = line.find('|');
        if (pipe == std::string::npos)
            items.push_back({line, line});
        else
            items.push_back({line.substr(pipe + 1), line.substr(0, pipe)});
    }
    mInterface.setItems(std::move(items));
}

//------------------------------------------------------------------------
// The command line of the running jackd, or empty when it is not running.
//
// This is the last resort for showing the truth. /etc/default/jackd-rt is read first because it is
// what a restart will use, and because frames/period can be changed live -- set-period keeps the
// file current while argv keeps whatever the server was launched with. For everything that cannot
// change without a restart, argv is exact.
//
// NO popen(), AND THEREFORE NO SHELL. The GTK build ran `ps -o args= -C jackd 2>/dev/null` through
// popen; here it is an argv array through proc::runCapture, which is the rule the rest of this
// project already follows.
std::string SettingsPanel::runningCmdline()
{
    std::string out;
    if (proc::runCapture({"ps", "-o", "args=", "-C", "jackd"}, out) != 0)
        return std::string();

    // ps prints one line per matching process; the GTK build kept the LAST one, and so does this.
    std::string last;
    std::istringstream stream(out);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty())
            last = line;
    }
    return last;
}

// The value jackd was started with for a given flag, or empty.
//
// `flag` MUST INCLUDE ITS TRAILING SPACE: jackd's realtime priority is "-P70" with no space, and a
// search for a bare "-P" would return "70" as the playback device.
std::string SettingsPanel::argFrom(const std::string &cmd, const char *flag)
{
    const size_t at = cmd.find(flag);
    if (at == std::string::npos)
        return std::string();

    const size_t from = at + strlen(flag);
    const size_t end = cmd.find_first_of(" \t\n", from);
    return cmd.substr(from, end == std::string::npos ? std::string::npos : end - from);
}

// jackd-rt starts the server as `-d alsa -P <device> -C <device>`, so -P carries the device.
// jack-route-select's server_card_id() and jack-usb-hotplug parse the same thing; they must agree
// on the form.
std::string SettingsPanel::runningDevice()
{
    return argFrom(runningCmdline(), "-P ");
}

// Select a device in the combo, tolerating the two spellings of device 0.
//
// JACKD_DEVICE may hold either "hw:CARD=x" (what detect-alsa-device.sh and set-device write) or
// "hw:CARD=x,DEV=0" (what an older jack-graph wrote, or a hand edit). The list uses the bare form,
// so try that first and then the suffixed one rather than leaving the field blank on a working
// machine.
void SettingsPanel::selectInterface(const std::string &id)
{
    if (id.empty())
        return;
    if (selectByValue(mInterface, id))
        return;

    const std::string dev0 = ",DEV=0";
    if (id.size() > dev0.size() &&
        id.compare(id.size() - dev0.size(), dev0.size(), dev0) == 0)
        selectByValue(mInterface, id.substr(0, id.size() - dev0.size()));
    else
        selectByValue(mInterface, id + dev0);
}

//------------------------------------------------------------------------
void SettingsPanel::updateServerStatus(bool running)
{
    mServerStatus = running ? "Status: Running" : "Status: Stopped";
    mStart.enabled = !running;
    mStop.enabled = running;
    // Nothing to apply to a server that is not running; Start carries the settings in that case.
    mApplyLive.enabled = running;
}

// Keep Interface showing the device jackd is actually on.
//
// The device is not only ours to change: mxeq is a separate process and moves it whenever the user
// picks an output there, and a USB hotplug moves it with no GUI involved at all.
//
// THE EARLY RETURN IS WHAT MAKES THIS SAFE AS WELL AS CHEAP. A selection the user has made but not
// yet pressed Start on is only ever overwritten on a tick where the running device genuinely moved
// -- which is new information worth showing, not a refresh stomping on them.
void SettingsPanel::refreshRunningState()
{
    const std::string dev = runningDevice();

    if (mRunningStateValid && dev == mLastRunningDevice)
        return;
    mLastRunningDevice = dev;
    mRunningStateValid = true;

    // jackd's presence in the process table is the liveness test here, not is_running(): it is
    // already in hand, and it costs no JACK client. Calling is_running() on a timer would open and
    // close one every tick, which shows up as a "status_check" client blinking in the canvas.
    updateServerStatus(!dev.empty());

    // Nothing to show for a stopped server: leave the combos holding whatever the next Start would
    // use.
    if (!dev.empty())
        loadCurrentSettings();

    repaint();
}

//------------------------------------------------------------------------
void SettingsPanel::loadCurrentSettings()
{
    updateServerStatus(mServer.is_running());

    // Precedence, and the reason for it: this window exists to show what the server is doing, so
    // the running server wins.
    //
    //   1. the running jackd     -- exact, and the only source that cannot be stale
    //   2. /etc/default/jackd-rt -- what the next start will use
    //   3. jack-graph's own config -- private memory, for when nothing is running
    //
    // Getting this backwards is what made Periods/Buffer lie. It used to read the file, then the
    // config, and only ask the server if both were silent -- so on a fresh install, where the
    // shipped file leaves JACKD_NPERIODS commented out and jackd-rt falls back to 3, the field
    // showed a remembered value while the server ran at 3.
    //
    // One command line read, parsed several times: ps is a fork, and this runs again on every
    // device change.
    const std::string cmd = runningCmdline();

    std::string dev, sr, period, nperiods, midi;
    std::ifstream ifs(kJackdDefaults);
    if (ifs.is_open()) {
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            const size_t eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            const std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                val = val.substr(1, val.size() - 2);

            if (key == "JACKD_DEVICE")
                dev = val;
            else if (key == "JACKD_SR")
                sr = val;
            else if (key == "JACKD_PERIOD")
                period = val;
            else if (key == "JACKD_NPERIODS")
                nperiods = val;
            else if (key == "JACKD_MIDI")
                midi = val;
        }
    }

    // Interface
    std::string v = argFrom(cmd, "-P ");
    if (v.empty())
        v = dev;
    if (v.empty())
        v = mConfig.get_interface();
    selectInterface(v);

    // Sample rate -- cannot change without a restart, so the command line is exact whenever there
    // is a server at all.
    v = argFrom(cmd, "-r ");
    if (v.empty())
        v = sr;
    if (v.empty() && mConfig.get_sample_rate() > 0)
        v = std::to_string(mConfig.get_sample_rate());
    selectByValue(mRate, v);

    // Periods/buffer -- likewise fixed for the life of the server.
    v = argFrom(cmd, "-n ");
    if (v.empty())
        v = nperiods;
    if (v.empty() && mConfig.get_periods_per_buffer() > 0)
        v = std::to_string(mConfig.get_periods_per_buffer());
    selectByValue(mPeriods, v);

    // FRAMES/PERIOD IS THE ONE VALUE THAT CAN CHANGE ON A RUNNING SERVER, so the order differs.
    // Ask JACK, because after an Apply Live the command line still holds the size jackd was
    // launched with. The file comes next: it is only correct here because set-period rewrites it
    // after a successful live change -- and if that write failed, the live query above has already
    // answered.
    v.clear();
    if (bufferSizeQuery) {
        const unsigned int n = bufferSizeQuery();
        if (n > 0)
            v = std::to_string(n);
    }
    if (v.empty())
        v = period;
    if (v.empty())
        v = argFrom(cmd, "-p ");
    if (v.empty() && mConfig.get_frames_per_period() > 0)
        v = std::to_string(mConfig.get_frames_per_period());
    selectByValue(mFrames, v);

    // MIDI: jackd carries it as "-X seq", and omits the flag entirely for none.
    v = argFrom(cmd, "-X ");
    if (v.empty() && !cmd.empty())
        v = "none";
    if (v.empty())
        v = midi;
    if (v.empty())
        v = mConfig.get_midi_driver();
    selectByValue(mMidi, v);
}

//------------------------------------------------------------------------
// The user's saved output device, read the same way jack-route-select and jack-bridge-ports read
// it. Empty when unset.
std::string SettingsPanel::preferredOutput()
{
    const char *home = std::getenv("HOME");
    if (!home)
        return std::string();

    std::ifstream ifs(std::string(home) + "/.config/jack-bridge/devices.conf");
    if (!ifs.is_open())
        return std::string();

    std::string line, pref;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        if (line.substr(0, eq) != "PREFERRED_OUTPUT")
            continue;

        pref = line.substr(eq + 1);
        if (pref.size() >= 2 && (pref.front() == '"' || pref.front() == '\'') &&
            pref.back() == pref.front())
            pref = pref.substr(1, pref.size() - 2);
    }
    return pref;
}

// Record a live frames/period change in /etc/default/jackd-rt so the next boot starts the server
// the user is actually listening to. Only that one line is rewritten: everything else in the file
// describes the running server, and frames/period is the only value a live change can alter.
bool SettingsPanel::persistPeriod(int frames)
{
    return proc::run({"pkexec", kServiceHelper, "set-period", std::to_string(frames)}) == 0;
}

// Restart the HDMI bridge so its ALSA period matches the server's new one. alsa_out takes its
// period from a command line fixed at spawn time, so after a live change the bridge would still be
// running the old size -- exactly the mismatch that makes the HDMI device underrun and crackle.
//
// HDMI is the only one left. Internal and USB both run jackd directly on the card, so there is no
// bridge to resize; running the route helper for USB would disconnect and rebuild every connection
// in the graph to accomplish nothing. Bluetooth is left alone too: its buffer is deliberately
// decoupled from JACK, and respawning it means tearing down a live A2DP connection.
void SettingsPanel::respawnBridges()
{
    const std::string pref = preferredOutput();
    if (pref != "hdmi")
        return;

    if (proc::run({kRouteHelper, pref}) != 0) {
        std::cerr << "jack-graph: jack-route-select " << pref
                  << " failed after a live buffer change; the bridge may still be running the "
                     "previous period\n";
    }
}

// Keep the routing state in step with the card jack-graph just started jackd on, so mxeq's device
// page and this window cannot disagree.
//
// A USB interface is not bridged any more: jackd runs ON it, and PREFERRED_OUTPUT has to say "usb"
// or jack-connection-manager keeps routing clients at the sink for the previous device. The
// reverse move matters just as much -- picking a non-USB card while the saved preference is still
// "usb" would leave every app aimed at an interface jackd no longer owns.
//
// hdmi and bluetooth are deliberately left alone: they run jackd on the internal card too, so
// selecting that card here must not silently demote an HDMI selection to internal and tear down
// its bridge.
void SettingsPanel::syncPreferredOutput(const std::string &iface)
{
    const std::string pfx = "hw:CARD=";
    if (iface.size() <= pfx.size() || iface.compare(0, pfx.size(), pfx) != 0)
        return; // "default", or a numeric device: nothing to derive from

    std::string cardId = iface.substr(pfx.size());
    const size_t comma = cardId.find(',');
    if (comma != std::string::npos)
        cardId = cardId.substr(0, comma);
    if (cardId.empty())
        return;

    // /proc/asound/<id>/usbid is created by the USB audio driver and nothing else -- an exact
    // test, unlike matching "USB" in a card description.
    const bool isUsb = access(("/proc/asound/" + cardId + "/usbid").c_str(), F_OK) == 0;
    const std::string current = preferredOutput();

    const char *target = nullptr;
    if (isUsb && current != "usb")
        target = "usb";
    else if (!isUsb && current == "usb")
        target = "internal";
    if (!target)
        return;

    if (proc::run({kRouteHelper, target}) != 0) {
        std::cerr << "jack-graph: jack-route-select " << target
                  << " failed; mxeq may still show the previous output device\n";
    }
}

//------------------------------------------------------------------------
// Live frames/period change, the one setting JACK can alter without a restart.
void SettingsPanel::onApplyLive()
{
    if (!onBufferSize)
        return;

    const std::string fppStr = mFrames.value();
    if (fppStr.empty())
        return;
    const int fpp = std::atoi(fppStr.c_str());
    if (fpp <= 0)
        return;

    if (!onBufferSize(static_cast<unsigned int>(fpp))) {
        // THE SERVER REFUSED THE LIVE CHANGE, which on a USB interface it always will: jackd's
        // alsa_driver never calls snd_pcm_hw_free, so on a buffer change it does snd_pcm_drop --
        // leaving the stream in SETUP, not OPEN -- and then re-runs hw_params_any +
        // set_period_size on the same handle. snd_hda_intel tolerates that; snd_usb_audio holds
        // its committed period until the params are freed, and answers
        //   ALSA: cannot set period size to N frames for capture
        // at every size, in both directions.
        //
        // Restart and apply it, which is what qjackctl does. Its log shows the same failure
        // followed by "Stopping jack server / Starting jack server" with the new period on the
        // driver line -- fast enough over jackdbus that it reads as a live change. There is no
        // prompt here for the same reason: the button says apply, so it applies.
        if (!persistPeriod(fpp)) {
            mLiveStatus = "Could not save frames/period, so the server was not restarted.";
            repaint();
            return;
        }

        mConfig.set_frames_per_period(fpp);
        mConfig.save();

        // refresh restarts jackd-rt on whatever the file now says, and brings the bridge ports and
        // the connection manager back with it.
        if (proc::run({"pkexec", kServiceHelper, "refresh"}) != 0) {
            mLiveStatus = "Saved " + fppStr +
                          " frames/period, but restarting the server failed. Use Stop then Start.";
            repaint();
            return;
        }

        // RECONNECT FIRST. refreshRunningState() re-reads this window, and that asks the client
        // for its buffer size -- with the old server gone, the handle is still allocated but
        // points at shared memory that has been unmapped, so calling into JACK with it segfaulted
        // the whole window. onApply drops the orphaned client and opens one on the new server,
        // which has to happen before anything reads from it.
        if (onApply)
            onApply();

        // The device did not change, so the periodic refresh would short-circuit and keep showing
        // the old numbers. Force a re-read.
        mRunningStateValid = false;
        refreshRunningState();

        mLiveStatus = "Frames/period is now " + fppStr +
                      ". The interface cannot change it while running, so the server was "
                      "restarted -- JACK clients will have reconnected.";
        repaint();
        return;
    }

    mConfig.set_frames_per_period(fpp);
    mConfig.save();

    const bool persisted = persistPeriod(fpp);
    respawnBridges();

    mLiveStatus = persisted ? "Frames/period is now " + fppStr +
                                  ". Other settings still need Stop then Start."
                            : "Frames/period is now " + fppStr +
                                  ", but saving it for next boot failed; it will revert on reboot.";
    repaint();
}

void SettingsPanel::onStart()
{
    const std::string iface = mInterface.value();

    JackSettings settings;
    settings.interface = iface;
    settings.sample_rate = std::atoi(mRate.value().c_str());
    settings.frames_per_period = std::atoi(mFrames.value().c_str());
    settings.periods_per_buffer = std::atoi(mPeriods.value().c_str());
    settings.realtime = true;
    settings.synchronous = false;
    settings.midi_driver = mMidi.value();

    mConfig.set_interface(settings.interface);
    mConfig.set_sample_rate(settings.sample_rate);
    mConfig.set_frames_per_period(settings.frames_per_period);
    mConfig.set_periods_per_buffer(settings.periods_per_buffer);
    mConfig.set_midi_driver(settings.midi_driver);
    mConfig.save();

    // Start handles everything: update /etc/default/jackd-rt and start the service.
    mServer.start(settings);

    // The chosen card decides whether this is USB mode; record it so mxeq and
    // jack-connection-manager agree with what just started.
    syncPreferredOutput(iface);

    if (onApply)
        onApply();

    // THE SLEEP IS THE GTK BUILD'S OWN and it stays: start() returns as soon as the service
    // manager has been told, and asking is_running() immediately afterwards races the server's own
    // start-up. The 1 Hz tick would correct it within a second anyway; this makes the button state
    // right straight away. It blocks the window for a fifth of a second, which is the cost.
    usleep(200000);
    updateServerStatus(mServer.is_running());
    mRunningStateValid = false;
    repaint();
}

void SettingsPanel::onStop()
{
    // DISCONNECT THE JACK CLIENT FIRST, without reconnecting, so no callback can fire into a
    // server that is shutting down.
    if (onDisconnect)
        onDisconnect();

    mServer.stop();

    usleep(200000);
    updateServerStatus(mServer.is_running());
    mRunningStateValid = false;
    repaint();
}

//------------------------------------------------------------------------
float SettingsPanel::layout()
{
    const float x = geo::kSetMargin;
    const float controlX = x + geo::kSetGroupPad + geo::kSetLabelW + geo::kSetLabelGap;

    float y = geo::kSetMargin + geo::kSetGroupTitleBand;

    // --- JACK Server ---
    {
        const float rowY = y + geo::kSetGroupPad;
        mStop.rect = Rect(x + geo::kSetContentW - geo::kSetGroupPad - geo::kSetButtonW, rowY,
                          geo::kSetButtonW, geo::kPillH);
        mStart.rect = Rect(mStop.rect.x - geo::kPillGap - geo::kSetButtonW, rowY,
                           geo::kSetButtonW, geo::kPillH);
        y += geo::setServerH() + geo::kSetGroupGap + geo::kSetGroupTitleBand;
    }

    // --- Audio ---
    {
        float rowY = y + geo::kSetGroupPad;
        mInterface.setRect(Rect(controlX, rowY, geo::kSetComboW + geo::kSetRowGap +
                                                    geo::kSetApplyW,
                                geo::kComboH));
        rowY += geo::kSetRowH + geo::kSetRowGap;
        mRate.setRect(Rect(controlX, rowY, geo::kSetComboW, geo::kComboH));
        rowY += geo::kSetRowH + geo::kSetRowGap;
        mFrames.setRect(Rect(controlX, rowY, geo::kSetComboW, geo::kComboH));
        mApplyLive.rect = Rect(controlX + geo::kSetComboW + geo::kSetRowGap,
                               rowY + (geo::kComboH - geo::kPillH) * 0.5f, geo::kSetApplyW,
                               geo::kPillH);
        rowY += geo::kSetRowH + geo::kSetRowGap;
        mPeriods.setRect(Rect(controlX, rowY, geo::kSetComboW, geo::kComboH));

        y += geo::setAudioH() + geo::kSetGroupGap;
    }

    // --- the live status line, outside any box ---
    y += static_cast<float>(geo::kSetStatusLines) * geo::kSetStatusLineH + geo::kSetGroupGap;

    // --- MIDI ---
    {
        y += geo::kSetGroupTitleBand;
        mMidi.setRect(Rect(controlX, y + geo::kSetGroupPad, geo::kSetComboW, geo::kComboH));
        y += geo::setMidiH() + geo::kSetGroupGap;
    }

    mClose.rect = Rect(x + geo::kSetContentW - geo::kSetButtonW, y, geo::kSetButtonW,
                       geo::kPillH);
    y += geo::kPillH + geo::kSetMargin;

    mHeight = y;
    return mHeight;
}

//------------------------------------------------------------------------
void SettingsPanel::draw(Canvas &c) const
{
    c.setColor(pal::kBgColor);
    c.fillRect(c.bounds());

    const float x = geo::kSetMargin;
    const float labelX = x + geo::kSetGroupPad;
    const float labelW = geo::kSetLabelW;

    // --- JACK Server ---
    {
        const Rect frame(x, geo::kSetMargin + geo::kSetGroupTitleBand, geo::kSetContentW,
                         geo::setServerH());
        drawGroupBox(c, frame, "JACK Server");
        label(c, labelX, mStart.rect.centerY(), mServerStatus.c_str(),
              mStart.rect.x - labelX - geo::kSetLabelGap);
        mStart.draw(c);
        mStop.draw(c);
    }

    // --- Audio ---
    {
        const Rect frame(x, mInterface.rect().y - geo::kSetGroupPad, geo::kSetContentW,
                         geo::setAudioH());
        drawGroupBox(c, frame, "Audio");

        label(c, labelX, mInterface.rect().centerY(), "Interface:", labelW);
        label(c, labelX, mRate.rect().centerY(), "Sample Rate:", labelW);
        label(c, labelX, mFrames.rect().centerY(), "Frames/Period:", labelW);
        label(c, labelX, mPeriods.rect().centerY(), "Periods/Buffer:", labelW);

        mInterface.drawClosed(c);
        mRate.drawClosed(c);
        mFrames.drawClosed(c);
        mPeriods.drawClosed(c);
        mApplyLive.draw(c);

        // THE TOOLTIP, DRAWN. It said what Apply Live does and what it does not; there are no
        // tooltips here, and this is the better place for it anyway.
        const float noteY = mPeriods.rect().bottom() + geo::kSetRowGap;
        c.setFont(Font::Body);
        c.setFontSize(geo::kSetNoteSize);
        c.setColor(pal::kDisabledColor);
        c.drawString("Apply Live changes frames/period on the running server.",
                     labelX, noteY + geo::kSetNoteLineH * 0.5f +
                                 geo::kSetNoteSize * geo::kLabelBaselineBias);
        c.drawString("Everything else needs Stop then Start.", labelX,
                     noteY + geo::kSetNoteLineH * 1.5f +
                         geo::kSetNoteSize * geo::kLabelBaselineBias);
    }

    // --- the live status line ---
    if (!mLiveStatus.empty()) {
        const Rect area(x, mMidi.rect().y - geo::kSetGroupPad - geo::kSetGroupTitleBand -
                               geo::kSetGroupGap -
                               static_cast<float>(geo::kSetStatusLines) * geo::kSetStatusLineH,
                        geo::kSetContentW,
                        static_cast<float>(geo::kSetStatusLines) * geo::kSetStatusLineH);
        drawWrappedText(c, area, mLiveStatus, pal::kAccent, geo::kSetStatusSize,
                        geo::kSetStatusLineH, geo::kSetStatusLines);
    }

    // --- MIDI ---
    {
        const Rect frame(x, mMidi.rect().y - geo::kSetGroupPad, geo::kSetContentW,
                         geo::setMidiH());
        drawGroupBox(c, frame, "MIDI");
        label(c, labelX, mMidi.rect().centerY(), "MIDI Driver:", labelW);
        mMidi.drawClosed(c);
    }

    mClose.draw(c);

    // The popups last, over everything. Only one can be open at a time.
    for (const Combo *combo : {&mInterface, &mRate, &mFrames, &mPeriods, &mMidi}) {
        if (combo->isOpen())
            combo->drawPopup(c);
    }
}

//------------------------------------------------------------------------
SettingsPanel::Target SettingsPanel::targetAt(float px, float py) const
{
    // An open popup takes everything, including a click outside it -- which closes it and is
    // SWALLOWED, so the click does not also press whatever is under it.
    if (mInterface.isOpen())
        return Target::Interface;
    if (mRate.isOpen())
        return Target::Rate;
    if (mFrames.isOpen())
        return Target::Frames;
    if (mPeriods.isOpen())
        return Target::Periods;
    if (mMidi.isOpen())
        return Target::Midi;

    if (mStart.hit(px, py))
        return Target::Start;
    if (mStop.hit(px, py))
        return Target::Stop;
    if (mApplyLive.hit(px, py))
        return Target::ApplyLive;
    if (mClose.hit(px, py))
        return Target::CloseButton;
    if (mInterface.hitClosed(px, py))
        return Target::Interface;
    if (mRate.hitClosed(px, py))
        return Target::Rate;
    if (mFrames.hitClosed(px, py))
        return Target::Frames;
    if (mPeriods.hitClosed(px, py))
        return Target::Periods;
    if (mMidi.hitClosed(px, py))
        return Target::Midi;
    return Target::Nothing;
}

void SettingsPanel::clearHover()
{
    mStart.hovered = mStop.hovered = mApplyLive.hovered = mClose.hovered = false;
    mInterface.hovered = mRate.hovered = mFrames.hovered = mPeriods.hovered = mMidi.hovered =
        false;
}

void SettingsPanel::motion(float px, float py)
{
    for (Combo *combo : {&mInterface, &mRate, &mFrames, &mPeriods, &mMidi}) {
        if (combo->isOpen()) {
            combo->motion(px, py);
            repaint();
            return;
        }
    }

    const Target t = targetAt(px, py);
    if (t == mHoverTarget)
        return;
    mHoverTarget = t;
    clearHover();
    switch (t) {
        case Target::Start:
            mStart.hovered = true;
            break;
        case Target::Stop:
            mStop.hovered = true;
            break;
        case Target::ApplyLive:
            mApplyLive.hovered = true;
            break;
        case Target::CloseButton:
            mClose.hovered = true;
            break;
        case Target::Interface:
            mInterface.hovered = true;
            break;
        case Target::Rate:
            mRate.hovered = true;
            break;
        case Target::Frames:
            mFrames.hovered = true;
            break;
        case Target::Periods:
            mPeriods.hovered = true;
            break;
        case Target::Midi:
            mMidi.hovered = true;
            break;
        case Target::Nothing:
            break;
    }
    repaint();
}

void SettingsPanel::press(float px, float py, int button)
{
    if (button != 1)
        return;
    mPressTarget = targetAt(px, py);
}

void SettingsPanel::release(float px, float py, int button)
{
    if (button != 1)
        return;

    const Target pressed = mPressTarget;
    mPressTarget = Target::Nothing;

    // An open popup: hand the click to it and stop, whether it landed inside or outside.
    for (Combo *combo : {&mInterface, &mRate, &mFrames, &mPeriods, &mMidi}) {
        if (combo->isOpen()) {
            combo->click(px, py);
            repaint();
            return;
        }
    }

    // PAIRED: act only if the release is on the same thing the press armed.
    const Target t = targetAt(px, py);
    if (t != pressed)
        return;

    switch (t) {
        case Target::Start:
            if (mStart.enabled)
                onStart();
            break;
        case Target::Stop:
            if (mStop.enabled)
                onStop();
            break;
        case Target::ApplyLive:
            if (mApplyLive.enabled)
                onApplyLive();
            break;
        case Target::CloseButton:
            if (onClose)
                onClose();
            break;
        case Target::Interface:
            mInterface.open(Rect(0, 0, geo::kSetW, mHeight));
            repaint();
            break;
        case Target::Rate:
            mRate.open(Rect(0, 0, geo::kSetW, mHeight));
            repaint();
            break;
        case Target::Frames:
            mFrames.open(Rect(0, 0, geo::kSetW, mHeight));
            repaint();
            break;
        case Target::Periods:
            mPeriods.open(Rect(0, 0, geo::kSetW, mHeight));
            repaint();
            break;
        case Target::Midi:
            mMidi.open(Rect(0, 0, geo::kSetW, mHeight));
            repaint();
            break;
        case Target::Nothing:
            break;
    }
}

bool SettingsPanel::key(Key k, const char *, int, unsigned)
{
    for (Combo *combo : {&mInterface, &mRate, &mFrames, &mPeriods, &mMidi}) {
        if (combo->isOpen()) {
            const bool used = combo->key(k);
            repaint();
            return used;
        }
    }
    if (k == Key::Escape) {
        if (onClose)
            onClose();
        return true;
    }
    return false;
}

} // namespace jackbridge

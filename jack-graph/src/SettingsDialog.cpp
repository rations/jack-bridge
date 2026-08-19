#include "SettingsDialog.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <cstdio>

SettingsDialog::SettingsDialog(Gtk::Window& parent, JackServerControl& server, Config& config)
    : Gtk::Dialog("JACK Settings", parent, true),
      m_server(server), m_config(config),
      m_content_box(Gtk::ORIENTATION_VERTICAL, 10),
      m_server_frame("JACK Server"),
      m_server_box(Gtk::ORIENTATION_HORIZONTAL, 10),
      m_start_btn("Start"),
      m_stop_btn("Stop"),
      m_audio_frame("Audio"),
      m_interface_label("Interface:"),
      m_sample_rate_label("Sample Rate:"),
      m_frames_label("Frames/Period:"),
      m_frames_apply_btn("Apply Live"),
      m_periods_label("Periods/Buffer:"),
      m_midi_frame("MIDI"),
      m_midi_label("MIDI Driver:") {
    set_default_size(420, -1);

    add_button("Close", Gtk::RESPONSE_CLOSE);

    build_ui();
    /* Devices first: load_current_settings() calls set_active_id(JACKD_DEVICE),
     * which silently does nothing on an empty combo. With the old order the
     * interface never showed the running device, and on_start() then wrote back
     * whatever the combo happened to fall on -- /var/log/jackd-rt.log has
     * sessions that started on "default" and "hw:0,0" because of this. */
    populate_devices();
    load_current_settings();
    refresh_running_state();

    /* The device can change while this dialog is open -- mxeq is a separate
     * process, and a USB hotplug moves the server with no GUI involved at all.
     * One second is well under the time a server restart takes, and each tick
     * is one small read plus a widget update only when something differs. */
    m_refresh_timer = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &SettingsDialog::on_refresh_tick), 1000);

    show_all_children();
}

SettingsDialog::~SettingsDialog() {
    if (m_refresh_timer.connected()) m_refresh_timer.disconnect();
}

void SettingsDialog::build_ui() {
    auto* content_area = get_content_area();
    content_area->pack_start(m_content_box, true, true, 10);

    m_server_box.pack_start(m_server_status_label, true, true, 0);
    m_server_box.pack_start(m_start_btn, false, false, 0);
    m_server_box.pack_start(m_stop_btn, false, false, 0);
    m_server_frame.add(m_server_box);
    m_content_box.pack_start(m_server_frame, false, false, 0);

    m_audio_grid.attach(m_interface_label, 0, 0, 1, 1);
    m_audio_grid.attach(m_interface_combo, 1, 0, 1, 1);
    m_audio_grid.attach(m_sample_rate_label, 0, 1, 1, 1);
    m_audio_grid.attach(m_sample_rate_combo, 1, 1, 1, 1);
    m_audio_grid.attach(m_frames_label, 0, 2, 1, 1);
    m_audio_grid.attach(m_frames_combo, 1, 2, 1, 1);
    /* Beside the frames combo, not with Start/Stop: frames/period is the only
     * field this button touches, and putting it here says so without a
     * paragraph of explanation. */
    m_audio_grid.attach(m_frames_apply_btn, 2, 2, 1, 1);
    m_frames_apply_btn.set_tooltip_text(
        "Change frames/period on the running server, without stopping it.\n"
        "Sample rate, periods/buffer, interface and MIDI need Stop then Start.");
    m_audio_grid.attach(m_periods_label, 0, 3, 1, 1);
    m_audio_grid.attach(m_periods_combo, 1, 3, 1, 1);
    m_audio_grid.set_column_spacing(10);
    m_audio_grid.set_row_spacing(8);
    m_audio_frame.add(m_audio_grid);
    m_content_box.pack_start(m_audio_frame, false, false, 0);

    m_live_status_label.set_halign(Gtk::ALIGN_START);
    m_live_status_label.set_line_wrap(true);
    m_content_box.pack_start(m_live_status_label, false, false, 0);

    m_midi_grid.attach(m_midi_label, 0, 0, 1, 1);
    m_midi_grid.attach(m_midi_combo, 1, 0, 1, 1);
    m_midi_grid.set_column_spacing(10);
    m_midi_grid.set_row_spacing(8);
    m_midi_frame.add(m_midi_grid);
    m_content_box.pack_start(m_midi_frame, false, false, 0);

    m_sample_rate_combo.append("44100", "44100");
    m_sample_rate_combo.append("48000", "48000");
    m_sample_rate_combo.append("88200", "88200");
    m_sample_rate_combo.append("96000", "96000");
    m_sample_rate_combo.append("192000", "192000");

    m_frames_combo.append("64", "64");
    m_frames_combo.append("128", "128");
    m_frames_combo.append("256", "256");
    m_frames_combo.append("512", "512");
    m_frames_combo.append("1024", "1024");
    m_frames_combo.append("2048", "2048");

    m_periods_combo.append("2", "2");
    m_periods_combo.append("3", "3");
    m_periods_combo.append("4", "4");
    m_periods_combo.append("5", "5");
    m_periods_combo.append("6", "6");
    m_periods_combo.append("7", "7");
    m_periods_combo.append("8", "8");

    m_midi_combo.append("none", "None");
    m_midi_combo.append("seq", "ALSA SEQ");

    m_start_btn.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::on_start));
    m_stop_btn.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::on_stop));
    m_frames_apply_btn.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::on_apply));
}

void SettingsDialog::populate_devices() {
    m_interface_combo.remove_all();
    m_interface_combo.append("default", "default");

    std::string devices = m_server.list_audio_devices();
    std::istringstream stream(devices);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        /* list_audio_devices() returns "hw:CARD=id,DEV=n|Card Name - Device".
         * The id is what jackd is started with and what gets persisted; the
         * label is the only half the user should have to read. */
        auto pipe = line.find('|');
        if (pipe == std::string::npos) {
            m_interface_combo.append(line, line);
        } else {
            m_interface_combo.append(line.substr(0, pipe), line.substr(pipe + 1));
        }
    }
}

/* The value jackd was started with for a given flag, or empty when it is not
 * running. `flag` must include its trailing space: jackd's realtime priority is
 * "-P70" with no space, and a search for bare "-P" would return "70" as the
 * playback device.
 *
 * This is the last resort for showing the truth. /etc/default/jackd-rt is read
 * first because it is what a restart will use, and because frames/period can be
 * changed live -- set-period keeps the file current while argv keeps whatever
 * the server was launched with. For everything that cannot change without a
 * restart, argv is exact. */
std::string SettingsDialog::running_cmdline() const {
    FILE* fp = popen("ps -o args= -C jackd 2>/dev/null", "r");
    if (!fp) return "";

    std::string out;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) out = line;
    pclose(fp);
    return out;
}

/* The value jackd was started with for a given flag, or empty.
 *
 * `flag` must include its trailing space: jackd's realtime priority is "-P70"
 * with no space, and a search for a bare "-P" would return "70" as the playback
 * device. */
std::string SettingsDialog::arg_from(const std::string& cmd, const char* flag) {
    size_t at = cmd.find(flag);
    if (at == std::string::npos) return "";

    at += strlen(flag);
    size_t end = cmd.find_first_of(" \t\n", at);
    return cmd.substr(at, end == std::string::npos ? std::string::npos : end - at);
}

/* jackd-rt starts the server as `-d alsa -P <device> -C <device>`, so -P carries
 * the device. jack-route-select's server_card_id() and jack-usb-hotplug parse
 * the same thing; they must agree on the form. */
std::string SettingsDialog::running_device() const {
    return arg_from(running_cmdline(), "-P ");
}

/* Select a device in the combo, tolerating the two spellings of device 0.
 *
 * JACKD_DEVICE may hold either "hw:CARD=x" (what detect-alsa-device.sh and
 * set-device write) or "hw:CARD=x,DEV=0" (what an older jack-graph wrote, or a
 * hand edit). The list uses the bare form, so try that first and then the
 * suffixed one rather than leaving the field blank on a working machine. */
void SettingsDialog::select_interface(const std::string& id) {
    if (id.empty()) return;

    if (!m_interface_combo.set_active_id(id)) {
        const std::string dev0 = ",DEV=0";
        if (id.size() > dev0.size() &&
            id.compare(id.size() - dev0.size(), dev0.size(), dev0) == 0) {
            m_interface_combo.set_active_id(id.substr(0, id.size() - dev0.size()));
        } else {
            m_interface_combo.set_active_id(id + dev0);
        }
    }
}

/* Keep Interface showing the device jackd is actually on.
 *
 * The device is not only ours to change: mxeq is a separate process and moves it
 * whenever the user picks an output there, and a USB hotplug moves it with no
 * GUI involved at all. Without this the field kept showing whatever was true
 * when the dialog opened.
 *
 * The early return is what makes this safe as well as cheap. A selection the
 * user has made but not yet pressed Start on is only ever overwritten on a tick
 * where the running device genuinely moved -- which is new information worth
 * showing, not a refresh stomping on them.
 */
void SettingsDialog::refresh_running_state() {
    std::string dev = running_device();

    if (m_running_state_valid && dev == m_last_running_device) return;
    m_last_running_device = dev;
    m_running_state_valid = true;

    /* jackd's presence in the process table is the liveness test here, not
     * is_running(): it is already in hand, and it costs no JACK client. Calling
     * is_running() on a timer would open and close one every tick, which shows
     * up as a "status_check" client blinking in jack-graph's own canvas. */
    update_server_status(!dev.empty());

    /* Nothing to show for a stopped server: leave the combos holding whatever the
     * next Start would use. */
    if (dev.empty()) return;

    /* The whole dialog, not just Interface: a restart onto another card can
     * change frames, periods and rate too, and showing the new device beside the
     * old server's numbers would be its own kind of lie. */
    load_current_settings();
}

bool SettingsDialog::on_refresh_tick() {
    refresh_running_state();
    return true;
}

void SettingsDialog::update_server_status(bool running) {
    m_server_status_label.set_text(running ? "Status: Running" : "Status: Stopped");
    m_start_btn.set_sensitive(!running);
    m_stop_btn.set_sensitive(running);
    /* Nothing to apply to a server that is not running; Start carries the
     * settings in that case. */
    m_frames_apply_btn.set_sensitive(running);
}

void SettingsDialog::load_current_settings() {
    update_server_status(m_server.is_running());

    /* Precedence, and the reason for it: this dialog exists to show what the
     * server is doing, so the running server wins.
     *
     *   1. the running jackd  -- exact, and the only source that cannot be stale
     *   2. /etc/default/jackd-rt -- what the next start will use
     *   3. jack-graph's own config -- private memory, for when nothing is running
     *
     * Getting this backwards is what made Periods/Buffer lie. It used to read
     * the file, then jack-graph's config, and only ask the server if both were
     * silent -- so on a fresh install, where the shipped file leaves
     * JACKD_NPERIODS commented out and jackd-rt falls back to 3, the field
     * showed a remembered value while the server ran at 3. Interface was already
     * resolved server-first, which is why it alone stayed honest.
     *
     * One command line read, parsed several times: ps is a fork, and this runs
     * again on every device change. */
    const std::string cmd = running_cmdline();

    std::string dev, sr, period, nperiods, midi;
    std::ifstream ifs("/etc/default/jackd-rt");
    if (ifs.is_open()) {
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                val = val.substr(1, val.size() - 2);

            if      (key == "JACKD_DEVICE")   dev      = val;
            else if (key == "JACKD_SR")       sr       = val;
            else if (key == "JACKD_PERIOD")   period   = val;
            else if (key == "JACKD_NPERIODS") nperiods = val;
            else if (key == "JACKD_MIDI")     midi     = val;
        }
        ifs.close();
    }

    /* Interface */
    std::string v = arg_from(cmd, "-P ");
    if (v.empty()) v = dev;
    if (v.empty()) v = m_config.get_interface();
    select_interface(v);

    /* Sample rate -- cannot change without a restart, so the command line is
     * exact whenever there is a server at all. */
    v = arg_from(cmd, "-r ");
    if (v.empty()) v = sr;
    if (v.empty() && m_config.get_sample_rate() > 0)
        v = std::to_string(m_config.get_sample_rate());
    if (!v.empty()) m_sample_rate_combo.set_active_id(v);

    /* Periods/buffer -- likewise fixed for the life of the server. */
    v = arg_from(cmd, "-n ");
    if (v.empty()) v = nperiods;
    if (v.empty() && m_config.get_periods_per_buffer() > 0)
        v = std::to_string(m_config.get_periods_per_buffer());
    if (!v.empty()) m_periods_combo.set_active_id(v);

    /* Frames/period is the one value that CAN change on a running server, so the
     * order differs. Ask JACK, because after an Apply Live the command line
     * still holds the size jackd was launched with. The file comes next: it is
     * only correct here because set-period rewrites it after a successful live
     * change -- and if that write failed, the live query above has already
     * answered. */
    v.clear();
    if (m_buffer_size_query) {
        unsigned int n = m_buffer_size_query();
        if (n > 0) v = std::to_string(n);
    }
    if (v.empty()) v = period;
    if (v.empty()) v = arg_from(cmd, "-p ");
    if (v.empty() && m_config.get_frames_per_period() > 0)
        v = std::to_string(m_config.get_frames_per_period());
    if (!v.empty()) m_frames_combo.set_active_id(v);

    /* MIDI: jackd carries it as "-X seq", and omits the flag entirely for none. */
    v = arg_from(cmd, "-X ");
    if (v.empty() && !cmd.empty()) v = "none";
    if (v.empty()) v = midi;
    if (v.empty()) v = m_config.get_midi_driver();
    if (!v.empty()) m_midi_combo.set_active_id(v);
}

/* The user's saved output device, read the same way jack-route-select and
 * jack-bridge-ports read it. Empty when unset. */
std::string SettingsDialog::preferred_output() const {
    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::ifstream ifs(std::string(home) + "/.config/jack-bridge/devices.conf");
    if (!ifs.is_open()) return "";

    std::string line, pref;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (line.substr(0, eq) != "PREFERRED_OUTPUT") continue;

        pref = line.substr(eq + 1);
        if (pref.size() >= 2 && (pref.front() == '"' || pref.front() == '\'') &&
            pref.back() == pref.front()) {
            pref = pref.substr(1, pref.size() - 2);
        }
    }
    return pref;
}

/* Record a live frames/period change in /etc/default/jackd-rt so the next boot
 * starts the server the user is actually listening to. Only that one line is
 * rewritten: everything else in the file describes the running server, and
 * frames/period is the only value a live change can alter. */
bool SettingsDialog::persist_period(int frames) {
    std::string cmd = "pkexec /usr/local/lib/jack-bridge/jack-bridge-service-helper set-period "
                      + std::to_string(frames) + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

/* Restart the HDMI bridge so its ALSA period matches the server's new one.
 * alsa_out takes its period from a command line fixed at spawn time, so after a
 * live change the bridge would still be running the old size -- exactly the
 * mismatch that makes the HDMI device underrun and crackle.
 *
 * HDMI is the only one left. Internal and USB both run jackd directly on the
 * card, so there is no bridge to resize; running the route helper for USB would
 * disconnect and rebuild every connection in the graph to accomplish nothing.
 * Bluetooth is left alone too: its buffer is deliberately decoupled from JACK,
 * and respawning it means tearing down a live A2DP connection. */
void SettingsDialog::respawn_bridges() {
    std::string pref = preferred_output();
    if (pref != "hdmi") return;

    std::string cmd = "/usr/local/lib/jack-bridge/jack-route-select " + pref + " >/dev/null 2>&1";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "jack-graph: jack-route-select " << pref
                  << " failed after a live buffer change; the bridge may still be "
                     "running the previous period\n";
    }
}

/* Keep the routing state in step with the card jack-graph just started jackd on,
 * so mxeq's device buttons and this dialog cannot disagree.
 *
 * A USB interface is not bridged any more: jackd runs ON it, and PREFERRED_OUTPUT
 * has to say "usb" or jack-connection-manager keeps routing clients at the sink
 * for the previous device. The reverse move matters just as much -- picking a
 * non-USB card while the saved preference is still "usb" would leave every app
 * aimed at an interface jackd no longer owns.
 *
 * hdmi and bluetooth are deliberately left alone: they run jackd on the internal
 * card too, so selecting that card here must not silently demote an HDMI
 * selection to internal and tear down its bridge.
 */
void SettingsDialog::sync_preferred_output(const std::string& iface) {
    const std::string pfx = "hw:CARD=";
    if (iface.size() <= pfx.size() || iface.compare(0, pfx.size(), pfx) != 0)
        return;  /* "default", or a numeric device: nothing to derive from */

    std::string card_id = iface.substr(pfx.size());
    auto comma = card_id.find(',');
    if (comma != std::string::npos) card_id = card_id.substr(0, comma);
    if (card_id.empty()) return;

    /* /proc/asound/<id>/usbid is created by the USB audio driver and nothing
     * else -- an exact test, unlike matching "USB" in a card description. */
    bool is_usb = (access(("/proc/asound/" + card_id + "/usbid").c_str(), F_OK) == 0);
    std::string current = preferred_output();

    const char* target = nullptr;
    if (is_usb && current != "usb") {
        target = "usb";
    } else if (!is_usb && current == "usb") {
        target = "internal";
    }
    if (!target) return;

    std::string cmd = "/usr/local/lib/jack-bridge/jack-route-select ";
    cmd += target;
    cmd += " >/dev/null 2>&1";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "jack-graph: jack-route-select " << target
                  << " failed; mxeq may still show the previous output device\n";
    }
}

/* Live frames/period change, the one setting JACK can alter without a restart. */
void SettingsDialog::on_apply() {
    if (!m_buffer_size_cb) return;

    std::string fpp_str = m_frames_combo.get_active_id();
    if (fpp_str.empty()) return;
    int fpp = std::stoi(fpp_str);

    if (!m_buffer_size_cb(static_cast<unsigned int>(fpp))) {
        /* The server refused the live change, which on a USB interface it always
         * will: jackd's alsa_driver never calls snd_pcm_hw_free, so on a buffer
         * change it does snd_pcm_drop -- leaving the stream in SETUP, not OPEN --
         * and then re-runs hw_params_any + set_period_size on the same handle.
         * snd_hda_intel tolerates that; snd_usb_audio holds its committed period
         * until the params are freed, and answers
         *   ALSA: cannot set period size to N frames for capture
         * at every size, in both directions.
         *
         * Restart and apply it, which is what qjackctl does. Its log shows the
         * same failure followed by "Stopping jack server / Starting jack server"
         * with the new period on the driver line -- fast enough over jackdbus
         * that it reads as a live change. There is no prompt here for the same
         * reason: the button says apply, so it applies. */
        if (!persist_period(fpp)) {
            m_live_status_label.set_text(
                "Could not save frames/period, so the server was not restarted.");
            return;
        }

        m_config.set_frames_per_period(fpp);
        m_config.save();

        /* refresh restarts jackd-rt on whatever the file now says, and brings the
         * bridge ports and the connection manager back with it. */
        if (system("pkexec /usr/local/lib/jack-bridge/jack-bridge-service-helper "
                   "refresh >/dev/null 2>&1") != 0) {
            m_live_status_label.set_text(
                "Saved " + fpp_str + " frames/period, but restarting the server "
                "failed. Use Stop then Start.");
            return;
        }

        /* Reconnect FIRST. refresh_running_state() re-reads the dialog, and that
         * asks the client for its buffer size -- with the old server gone, the
         * handle is still allocated but points at shared memory that has been
         * unmapped, so calling into JACK with it segfaulted the whole window.
         * m_apply_cb() drops the orphaned client and opens one on the new
         * server, which has to happen before anything reads from it. */
        if (m_apply_cb) m_apply_cb();

        /* The device did not change, so the periodic refresh would short-circuit
         * and keep showing the old numbers. Force a re-read. */
        m_running_state_valid = false;
        refresh_running_state();

        m_live_status_label.set_text(
            "Frames/period is now " + fpp_str +
            ". The interface cannot change it while running, so the server was "
            "restarted -- JACK clients will have reconnected.");
        return;
    }

    m_config.set_frames_per_period(fpp);
    m_config.save();

    bool persisted = persist_period(fpp);
    respawn_bridges();

    m_live_status_label.set_text(
        persisted
            ? "Frames/period is now " + fpp_str +
                  ". Other settings still need Stop then Start."
            : "Frames/period is now " + fpp_str +
                  ", but saving it for next boot failed; it will revert on reboot.");
}

void SettingsDialog::on_start() {
    std::string iface = m_interface_combo.get_active_id();
    std::string sr_str = m_sample_rate_combo.get_active_id();
    std::string fpp_str = m_frames_combo.get_active_id();
    std::string ppb_str = m_periods_combo.get_active_id();
    
    JackSettings settings;
    settings.interface = iface;
    settings.sample_rate = std::stoi(sr_str);
    settings.frames_per_period = std::stoi(fpp_str);
    settings.periods_per_buffer = std::stoi(ppb_str);
    settings.realtime = true;
    settings.synchronous = false;
    settings.midi_driver = m_midi_combo.get_active_id();

    // Update internal config too
    m_config.set_interface(settings.interface);
    m_config.set_sample_rate(settings.sample_rate);
    m_config.set_frames_per_period(settings.frames_per_period);
    m_config.set_periods_per_buffer(settings.periods_per_buffer);
    m_config.set_midi_driver(settings.midi_driver);
    m_config.save();

    // Start button handles everything: update /etc/default/jackd-rt and start service
    m_server.start(settings);

    // The chosen card decides whether this is USB mode; record it so mxeq and
    // jack-connection-manager agree with what just started.
    sync_preferred_output(iface);

    // Notify main window to reconnect and refresh
    if (m_apply_cb) {
        m_apply_cb();
    }

    // Update button states
    usleep(200000);
    update_server_status(m_server.is_running());
}

void SettingsDialog::on_stop() {
    // Disconnect JACK client FIRST (without reconnecting) to prevent callbacks during server shutdown
    if (m_disconnect_cb) {
        m_disconnect_cb();
    }

    // Now stop the server
    m_server.stop();

    // Update button states
    usleep(200000);
    update_server_status(m_server.is_running());
}

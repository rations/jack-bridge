#pragma once

#include <gtkmm.h>
#include <functional>
#include "JackServerControl.hpp"
#include "Config.hpp"

class SettingsDialog : public Gtk::Dialog {
public:
    SettingsDialog(Gtk::Window& parent, JackServerControl& server, Config& config);
    ~SettingsDialog() override;

    using ApplyCallback = std::function<void()>;
    /* Returns false if the running server refused the new frames/period. */
    using BufferSizeCallback = std::function<bool(unsigned int)>;
    void set_apply_callback(ApplyCallback cb) { m_apply_cb = std::move(cb); }
    void set_disconnect_callback(ApplyCallback cb) { m_disconnect_cb = std::move(cb); }
    void set_buffer_size_callback(BufferSizeCallback cb) { m_buffer_size_cb = std::move(cb); }
    /* Asks the running server its current frames/period. Only JACK knows this
     * after a live change -- the command line keeps the value jackd was launched
     * with. Returns 0 when there is no server to ask. */
    using BufferSizeQuery = std::function<unsigned int()>;
    void set_buffer_size_query(BufferSizeQuery cb) { m_buffer_size_query = std::move(cb); }
    /* Re-read everything. JackGraph calls this once after wiring the callbacks
     * above, because the constructor ran before they existed. */
    void reload() { load_current_settings(); }

private:
    void build_ui();
    void populate_devices();
    void load_current_settings();
    void update_server_status(bool running);
    void on_apply();
    void on_start();
    void on_stop();
    bool persist_period(int frames);
    void respawn_bridges();
    std::string preferred_output() const;
    void sync_preferred_output(const std::string& iface);
    std::string running_cmdline() const;
    static std::string arg_from(const std::string& cmd, const char* flag);
    std::string running_device() const;
    void select_interface(const std::string& id);
    void refresh_running_state();
    bool on_refresh_tick();

    JackServerControl& m_server;
    Config& m_config;

    Gtk::Box m_content_box;

    Gtk::Frame m_server_frame;
    Gtk::Box m_server_box;
    Gtk::Label m_server_status_label;
    Gtk::Button m_start_btn;
    Gtk::Button m_stop_btn;

    Gtk::Frame m_audio_frame;
    Gtk::Grid m_audio_grid;
    Gtk::Label m_interface_label;
    Gtk::ComboBoxText m_interface_combo;
    Gtk::Label m_sample_rate_label;
    Gtk::ComboBoxText m_sample_rate_combo;
    Gtk::Label m_frames_label;
    Gtk::ComboBoxText m_frames_combo;
    Gtk::Button m_frames_apply_btn;
    Gtk::Label m_periods_label;
    Gtk::ComboBoxText m_periods_combo;
    Gtk::Label m_live_status_label;

    Gtk::Frame m_midi_frame;
    Gtk::Grid m_midi_grid;
    Gtk::Label m_midi_label;
    Gtk::ComboBoxText m_midi_combo;

    sigc::connection m_refresh_timer;
    /* Last device the refresh saw, so a tick where nothing changed costs one
     * process read and stops there. It is also what protects a selection the
     * user has made but not yet started: the combo is only ever re-synced on a
     * tick where the running device genuinely moved. */
    std::string m_last_running_device;
    bool m_running_state_valid = false;

    BufferSizeQuery m_buffer_size_query;
    ApplyCallback m_apply_cb;
    ApplyCallback m_disconnect_cb;
    BufferSizeCallback m_buffer_size_cb;
};

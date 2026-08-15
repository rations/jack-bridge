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

    ApplyCallback m_apply_cb;
    ApplyCallback m_disconnect_cb;
    BufferSizeCallback m_buffer_size_cb;
};

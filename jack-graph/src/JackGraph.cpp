#include "JackGraph.hpp"
#include <iostream>

JackGraph::JackGraph()
    : m_main_box(Gtk::ORIENTATION_VERTICAL),
      m_jack_connected(false), m_alsa_connected(false) {
    set_title("Jack Graph");
    set_default_size(1200, 800);

    m_config.load();

    setup_ui();
    setup_menu();

    /* Connected before the first client open: the dispatcher is what carries a
     * server shutdown from JACK's thread to this one, and it has to exist before
     * anything can emit it. */
    m_jack_shutdown_dispatcher.connect(sigc::mem_fun(*this, &JackGraph::on_jack_server_gone));

    // Connect to JACK if it's already running (system service)
    m_jack_connected = m_jack.connect("jack-graph");
    if (m_jack_connected) {
        std::cerr << "jack-graph: Connected to running JACK server" << std::endl;
        attach_jack_callbacks();
    } else {
        std::cerr << "jack-graph: JACK not running. Use Settings to start it." << std::endl;
    }

    m_alsa_connected = m_alsa.connect("jack-graph");

    refresh_ports();
    update_status_bar();

    show_all_children();

    /* Fit all nodes into the visible viewport on first launch.
     * Deferred via signal_idle so GTK has finished allocating widget sizes. */
    Glib::signal_idle().connect_once([this]() {
        m_canvas.fit_to_window();
    });
}

JackGraph::~JackGraph() {
    /* A pending reconnect poll would fire into a half-destroyed window. */
    if (m_reconnect_timer.connected()) m_reconnect_timer.disconnect();
    m_config.save();
}

void JackGraph::attach_jack_callbacks() {
    m_jack.set_port_callback([this]() {
        bool expected = false;
        if (m_refresh_pending.compare_exchange_strong(expected, true)) {
            Glib::signal_timeout().connect([this]() -> bool {
                m_refresh_pending.store(false);
                refresh_ports();
                return false;
            }, 100);
        }
    });
    m_jack.set_xrun_callback([this]() {
        Glib::signal_idle().connect_once([this]() {
            update_status_bar();
        });
    });
    m_jack.set_shutdown_callback([this]() {
        /* JACK's thread. Glib::Dispatcher is the thread-safe hop to the main
         * loop; nothing else may be touched from here. */
        m_jack_shutdown_dispatcher.emit();
    });
}

/* The server went away without this window asking it to.
 *
 * That is now a routine event rather than a crash: selecting a USB interface
 * restarts jackd on the interface, and so does unplugging or replugging one.
 * Before this handler the canvas simply froze on a dead client until the user
 * noticed and hit Refresh. */
void JackGraph::on_jack_server_gone() {
    if (!m_jack.is_connected()) return;  /* already torn down */

    std::cerr << "jack-graph: JACK server went away; waiting for it to return" << std::endl;
    m_jack.disconnect();
    m_jack_connected = false;
    refresh_ports();
    update_status_bar();

    m_reconnect_attempts = 0;
    if (m_reconnect_timer.connected()) m_reconnect_timer.disconnect();
    m_reconnect_timer = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &JackGraph::try_reconnect_jack), 500);
}

bool JackGraph::try_reconnect_jack() {
    if (m_jack_connected) return false;

    if (m_jack.connect("jack-graph")) {
        m_jack_connected = true;
        attach_jack_callbacks();
        refresh_ports();
        update_status_bar();
        std::cerr << "jack-graph: reconnected to JACK" << std::endl;
        return false;
    }

    /* 60 * 500ms = 30s, which comfortably covers a device switch (jackd-rt
     * restart plus the interface's own warm-up). A server still absent after
     * that was stopped deliberately, not restarted, so stop polling and leave it
     * to Settings -> Start. */
    if (++m_reconnect_attempts >= 60) {
        std::cerr << "jack-graph: JACK did not return within 30s; stopping reconnect polling"
                  << std::endl;
        update_status_bar();
        return false;
    }
    return true;
}

void JackGraph::setup_ui() {
    add(m_main_box);

    m_main_box.pack_start(m_menu_bar, Gtk::PACK_SHRINK);

    m_scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_scrolled_window.add(m_canvas);
    m_main_box.pack_start(m_scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    auto* btn_bar = Gtk::manage(new Gtk::ButtonBox(Gtk::ORIENTATION_HORIZONTAL));
    btn_bar->set_layout(Gtk::BUTTONBOX_CENTER);
    btn_bar->set_spacing(8);
    auto* btn_refresh = Gtk::manage(new Gtk::Button("Refresh"));
    auto* btn_settings = Gtk::manage(new Gtk::Button("JACK Settings"));
    btn_refresh->signal_clicked().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_refresh));
    btn_settings->signal_clicked().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_settings));
    btn_bar->add(*btn_refresh);
    btn_bar->add(*btn_settings);
    m_main_box.pack_start(*btn_bar, Gtk::PACK_SHRINK);

    m_statusbar.set_halign(Gtk::ALIGN_CENTER);
    m_statusbar.set_hexpand(true);
    m_main_box.pack_start(m_statusbar, Gtk::PACK_SHRINK);

    m_canvas.set_connect_callback(
        [this](const std::string& src, const std::string& dst) {
            on_connect_jack(src, dst);
        });

    m_canvas.set_disconnect_callback(
        [this](const std::string& src, const std::string& dst) {
            on_disconnect_jack(src, dst);
        });
}

void JackGraph::setup_menu() {
    auto file_menu = Gtk::manage(new Gtk::Menu());
    auto file_item = Gtk::manage(new Gtk::MenuItem("_File", true));
    file_item->set_submenu(*file_menu);
    m_menu_bar.append(*file_item);

    auto refresh_item = Gtk::manage(new Gtk::MenuItem("_Refresh", true));
    refresh_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_refresh));
    file_menu->append(*refresh_item);

    auto sep = Gtk::manage(new Gtk::SeparatorMenuItem());
    file_menu->append(*sep);

    auto quit_item = Gtk::manage(new Gtk::MenuItem("_Quit", true));
    quit_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_quit));
    file_menu->append(*quit_item);

    auto view_menu = Gtk::manage(new Gtk::Menu());
    auto view_item = Gtk::manage(new Gtk::MenuItem("_View", true));
    view_item->set_submenu(*view_menu);
    m_menu_bar.append(*view_item);

    auto zoom_in_item = Gtk::manage(new Gtk::MenuItem("Zoom _In", true));
    zoom_in_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_zoom_in));
    view_menu->append(*zoom_in_item);

    auto zoom_out_item = Gtk::manage(new Gtk::MenuItem("Zoom _Out", true));
    zoom_out_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_zoom_out));
    view_menu->append(*zoom_out_item);

    auto zoom_normal_item = Gtk::manage(new Gtk::MenuItem("Zoom _Normal", true));
    zoom_normal_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_zoom_normal));
    view_menu->append(*zoom_normal_item);

    auto settings_menu = Gtk::manage(new Gtk::Menu());
    auto settings_item = Gtk::manage(new Gtk::MenuItem("_Settings", true));
    settings_item->set_submenu(*settings_menu);
    m_menu_bar.append(*settings_item);

    auto jack_settings_item = Gtk::manage(new Gtk::MenuItem("_JACK Settings", true));
    jack_settings_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_settings));
    settings_menu->append(*jack_settings_item);

    auto help_menu = Gtk::manage(new Gtk::Menu());
    auto help_item = Gtk::manage(new Gtk::MenuItem("_Help", true));
    help_item->set_submenu(*help_menu);
    m_menu_bar.append(*help_item);

    auto about_item = Gtk::manage(new Gtk::MenuItem("_About", true));
    about_item->signal_activate().connect(
        sigc::mem_fun(*this, &JackGraph::on_menu_about));
    help_menu->append(*about_item);

}

void JackGraph::refresh_ports() {
    m_jack.scan_ports();
    m_canvas.remove_all();

    if (m_jack_connected) {
        std::string our_client = m_jack.get_actual_client_name();
        auto ports = m_jack.get_ports();
        for (const auto& p : ports) {
            if (p.client == our_client) continue;
            auto node = std::make_shared<Node>(
                p.name,
                p.is_audio ? PortType::AUDIO : PortType::MIDI,
                p.is_output ? PortDirection::OUTPUT : PortDirection::INPUT);
            m_canvas.add_node(node);
        }

        auto conns = m_jack.get_connections();
        auto all_nodes = m_canvas.get_nodes();
        for (const auto& c : conns) {
            std::shared_ptr<Node> src_node, dst_node;
            for (auto& n : all_nodes) {
                if (n->full_name() == c.source) src_node = n;
                if (n->full_name() == c.destination) dst_node = n;
            }
            if (src_node && dst_node) {
                auto conn = std::make_shared<Connection>(src_node, dst_node, src_node->type);
                m_canvas.add_connection(conn);
            }
        }
    }

    /* Only enumerate ALSA MIDI ports when JACK is not connected.
     * When JACK is running it already bridges all ALSA MIDI devices via
     * jack_get_ports above; adding them again from ALSA produces phantom
     * duplicate ports (e.g. Midi-Through shows 3 ports instead of 2). */
    if (m_alsa_connected && !m_jack_connected) {
        auto ports = m_alsa.get_ports();
        for (const auto& p : ports) {
            std::string full_name = p.client + ":" + p.name;
            auto node = std::make_shared<Node>(
                full_name,
                PortType::MIDI,
                p.is_output ? PortDirection::OUTPUT : PortDirection::INPUT,
                true);
            m_canvas.add_node(node);
        }
    }

    m_canvas.layout(true);
    update_status_bar();
}

void JackGraph::on_connect_jack(const std::string& source, const std::string& dest) {
    if (m_jack_connected) {
        m_jack.connect_ports(source, dest);
    }
}

void JackGraph::on_disconnect_jack(const std::string& source, const std::string& dest) {
    if (m_jack_connected) {
        m_jack.disconnect_ports(source, dest);
    }
}

void JackGraph::on_connect_alsa(const std::string& source, const std::string& dest) {
    (void)source;
    (void)dest;
}

void JackGraph::on_disconnect_alsa(const std::string& source, const std::string& dest) {
    (void)source;
    (void)dest;
}

void JackGraph::on_menu_refresh() {
    refresh_ports();
    m_canvas.fit_to_window();
}

void JackGraph::on_menu_zoom_in() {
    m_canvas.set_zoom(m_canvas.get_zoom() * 1.2);
}

void JackGraph::on_menu_zoom_out() {
    m_canvas.set_zoom(m_canvas.get_zoom() / 1.2);
}

void JackGraph::on_menu_zoom_normal() {
    m_canvas.set_zoom(1.0);
}

void JackGraph::on_menu_settings() {
    SettingsDialog dialog(*this, m_server, m_config);
    
    // Callback for Start (reconnect after server starts)
    dialog.set_apply_callback([this]() {
        if (m_jack.is_connected()) {
            m_jack.disconnect();
            m_jack_connected = false;
        }
        if (m_server.is_running()) {
            m_jack_connected = m_jack.connect("jack-graph");
            if (m_jack_connected) {
                attach_jack_callbacks();
                refresh_ports();
            }
        }
        update_status_bar();
    });
    
    // Callback for a live frames/period change (no server restart)
    dialog.set_buffer_size_callback([this](unsigned int nframes) -> bool {
        if (!m_jack.is_connected()) return false;
        if (!m_jack.set_buffer_size(nframes)) return false;
        update_status_bar();
        return true;
    });

    // Callback for Stop (just disconnect, don't reconnect)
    dialog.set_disconnect_callback([this]() {
        if (m_jack.is_connected()) {
            m_jack.disconnect();
            m_jack_connected = false;
        }
        refresh_ports();
        update_status_bar();
    });
    
    /* The constructor read the server before these callbacks existed, so the
     * frames/period query had nothing to ask. Re-read now that it does. */
    dialog.set_buffer_size_query([this]() -> unsigned int {
        return m_jack.is_connected() ? m_jack.get_buffer_size() : 0;
    });
    dialog.reload();

    dialog.run();
}

void JackGraph::on_menu_about() {
    Gtk::AboutDialog dialog;
    dialog.set_program_name("Jack Graph");
    dialog.set_version("0.1.0");
    dialog.set_comments("JACK and ALSA port connection manager");
    dialog.set_copyright("GPL-3.0+");
    dialog.set_transient_for(*this);
    dialog.run();
}

bool JackGraph::on_delete_event(GdkEventAny*) {
    m_jack.disconnect();
    m_jack_connected = false;
    if (auto app = get_application()) {
        app->quit();
    }
    return false;
}

void JackGraph::on_menu_quit() {
    on_delete_event(nullptr);
}

void JackGraph::update_status_bar() {
    std::string status;

    if (m_jack_connected) {
        status += "JACK: connected";
        status += " | Buffer: " + std::to_string(m_jack.get_buffer_size()) + " frames";
        status += " | Rate: " + std::to_string(m_jack.get_sample_rate()) + " Hz";
        status += " | Xruns: " + std::to_string(m_jack.get_xrun_count());
    } else {
        status += "JACK: not connected";
    }

    status += " | Server: " + m_server.get_status();

    if (m_alsa_connected) {
        status += " | ALSA MIDI: connected";
    }

    m_statusbar.set_text(status);
}

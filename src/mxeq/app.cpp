// See app.h.

#include "app.h"

#include "gfx/palette.h"

#include <cstdio>

namespace jackbridge
{

namespace
{

// The two dropdowns on the recorder page. The LABEL is what is shown and the VALUE is what the app
// matches on -- combo.h records why that distinction exists, and it is the same reason the GTK build
// used GtkComboBoxText ids.
std::vector<ComboItem> channelItems()
{
    return {{"Mono", "1"}, {"Stereo", "2"}};
}
std::vector<ComboItem> rateItems()
{
    return {{"44100 Hz", "44100"}, {"48000 Hz", "48000"}};
}

} // namespace

//------------------------------------------------------------------------
App::App() = default;

// SEVER FIRST, CLOSE SECOND. Both closes below fire callbacks -- snd_mixer_close() invalidates the
// poll descriptors and says so, and dropping the D-Bus connection hands back every watch libdbus
// owns -- and neither has anywhere valid to report to once main() is unwinding. app.h records the
// AddressSanitizer report that this came from.
void App::shutdown()
{
    onHeightChanged = nullptr;
    onNeedsRepaint = nullptr;
    onMixerDescriptorsChanged = nullptr;
    mDevices.addTimer = nullptr;
    mDevices.removeTimer = nullptr;
    mBluez.onWatchesChanged = nullptr;
    mPanel.onNeedsRepaint = nullptr;

    // Unregisters the pairing agent while the connection is still up, which is what stops a fast
    // restart meeting "Already Exists" from RegisterAgent.
    mBluez.close();
    mMixer.close();
}

void App::requestHeight()
{
    if (onHeightChanged)
        onHeightChanged(mPanel.layout());
}

void App::message(const std::string &text, bool isError)
{
    mPanel.showMessage(text, isError);
    requestHeight();
}

//------------------------------------------------------------------------
bool App::start()
{
    mPanel.onNeedsRepaint = [this] {
        if (onNeedsRepaint)
            onNeedsRepaint();
    };

    //--- the mixer ------------------------------------------------------
    mMixer.onChanged = [this] {
        // AN EXTERNAL CHANGE. Rebuilding the page would destroy the strip the pointer is on, so the
        // values are refreshed in place -- and the strip being DRAGGED is skipped, because every
        // setVolume we make comes back through here a moment later and would otherwise fight the
        // pointer. Audio-Gui's panel exposes dragging() for exactly this.
        for (Panel::Strip &s : mPanel.mixerStrips()) {
            if (s.element < 0 || s.element >= static_cast<int>(mElements.size()))
                continue;
            const AlsaMixer::Element &e = mElements[static_cast<size_t>(s.element)];
            if (!mPanel.dragging())
                s.slider.value = mMixer.volume(e);
            if (s.hasSwitch) {
                const bool alsaOn = mMixer.switchOn(e);
                s.switchBox.on = s.switchBox.label == "Mute" ? !alsaOn : alsaOn;
            }
        }
        for (Panel::SwitchCell &s : mPanel.mixerSwitches()) {
            if (s.element < 0 || s.element >= static_cast<int>(mElements.size()))
                continue;
            s.toggle.on = mMixer.switchOn(mElements[static_cast<size_t>(s.element)]);
        }
        for (Panel::EnumCell &e : mPanel.mixerEnums()) {
            if (e.element < 0 || e.element >= static_cast<int>(mElements.size()))
                continue;
            e.combo.setIndex(mMixer.enumIndex(mElements[static_cast<size_t>(e.element)]));
        }
        if (onNeedsRepaint)
            onNeedsRepaint();
    };

    mMixer.onDescriptorsChanged = [this] {
        if (onMixerDescriptorsChanged)
            onMixerDescriptorsChanged();
    };

    //--- the output device ----------------------------------------------
    mDevices.onMessage = [this](const std::string &m, bool err) { message(m, err); };
    mDevices.onChanged = [this] {
        rebuildDevicesPage();
        requestHeight();
    };
    mDevices.onMixerCard = [this](int card, bool curate) {
        // ONE CALL REOPENS THE CARD AND REBUILDS THE PAGE. Splitting them would leave the panel
        // holding element indices into a list whose snd_mixer_elem_t pointers had just been
        // invalidated by snd_mixer_close().
        if (mMixer.reopen(card, curate))
            rebuildMixerPage();
        else
            mPanel.setMixerPlaceholder("Could not open the mixer for card " +
                                       std::to_string(card) + ".");
        requestHeight();
    };
    mDevices.onMixerPlaceholder = [this](const std::string &text) {
        // The card is closed too: there is no mixer to poll, and leaving the old card's descriptors
        // registered would keep waking the loop for a device nobody is looking at.
        mMixer.close();
        if (onMixerDescriptorsChanged)
            onMixerDescriptorsChanged();
        mPanel.setMixerPlaceholder(text);
        requestHeight();
    };

    //--- the recorder ---------------------------------------------------
    mRecorder.onMessage = [this](const std::string &m, bool err) { message(m, err); };
    mRecorder.onFinished = [this] {
        refreshRecorderPage();
        requestHeight();
    };

    //--- the Steam bridge -----------------------------------------------
    mSteam.onMessage = [this](const std::string &m, bool err) { message(m, err); };
    mSteam.onChanged = [this] {
        refreshSteamPage();
        requestHeight();
    };

    //--- Bluetooth ------------------------------------------------------
    mBluez.onDevice = [this](const BluezDeviceProps &p) {
        BtDevice d;
        d.path = p.path;
        d.name = p.name;
        d.paired = p.paired;
        d.trusted = p.trusted;
        d.connected = p.connected;
        mBtModel.upsert(d);
        rebuildBluetoothPage();
    };
    mBluez.onDeviceRemoved = [this](const std::string &path) {
        if (mBtModel.remove(path))
            rebuildBluetoothPage();
    };
    mBluez.onAdapterChanged = [this] { rebuildBluetoothPage(); };
    mBluez.onOperation = [this](bool ok, const std::string &m) {
        // SUCCESS IS QUIET, failures carry BlueZ's own text. That is the GTK build's bt_op_cb
        // behaviour, and it is right: a message strip that reports every success is a strip nobody
        // reads when it reports a failure.
        if (!ok && !m.empty())
            message(m, true);
        rebuildBluetoothPage();
    };
    mBluez.onAgentPrompt = [this](const std::string &prompt) { message(prompt, false); };

    //--- panel events ---------------------------------------------------
    // EVERY PAGE IS A DIFFERENT HEIGHT and the window follows the active one, so this is not
    // optional plumbing: without it the window keeps whatever height the previous page needed and
    // a taller page is simply cut off at the bottom -- which is what the GTK build's
    // on_any_expander_toggled() existed to prevent, with six hand-tuned constants where this has
    // one call.
    mPanel.cb.pageChanged = [this](Panel::Page) { requestHeight(); };

    mPanel.cb.setVolume = [this](int element, int percent) {
        if (element >= 0 && element < static_cast<int>(mElements.size()))
            mMixer.setVolume(mElements[static_cast<size_t>(element)], percent);
    };
    mPanel.cb.setSwitch = [this](int element, bool on) {
        if (element >= 0 && element < static_cast<int>(mElements.size()))
            mMixer.setSwitchOn(mElements[static_cast<size_t>(element)], on);
    };
    mPanel.cb.setEnum = [this](int element, int index) {
        if (element >= 0 && element < static_cast<int>(mElements.size()))
            mMixer.setEnumIndex(mElements[static_cast<size_t>(element)], index);
    };

    mPanel.cb.selectDevice = [this](int index) {
        static const Devices::Output kOrder[] = {Devices::Output::Internal, Devices::Output::USB,
                                                Devices::Output::HDMI,
                                                Devices::Output::Bluetooth};
        if (index < 0 || index >= 4)
            return;
        mPanel.clearMessage();
        mDevices.select(kOrder[index]);
        rebuildDevicesPage();
        requestHeight();
    };

    mPanel.cb.startRecording = [this] {
        Recorder::Settings s;
        s.filename = mPanel.recorderFilename();
        s.channels = mPanel.recorderChannels();
        s.rate = mPanel.recorderRate();
        mPanel.clearMessage();
        mRecorder.start(s);
        refreshRecorderPage();
        requestHeight();
    };
    mPanel.cb.stopRecording = [this] { mRecorder.stop(); };

    mPanel.cb.setDiscoverable = [this](bool on) {
        mPanel.clearMessage();
        mBluez.setDiscoverable(on);
        rebuildBluetoothPage();
    };
    mPanel.cb.startScan = [this] {
        mPanel.clearMessage();
        mBluez.startDiscovery();
        rebuildBluetoothPage();
    };
    mPanel.cb.stopScan = [this] {
        mBluez.stopDiscovery();
        rebuildBluetoothPage();
    };
    mPanel.cb.deviceSelected = [this](const std::string &path) {
        mBtSelected = path;
        // The Devices page's Bluetooth radio routes to whatever is selected here, which is what
        // bt_target_mac() read out of the tree.
        mDevices.setBluetoothSelection(path);
        rebuildBluetoothPage();
    };
    mPanel.cb.pairDevice = [this](const std::string &path) {
        mPanel.clearMessage();
        mBluez.pair(path);
    };
    mPanel.cb.trustDevice = [this](const std::string &path) {
        mPanel.clearMessage();
        mBluez.setTrusted(path, true);
    };
    mPanel.cb.connectDevice = [this](const std::string &path) {
        mPanel.clearMessage();
        mBluez.connect(path);
    };
    mPanel.cb.removeDevice = [this](const std::string &path) {
        mPanel.clearMessage();
        mBluez.removeDevice(path);
        mBtModel.remove(path);
        rebuildBluetoothPage();
    };
    mPanel.cb.setBtOutput = [this](const std::string &path) {
        mPanel.clearMessage();
        mDevices.setBluetoothSelection(path);
        mDevices.select(Devices::Output::Bluetooth);
        rebuildDevicesPage();
        requestHeight();
    };

    mPanel.cb.toggleSteam = [this] {
        mPanel.clearMessage();
        mSteam.toggle();
        refreshSteamPage();
    };

    //--- initial content ------------------------------------------------
    // The ALSA include block, before anything else touches the recorder: it is what makes recording
    // work without root.
    Devices::ensureAsoundrcBootstrap();

    mPanel.setRecorderCombos(channelItems(), rateItems());
    mPanel.setRecorderFilename(Recorder::defaultFilename());
    refreshRecorderPage();
    refreshSteamPage();

    // initialise() settles which output is live and calls onMixerCard or onMixerPlaceholder, so the
    // mixer page is built by this.
    mDevices.initialise();
    rebuildDevicesPage();

    if (!mBluez.open()) {
        // NOT FATAL, and deliberately not even a message: a machine with no Bluetooth hardware
        // would otherwise greet every user with an error about it. The page says so itself.
        fprintf(stderr, "jack-bridge: Bluetooth is unavailable; continuing without it\n");
    }
    for (const BluezDeviceProps &p : mBluez.knownDevices()) {
        BtDevice d;
        d.path = p.path;
        d.name = p.name;
        d.paired = p.paired;
        d.trusted = p.trusted;
        d.connected = p.connected;
        mBtModel.upsert(d);
    }
    mBtModel.setFavourite(Devices::loadBluetoothDeviceMac());
    rebuildBluetoothPage();

    requestHeight();
    return true;
}

//------------------------------------------------------------------------
void App::rebuildMixerPage()
{
    mElements = mMixer.elements();

    std::vector<Panel::Strip> strips;
    std::vector<Panel::SwitchCell> switches;
    std::vector<Panel::EnumCell> enums;

    for (size_t i = 0; i < mElements.size(); ++i) {
        const AlsaMixer::Element &e = mElements[i];
        const int index = static_cast<int>(i);

        switch (e.kind) {
            case AlsaMixer::Kind::Slider: {
                Panel::Strip s;
                s.label = e.name;
                s.element = index;
                s.slider.value = mMixer.volume(e);
                s.slider.enabled = true;

                // ROLES 1 AND 2, AND THE SAME PLACE ON EVERY CARD. A playback element with a
                // switch gets a box labelled "Mute", which is INVERTED against ALSA -- the box
                // says muted, ALSA stores playing, and this is the only spot that knows it drew
                // the word "Mute". A capture element with a switch gets "Enable", which is not
                // inverted.
                if (e.hasSwitch) {
                    s.hasSwitch = true;
                    s.switchBox.label = e.isCapture ? "Enable" : "Mute";
                    s.switchBox.on = e.isCapture ? mMixer.switchOn(e) : !mMixer.switchOn(e);
                }
                strips.push_back(std::move(s));
                break;
            }

            case AlsaMixer::Kind::Switch: {
                // ROLE 3: a switch-only control -- IEC958, S/PDIF -- labelled with its own name,
                // because that name is the only thing on screen saying what the box does. It has
                // no slider to sit beside, so the panel gives it a row of its own, in card order.
                Panel::SwitchCell sc;
                sc.element = index;
                sc.toggle.label = e.name;
                sc.toggle.on = mMixer.switchOn(e);
                switches.push_back(std::move(sc));
                break;
            }

            case AlsaMixer::Kind::Enum: {
                Panel::EnumCell ec;
                ec.element = index;
                ec.label = e.name;
                std::vector<ComboItem> items;
                for (const std::string &name : mMixer.enumItems(e))
                    items.push_back({name, name});
                ec.combo.setItems(std::move(items));
                ec.combo.setIndex(mMixer.enumIndex(e));
                enums.push_back(std::move(ec));
                break;
            }
        }
    }

    mPanel.setMixer(std::move(strips), std::move(switches), std::move(enums));
}

//------------------------------------------------------------------------
void App::rebuildDevicesPage()
{
    static const Devices::Output kOrder[] = {Devices::Output::Internal, Devices::Output::USB,
                                            Devices::Output::HDMI, Devices::Output::Bluetooth};
    static const char *const kLabels[] = {"Internal", "USB", "HDMI", "Bluetooth"};

    std::vector<Panel::DeviceEntry> entries;
    int selected = 0;
    for (int i = 0; i < 4; ++i) {
        Panel::DeviceEntry e;
        e.label = kLabels[i];
        e.detail = mDevices.detail(kOrder[i]);
        e.enabled = mDevices.available(kOrder[i]);
        entries.push_back(std::move(e));
        if (mDevices.selected() == kOrder[i])
            selected = i;
    }
    mPanel.setDevices(std::move(entries), selected);
}

//------------------------------------------------------------------------
void App::rebuildBluetoothPage()
{
    std::vector<ListRow> rows;
    for (const BtDevice &d : mBtModel.sorted()) {
        ListRow r;
        r.id = d.path;
        // The star is a prefix on the NAME here only because it belongs to the name's slot; it is a
        // separate bool in the model, which is the whole point. The GTK build kept it in the label
        // and had to parse it back off on every update.
        r.label = (d.favourite ? "\xE2\x98\x85 " : "") + d.name;
        // THE STATE IS CHIPS, NOT TEXT APPENDED TO THE NAME. See btmodel.h.
        if (d.connected)
            r.chips.push_back({"Connected", pal::kAccent});
        if (d.paired)
            r.chips.push_back({"Paired", pal::kDimColor});
        if (d.trusted)
            r.chips.push_back({"Trusted", pal::kGold});
        rows.push_back(std::move(r));
    }
    mPanel.setBluetoothDevices(std::move(rows));

    mPanel.setBluetoothState(mBluez.adapterReady(), mBluez.discoverable(), mBluez.discovering());

    // The action gating, from the selected device's live state. Re-read rather than taken from the
    // model, because the model is only as fresh as the last signal and this is what the GTK build
    // checked with gui_bt_get_device_state immediately before each call.
    bool paired = false, trusted = false, connected = false;
    const bool have = !mBtSelected.empty() &&
                      mBluez.deviceState(mBtSelected, &paired, &trusted, &connected);
    mPanel.setBluetoothSelectionState(have, paired, trusted, connected);
}

//------------------------------------------------------------------------
void App::refreshSteamPage()
{
    mPanel.setSteamState(mSteam.isActive(), mSteam.statusText());
}

void App::refreshRecorderPage()
{
    if (mRecorder.isRecording()) {
        const int s = mRecorder.elapsedSeconds();
        char buf[64];
        snprintf(buf, sizeof(buf), "Recording... %02d:%02d", s / 60, s % 60);
        mPanel.setRecorderState(true, buf);
    } else {
        mPanel.setRecorderState(false, "Idle");
    }
}

//------------------------------------------------------------------------
void App::onMixerReadable()
{
    mMixer.handleEvents(); // drains ALSA's queue and fires onChanged
}

void App::tickDevices()
{
    mDevices.poll();
}

void App::tickRecorder()
{
    if (!mRecorder.isRecording())
        return;
    refreshRecorderPage();
    if (onNeedsRepaint)
        onNeedsRepaint();
}

} // namespace jackbridge

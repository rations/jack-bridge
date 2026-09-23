// The mxeq window's five pages: layout, drawing and hit-testing.
//
// CAIRO ONLY. No Xlib, no libasound, no BlueZ, no knowledge of subprocesses. It is handed display
// strings and widget states by app.cpp and hands back semantic events -- which is what lets
// tools/uirender compose and audit the real pages with no X server, no sound card and no Bluetooth
// adapter. That last part is not a nicety: the four checkbox roles this mixer draws are split
// between two layouts and ONLY ONE OF THEM IS ON SCREEN ON ANY GIVEN MACHINE, so the only way to
// check both is to hand the panel fabricated data.
//
// THERE IS A LAYOUT PASS, as in Audio-Gui and for the same reason: the mixer's strips and switches
// come from whatever ALSA elements the selected card exposes, so no rectangle can be resolved at
// compile time. layout() walks the active page top to bottom, gives every widget its rect and
// returns the height the window should be; geometry.h still fixes every constant it uses and
// static_asserts the worst case.
//
// PRESS AND RELEASE ARE PAIRED, following simple-login-gui's panel: one targetAt() serves both
// halves, press only records what was under the pointer, and release acts only if it is still the
// same thing. Acting on press means a control fires under the finger with no way to change your mind
// once it is down.
//
//------------------------------------------------------------------------------------------------
// THE FOUR CHECKBOX ROLES, which are the reason this file looks the way it does.
//
// The GTK mixer drew checkboxes in four distinct roles, and EVERY ONE IS STILL A LABELLED CHECKBOX
// here -- not an icon, not a pill, not collapsed into a neighbour. Audio-Gui's house idiom puts a
// wordless speaker/dot IconButton at the end of a strip; that idiom is deliberately NOT adopted,
// because three of these four carry an ALSA element's own name and that name is the only thing on
// screen telling you what the box does. An icon cannot say IEC958.
//
//   1. PLAYBACK MUTE -- label "Mute", on any playback element that has a playback switch. Under the
//      slider in the GTK build (mxeq.c:704-713); at the right end of the strip row here.
//
//   2. CAPTURE ENABLE -- label "Enable", on a capture element with both a volume and a switch, at
//      the right end of its own strip row. ON EVERY CARD.
//
//   3. ANY SWITCH-ONLY CONTROL -- labelled with the element's own ALSA name: "IEC958", "S/PDIF",
//      and whatever else a codec exposes. A checkbox row of its own, in the card's element order.
//
// THERE WERE FOUR ROLES AND NOW THERE ARE THREE, because the old 2 and 3 were the same control in
// two places: a capture element with a volume and a switch got an inline "Enable" on USB and a
// "Capture" checkbox in a separate zone on the internal card. One card is ever on screen, so the
// difference was invisible to the only person who could have judged it, and the second placement
// cost a whole second layout to express. Both are the inline "Enable" now, and the strip's own
// label already says which capture it is -- a box reading "Capture" next to a slider labelled
// "Capture" said it twice.
//
// The divider and the switch row are gone with it: one grid, every card, in ALSA element order.
// What mixer_sync_switch_row() was protecting -- a zone that takes no height when nothing is in it
// -- is now free, because a control with nothing to show simply contributes no row.
//
// "Input Source" and any other enumerated control stays a labelled dropdown, now a gfx::Combo. It
// is NOT reducible to the checkboxes beside the mic and line sliders: those set each input's
// MONITORING level (pvolume/pswitch -- how loud it plays back through the speakers), while the enum
// is a mux picking which single input the capture ADC actually records. Different hardware, and
// the enum's one-of-N cannot be drawn as N independent boxes without lying about it.
//------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/canvas.h"
#include "gfx/combo.h"
#include "gfx/keys.h"
#include "gfx/listview.h"
#include "gfx/textfield.h"
#include "gfx/widgets.h"

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

class Panel
{
public:
    enum class Page { Mixer, Devices, Recorder, Bluetooth, Steam };
    static const char *pageLabel(Page p);

    //--- what the mixer page is made of ---------------------------------
    // One horizontal strip: a label, a groove, a numeric readout and -- when the element has one --
    // a LABELLED switch at the right end. Roles 1 and 2 above.
    struct Strip {
        std::string label; // the ALSA element's name
        Slider slider;
        Toggle switchBox; // labelled "Mute" (playback) or "Enable" (capture)
        bool hasSwitch = false;
        int element = -1; // index into the app's element list
    };

    // A switch-only control -- IEC958, S/PDIF -- as a checkbox row of its own, labelled with the
    // element's own ALSA name because that name is the only thing on screen saying what it does.
    struct SwitchCell {
        Toggle toggle;
        int element = -1;
    };

    // A labelled dropdown for an enumerated control.
    struct EnumCell {
        std::string label;
        Combo combo;
        int element = -1;
    };

    //--- what the devices page is made of -------------------------------
    struct DeviceEntry {
        std::string label;  // "Internal", "USB", "HDMI", "Bluetooth"
        std::string detail; // which card it resolved to, or why it is unavailable
        Toggle radio;
        bool enabled = true;
    };

    //--- callbacks ------------------------------------------------------
    struct Callbacks {
        std::function<void(Page)> pageChanged;

        // `element` is the index the Strip/SwitchCell/EnumCell carried.
        std::function<void(int element, int percent)> setVolume;
        // `on` is ALREADY IN ALSA'S SENSE, not the checkbox's: the Mute box's inversion is applied
        // here, in the panel, because the panel is what knows it drew a box labelled "Mute". The
        // GTK build had two handlers differing only in that inversion.
        std::function<void(int element, bool on)> setSwitch;
        std::function<void(int element, int index)> setEnum;

        std::function<void(int deviceIndex)> selectDevice;

        std::function<void()> startRecording;
        std::function<void()> stopRecording;

        std::function<void(bool on)> setDiscoverable;
        std::function<void()> startScan;
        std::function<void()> stopScan;
        // The BlueZ object path of the selected row, which the ListView keeps as its row id.
        std::function<void(const std::string &path)> deviceSelected;
        std::function<void(const std::string &path)> pairDevice;
        std::function<void(const std::string &path)> trustDevice;
        std::function<void(const std::string &path)> connectDevice;
        std::function<void(const std::string &path)> removeDevice;
        std::function<void(const std::string &path)> setBtOutput;

        std::function<void()> toggleSteam;
    };
    Callbacks cb;

    // Set by motion() and by anything else that changed what is on screen, so a pointer merely
    // crossing the window does not recompose it thirty times on the way past. Read and cleared by
    // the app's dirty flag.
    std::function<void()> onNeedsRepaint;

    //--- content --------------------------------------------------------
    Page page() const
    {
        return mPage;
    }
    void setPage(Page p);

    // Replaces the whole mixer. Called on every card change, exactly as the GTK build destroyed and
    // rebuilt its widgets. The three vectors stay separate because each is a different widget and
    // the app refreshes them in place by index; what orders them on screen is mMixRows, built here
    // from each cell's `element` so the page follows the CARD's order rather than the widget kind.
    void setMixer(std::vector<Strip> strips, std::vector<SwitchCell> switches,
                  std::vector<EnumCell> enums);

    // Shown instead of the strips when the output has no mixer: HDMI, Bluetooth, or a card with
    // nothing presentable on it.
    void setMixerPlaceholder(const std::string &text);

    // Mutable access to the mixer's widgets, so an EXTERNAL change (somebody moving a control in
    // alsamixer) can refresh the values in place. Rebuilding the page instead would destroy the
    // strip the pointer is on mid-drag, and the poll-descriptor path makes that a real collision:
    // every setVolume we make comes back as an ALSA event a moment later.
    std::vector<Strip> &mixerStrips()
    {
        return mStrips;
    }
    std::vector<SwitchCell> &mixerSwitches()
    {
        return mSwitches;
    }
    std::vector<EnumCell> &mixerEnums()
    {
        return mEnums;
    }

    void setDevices(std::vector<DeviceEntry> entries, int selected);

    // The two recorder dropdowns' items. Set once; the VALUE of each item is the token the app
    // matches on, never the visible label.
    void setRecorderCombos(std::vector<ComboItem> channels, std::vector<ComboItem> rates);

    void setRecorderState(bool recording, const std::string &status);
    // The typed filename, so the app can read it when Record is pressed and seed it at start-up.
    std::string recorderFilename() const
    {
        return mRecFilename.text();
    }
    void setRecorderFilename(const std::string &s);
    int recorderChannels() const; // 1 or 2
    int recorderRate() const;     // 44100 or 48000

    void setBluetoothDevices(std::vector<ListRow> rows);
    void setBluetoothState(bool adapterReady, bool discoverable, bool discovering);
    // Which actions the selected device can take, so a pill that cannot work is drawn disabled
    // rather than failing when pressed. This is the gating the GTK build did with
    // gui_bt_get_device_state before each call.
    // `connected` is BluezDeviceProps::connectedForUse(), and `audioSink` says whether the device
    // can play audio at all -- Set as Output needs both.
    void setBluetoothSelectionState(bool haveSelection, bool paired, bool trusted, bool connected,
                                    bool audioSink);

    void setSteamState(bool active, const std::string &status);

    // The message strip at the foot of the page. Replaces eight gtk_message_dialog_new +
    // gtk_dialog_run sites: gtk_dialog_run spins a nested main loop, there is no nested loop here,
    // and a modal error over a mixer is a dialog you have to dismiss before you can turn the volume
    // down. Cleared by a click on it or by the next action.
    void showMessage(const std::string &text, bool isError);
    void clearMessage();
    bool hasMessage() const
    {
        return !mMessage.empty();
    }

    //--- layout and paint -----------------------------------------------
    // Assigns every widget on the ACTIVE page its rect and returns the window height the content
    // needs, clamped to geometry.h's bounds.
    float layout();
    float height() const
    {
        return mHeight;
    }

    void draw(Canvas &c) const;

    //--- input ----------------------------------------------------------
    // (-1, -1) means the pointer left the window, and clears every hover and drag.
    void motion(float x, float y);
    void press(float x, float y, int button);
    void release(float x, float y, int button);
    bool key(Key k, const char *text, int len, unsigned state);
    // A wheel notch. Only the mixer page and the Bluetooth list consume it.
    bool scroll(float x, float y, int dir);

    // True while a slider is being dragged, so the app can refuse to overwrite the value underneath
    // the pointer when the mixer reports an external change. With poll descriptors now wired up
    // that is a real collision and not a theoretical one: every set_volume we make comes back as an
    // ALSA event a moment later.
    bool dragging() const
    {
        return mDragStrip >= 0;
    }

    // Whether the mixer page is taller than its viewport, and by how much. The mixer is the one page
    // with no bound on its content -- the card decides how many controls there are -- so it is the
    // one page that scrolls.
    bool mixerScrollable() const;

private:
    enum class Target {
        // NOT called None: X11/X.h defines None as 0L and a macro does not care that this is a
        // scoped enumeration. Every project in this family hit that and renamed the same way.
        Nothing,
        Tab,
        Slider,
        StripSwitch,
        SwitchCell,
        EnumCombo,
        DeviceRadio,
        RecordButton,
        StopButton,
        RecFilename,
        RecChannels,
        RecRate,
        BtDiscoverable,
        BtScan,
        BtStop,
        BtList,
        BtAction,
        SteamToggle,
        Message,
    };

    Target targetAt(float x, float y, int &index) const;
    void clearHover();
    void layoutMixer(float y);
    void layoutDevices(float y);
    void layoutRecorder(float y);
    void layoutBluetooth(float y);
    void layoutSteam(float y);
    void drawMixer(Canvas &c) const;
    void drawDevices(Canvas &c) const;
    void drawRecorder(Canvas &c) const;
    void drawBluetooth(Canvas &c) const;
    void drawSteam(Canvas &c) const;
    void repaint();
    // The mixer page's own content height, before clamping. Used for the scroll bound.
    float mixerContentH() const;

    Page mPage = Page::Mixer;
    float mHeight = 0.0f;

    // The tab bar.
    Pill mTabs[5];

    // Mixer.
    std::vector<Strip> mStrips;
    std::vector<SwitchCell> mSwitches;
    std::vector<EnumCell> mEnums;

    // THE ONE THING THAT ORDERS THE MIXER PAGE. Laying the three vectors out one after another put
    // every dropdown below every checkbox below every slider, which is the widget's order and not
    // the card's: on this ALC897 it dragged Input Source down past Rear Mic, which sits after it on
    // the card. Sorting by `element` puts each control back where the codec has it.
    enum class MixRowKind { Strip, Switch, Enum };
    struct MixRow {
        MixRowKind kind;
        size_t index; // into mStrips / mSwitches / mEnums
    };
    std::vector<MixRow> mMixRows;
    void buildMixRows();
    int mixRowElement(const MixRow &r) const;

    // The Bluetooth page's enabled/active states from the stored flags. Called by layoutBluetooth()
    // AND by both Bluetooth setters, because the flags change without any rect changing.
    void applyBluetoothGating();
    float mixRowHeight(const MixRow &r) const;
    std::string mMixerPlaceholder;
    float mMixerScroll = 0.0f;
    // The rect the strips are clipped to, so a scrolled strip is cut at the viewport edge.
    Rect mMixerViewport;

    // Devices.
    std::vector<DeviceEntry> mDevices;
    int mDeviceSelected = 0;

    // Recorder.
    TextField mRecFilename;
    Combo mRecChannels;
    Combo mRecRate;
    Pill mRecRecord;
    Pill mRecStop;
    std::string mRecStatus = "Idle";
    bool mRecording = false;

    // Bluetooth.
    PillToggle mBtDiscoverable;
    Pill mBtScan;
    Pill mBtStopScan;
    ListView mBtList;
    Pill mBtActions[5]; // Pair, Trust, Connect, Remove, Set as Output
    bool mBtAdapterReady = false;
    bool mBtDiscovering = false;
    bool mBtHaveSelection = false;
    bool mBtPaired = false;
    bool mBtTrusted = false;
    bool mBtConnected = false;
    bool mBtAudioSink = false;

    // Steam.
    PillToggle mSteamToggle;
    std::string mSteamStatus;

    // The message strip.
    std::string mMessage;
    bool mMessageIsError = false;
    Rect mMessageRect;

    // Hover and press state.
    int mDragStrip = -1;
    Target mPressTarget = Target::Nothing;
    int mPressIndex = -1;
    Target mHoverTarget = Target::Nothing;
    int mHoverIndex = -1;
};

} // namespace jackbridge

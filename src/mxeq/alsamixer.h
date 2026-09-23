// The sound card's mixer, as a model with no UI in it.
//
// Ported from Audio-Gui's AlsaMixer and GENERALISED. That one probes a fixed seven-entry kWanted[]
// table -- Master, Headphone, Speaker, Mic, Mic Boost, Capture, IEC958 -- and reports the rest
// absent. mxeq cannot do that: it enumerates EVERY simple element the card has and then curates,
// because the whole point of the USB path is to show an interface's own controls, whatever they
// are. So the element list is built rather than looked up, and three rules come across from
// mxeq.c verbatim in behaviour.
//
//------------------------------------------------------------------------------------------------
// THE THREE CURATION RULES, and CLAUDE.md says two of them are load-bearing.
//
//   1. kInternalAllow is matched as case-insensitive SUBSTRINGS, not exact names. HDA codecs label
//      the same function differently between machines: "Master" / "Master Front", "Speaker" /
//      "Front", "Mic" / "Front Mic" / "Internal Mic", "IEC958" / "S/PDIF". Exact-name matching
//      hides real controls on hardware other than the developer's. DO NOT CONVERT THIS TO EXACT
//      MATCHING -- CLAUDE.md forbids it by name.
//
//   2. THE FALLBACK. If curation matches nothing on a card, the filter is abandoned and every
//      control is shown. This project runs on hardware the maintainers cannot test, and an
//      unrecognised codec must never produce an empty mixer panel. DO NOT REMOVE IT -- CLAUDE.md
//      forbids that by name too.
//
//   3. isDspTopologyControl() matches by SHAPE, not by name: an uppercase widget type, a widget
//      number, a dot, an instance number. See the comment on it.
//
// usesSwitchRow() IS GONE, along with the two-zone layout it selected. The panel draws one grid on
// every card now, in ALSA element order, so nothing here decides an arrangement any more -- `curate`
// narrows the element SET and that is all it does. See panel.h.
//
//------------------------------------------------------------------------------------------------
// EXTERNAL CHANGES ARE DELIVERED LIVE, WHICH IS NEW. The GTK mxeq called no snd_mixer_poll_*
// anything: move Master in alsamixer(1) and the panel went on showing the old value until it was
// rebuilt. Audio-Gui's window already waits on the descriptors, so the mechanism is a port even
// though the behaviour is an addition -- main.cpp hands pollDescriptors() to X11Window::addFd and
// the window calls handleEvents() when one fires. This class does no waiting of its own.
//
// THE DESCRIPTORS DO NOT SURVIVE A REOPEN. snd_mixer_close() invalidates them and every
// Element::elem pointer with them, so a caller that reopens must re-register what
// pollDescriptors() then returns. onDescriptorsChanged fires to say so -- and it matters here more
// than in Audio-Gui, because mxeq reopens onto a different card every time the output device
// changes.

#pragma once

#include <alsa/asoundlib.h>

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

class AlsaMixer
{
public:
    // How an element should be presented, DERIVED FROM ITS ALSA CAPABILITIES AND NEVER FROM ITS
    // NAME, so the widget is right on any codec. CLAUDE.md records this as the mixer panel's rule
    // and it is the reason `Input Source` is a dropdown rather than a slider that did nothing.
    enum class Kind {
        Slider, // has a playback or capture volume
        Switch, // switch only, no volume -- IEC958, S/PDIF
        Enum,   // enumerated -- Input Source
    };

    struct Element {
        // The ALSA simple-element name, with " #N" appended when the element's index is not zero.
        //
        // ALSA can expose several elements sharing a name, distinguished only by index: 'Headphone'
        // 0 and 1 for a front and a rear jack, or one 'Capture'/'Input Source' pair per capture
        // stream. They are genuinely separate controls, so they are all kept -- but the extras are
        // labelled, because two identical-looking widgets in a panel is worse than either.
        std::string name;

        Kind kind = Kind::Slider;
        bool isCapture = false;

        // The element has a switch AS WELL AS its volume: a playback element's mute, or a capture
        // element's enable. This is what decides two of the four checkbox roles the panel draws --
        // see panel.h.
        bool hasSwitch = false;

        // Owned by the mixer handle, not by us. INVALIDATED BY reopen().
        snd_mixer_elem_t *elem = nullptr;
    };

    AlsaMixer() = default;
    ~AlsaMixer();

    AlsaMixer(const AlsaMixer &) = delete;
    AlsaMixer &operator=(const AlsaMixer &) = delete;

    // The hardware changed underneath us -- somebody moved a control in alsamixer(1), or a media
    // key did. Fired from handleEvents().
    std::function<void()> onChanged;

    // The poll descriptors have been torn down and rebuilt; re-register them. Fired by open() and
    // reopen().
    std::function<void()> onDescriptorsChanged;

    // Open `cardNumber` as hw:N and collect its controls. `curate` narrows the set to
    // kInternalAllow and is TRUE for the internal card only: USB interfaces are never curated,
    // because their controls are the point of the device.
    //
    // Returns false having warned. A card with no simple elements at all is a failure here, and
    // the panel draws its own explanation.
    bool open(int cardNumber, bool curate);

    // Close the current card and open another, rebuilding the element list and the descriptors.
    // Returns false leaving the mixer closed.
    bool reopen(int cardNumber, bool curate);

    void close();

    bool isOpen() const
    {
        return mHandle != nullptr;
    }
    int cardNumber() const
    {
        return mCard;
    }

    // True when curation was asked for and matched nothing, so the fallback showed everything.
    // Exposed so the panel and the audit can tell that case apart from an ordinary USB card.
    bool curationFellBack() const
    {
        return mCurationFellBack;
    }

    const std::vector<Element> &elements() const
    {
        return mElements;
    }

    // Valid only until the next open/reopen/close.
    const std::vector<int> &pollDescriptors() const
    {
        return mFds;
    }

    // Drain ALSA's event queue and fire onChanged. Call when a poll descriptor is readable.
    void handleEvents();

    //--- values ---------------------------------------------------------
    // Volume is 0..100, a percentage of the control's own range. Clamped.
    int volume(const Element &e) const;
    void setVolume(const Element &e, int percent);

    // The switch as ALSA means it: true is "playing" for a playback element and "enabled" for a
    // capture one. NOT inverted here. The panel's Mute box inverts it at the point of drawing,
    // which is where the inversion belongs -- the GTK build had two handlers, on_mute_toggled and
    // on_switch_toggled, differing only in that inversion, and that is one handler too many.
    bool switchOn(const Element &e) const;
    void setSwitchOn(const Element &e, bool on);

    std::vector<std::string> enumItems(const Element &e) const;
    int enumIndex(const Element &e) const;
    void setEnumIndex(const Element &e, int index);

    //--- the curation rules, exposed so tools/uirender can drive them ---
    //
    // DSP pipeline widgets exposed as mixer controls by SOF firmware topologies.
    //
    // On sof-hda-dsp the topology publishes a gain control for every pipeline in the DSP graph:
    // "PGA1.0 1 Master Playback Volume" (analog playback), "PGA2.0 2 Master Capture Volume",
    // "PGA7/8/9.0 Master" (the three HDMI pipelines), "PGA30.0 30" (deep buffer), plus EQ
    // coefficient blobs like "EQIIR10.0 eqiir_coef_10". PGA is a Programmable Gain Amplifier and
    // the number is a topology widget id, not anything a user recognises. They sit behind the
    // codec's own Master/Speaker/Headphone controls and are normally pinned at 0 dB, so a user who
    // "turns down the volume" with one has changed DSP-side gain and left the real control alone.
    //
    // MATCHED BY SHAPE RATHER THAN BY NAME: an uppercase widget type, a widget number, a dot, an
    // instance number. Listing names one at a time was worse than wrong, it was inconsistent --
    // kInternalAllow admits "master" as a substring, so PGA1/2/7/8/9 appeared (they carry the word
    // Master) while PGA30/31 did not, splitting identical hardware objects across the filter. The
    // shape rule also covers widget types on other SOF machines that neither the author nor this
    // card has: DRC, MIXIN, SRC, TDFB.
    //
    // The '.' is what makes this safe: HDA codec controls never contain one in that position.
    // "IEC958 Playback Switch" gets as far as IEC/958 and stops at the space, so the HDMI digital
    // switches users need are untouched.
    static bool isDspTopologyControl(const std::string &name);

    // Case-insensitive substring match against a NULL-terminated pattern list.
    static bool nameMatchesAny(const std::string &name, const char *const *patterns);

    static const char *const kInternalAllow[];
    static const char *const kAlwaysHide[];

private:
    bool attach(int cardNumber);
    // Fills mElements. Returns how many it collected. `curate` here is whether to apply
    // kInternalAllow on this pass, which the fallback calls a second time with false.
    int collect(bool curate);
    void collectDescriptors();
    bool classify(snd_mixer_elem_t *elem, Kind *kind, bool *isCapture, bool *hasSwitch) const;

    snd_mixer_t *mHandle = nullptr;
    int mCard = -1;
    bool mCurationFellBack = false;
    std::vector<Element> mElements;
    std::vector<int> mFds;
};

} // namespace jackbridge

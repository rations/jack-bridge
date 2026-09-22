// See alsamixer.h.

#include "alsamixer.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace jackbridge
{

//------------------------------------------------------------------------
// Which controls the internal card shows. Matched as case-insensitive SUBSTRINGS -- see the header
// and CLAUDE.md. Carried across from mxeq.c unchanged, entry for entry.
const char *const AlsaMixer::kInternalAllow[] = {
    "master",  "headphone", "speaker",
    "front",   "surround",  "center", "lfe", "side", // multi-channel outputs
    "pcm",     "mic",       "line",
    "capture", "iec958",    "s/pdif",
    "input source",
    nullptr,
};

// Hardware-only controls that just confuse users here, on any card. Auto-Mute drives jack-sense
// speaker muting and Loopback Mixing drives analog loopback; both remain available in alsamixer(1).
const char *const AlsaMixer::kAlwaysHide[] = {
    "auto-mute", "automute", "loopback", "beep", nullptr,
};

namespace
{

// ASCII lower-casing with an explicit unsigned cast and no locale in it.
//
// This replaces g_ascii_strdown, and the name of that function is the point: it is ASCII by
// contract. std::tolower(char) is undefined for a negative value, which every byte above 0x7F is on
// this platform, and it also consults the current locale -- so in a Turkish locale it maps 'I' to a
// dotless i and "IEC958" stops matching "iec958". A mixer's element names come from the kernel and
// are ASCII; this keeps the comparison ASCII too, deliberately.
char asciiLower(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    if (u >= 'A' && u <= 'Z')
        return static_cast<char>(u - 'A' + 'a');
    return c;
}

bool asciiIsUpper(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return u >= 'A' && u <= 'Z';
}

bool asciiIsDigit(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return u >= '0' && u <= '9';
}

} // namespace

//------------------------------------------------------------------------
bool AlsaMixer::isDspTopologyControl(const std::string &name)
{
    size_t p = 0;
    const size_t n = name.size();

    if (p >= n || !asciiIsUpper(name[p]))
        return false;
    while (p < n && asciiIsUpper(name[p])) // PGA, EQIIR, DRC...
        ++p;
    if (p >= n || !asciiIsDigit(name[p])) // widget number
        return false;
    while (p < n && asciiIsDigit(name[p]))
        ++p;
    if (p >= n || name[p] != '.') // the discriminator
        return false;
    ++p;
    return p < n && asciiIsDigit(name[p]); // instance number
}

bool AlsaMixer::nameMatchesAny(const std::string &name, const char *const *patterns)
{
    std::string lower(name.size(), '\0');
    for (size_t i = 0; i < name.size(); ++i)
        lower[i] = asciiLower(name[i]);

    for (int i = 0; patterns[i]; ++i) {
        if (lower.find(patterns[i]) != std::string::npos)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------
AlsaMixer::~AlsaMixer()
{
    close();
}

void AlsaMixer::close()
{
    if (mHandle) {
        snd_mixer_close(mHandle);
        mHandle = nullptr;
    }
    // Every elem pointer belonged to that handle and is now dangling, so the list goes with it.
    // The GTK build had this teardown open-coded in three places and one copy forgot the strdup'd
    // names, leaking them on every card switch; there is one path here.
    mElements.clear();
    mFds.clear();
    mCard = -1;
    mCurationFellBack = false;
}

//------------------------------------------------------------------------
bool AlsaMixer::attach(int cardNumber)
{
    char card[32];
    snprintf(card, sizeof(card), "hw:%d", cardNumber);

    snd_mixer_t *m = nullptr;
    if (snd_mixer_open(&m, 0) < 0) {
        fprintf(stderr, "jack-bridge: cannot open a mixer for card %d\n", cardNumber);
        return false;
    }
    if (snd_mixer_attach(m, card) < 0) {
        fprintf(stderr, "jack-bridge: cannot attach to %s\n", card);
        snd_mixer_close(m);
        return false;
    }
    if (snd_mixer_selem_register(m, nullptr, nullptr) < 0) {
        fprintf(stderr, "jack-bridge: cannot register the simple element class for %s\n", card);
        snd_mixer_close(m);
        return false;
    }
    if (snd_mixer_load(m) < 0) {
        fprintf(stderr, "jack-bridge: cannot load the mixer elements for %s\n", card);
        snd_mixer_close(m);
        return false;
    }

    mHandle = m;
    mCard = cardNumber;
    return true;
}

//------------------------------------------------------------------------
bool AlsaMixer::open(int cardNumber, bool curate)
{
    close();

    // Assigned from what was ASKED FOR, before the fallback can run, and never touched again --
    // see the header note. An unrecognised internal codec keeps the two-zone layout.
    mUsesSwitchRow = curate;

    if (!attach(cardNumber))
        return false;

    int found = collect(curate);

    // THE SAFETY NET. If curation matched nothing, the name list simply does not fit this
    // hardware. Showing an empty mixer would be worse than showing too much.
    if (curate && found == 0) {
        fprintf(stderr,
                "jack-bridge: no curated controls matched on card %d; showing all controls\n",
                cardNumber);
        found = collect(false);
        mCurationFellBack = true;
    }

    if (found == 0) {
        fprintf(stderr, "jack-bridge: no usable mixer controls on card %d\n", cardNumber);
        // NOT a failure: the panel draws its own explanation, and the handle stays open so the
        // poll descriptors still report a card that gains controls later.
    }

    collectDescriptors();
    if (onDescriptorsChanged)
        onDescriptorsChanged();
    return true;
}

bool AlsaMixer::reopen(int cardNumber, bool curate)
{
    if (!open(cardNumber, curate)) {
        close();
        // Still fire it: the caller registered the OLD descriptors with its event loop and they are
        // gone whether or not the new card opened. Leaving a closed mixer's stale descriptors in a
        // select() set is a loop that wakes for ever on a bad fd.
        if (onDescriptorsChanged)
            onDescriptorsChanged();
        return false;
    }
    return true;
}

//------------------------------------------------------------------------
bool AlsaMixer::classify(snd_mixer_elem_t *elem, Kind *kind, bool *isCapture, bool *hasSwitch) const
{
    // FROM CAPABILITIES, NEVER FROM THE NAME. Order matters: an enumerated element may also report
    // a volume on some codecs, and presenting Input Source as a slider is the bug this ordering
    // fixed.
    if (snd_mixer_selem_is_enumerated(elem)) {
        *kind = Kind::Enum;
        *isCapture = snd_mixer_selem_is_enum_capture(elem) != 0;
        *hasSwitch = false;
        return true;
    }

    if (snd_mixer_selem_has_playback_volume(elem) || snd_mixer_selem_has_capture_volume(elem)) {
        *kind = Kind::Slider;
        *isCapture = snd_mixer_selem_has_capture_volume(elem) &&
                     !snd_mixer_selem_has_playback_volume(elem);
        // Whether the element carries a switch BESIDE its volume. This is what tells the panel to
        // draw a Mute box on a playback strip or an Enable box on a capture strip.
        *hasSwitch = *isCapture ? snd_mixer_selem_has_capture_switch(elem) != 0
                                : snd_mixer_selem_has_playback_switch(elem) != 0;
        return true;
    }

    if (snd_mixer_selem_has_playback_switch(elem) || snd_mixer_selem_has_capture_switch(elem)) {
        *kind = Kind::Switch;
        *isCapture = snd_mixer_selem_has_capture_switch(elem) &&
                     !snd_mixer_selem_has_playback_switch(elem);
        *hasSwitch = true;
        return true;
    }

    return false; // nothing presentable
}

//------------------------------------------------------------------------
int AlsaMixer::collect(bool curate)
{
    mElements.clear();
    if (!mHandle)
        return 0;

    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(mHandle); elem;
         elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE)
            continue;

        const char *raw = snd_mixer_selem_get_name(elem);
        if (!raw)
            continue;
        const std::string name(raw);

        if (nameMatchesAny(name, kAlwaysHide))
            continue;
        // Before the allow list, and ON EVERY CARD: these are firmware internals, not a property of
        // how curated this card's panel is.
        if (isDspTopologyControl(name))
            continue;
        if (curate && !nameMatchesAny(name, kInternalAllow))
            continue;

        Element e;
        if (!classify(elem, &e.kind, &e.isCapture, &e.hasSwitch))
            continue;

        // Auto-enable capture switches so a fresh install can record without the user first hunting
        // through alsamixer.
        if (e.isCapture && snd_mixer_selem_has_capture_switch(elem)) {
            int sw = 0;
            snd_mixer_selem_get_capture_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
            if (!sw) {
                snd_mixer_selem_set_capture_switch_all(elem, 1);
                fprintf(stderr, "jack-bridge: auto-enabled capture for '%s' on card %d\n",
                        name.c_str(), mCard);
            }
        }

        const unsigned int index = snd_mixer_selem_get_index(elem);
        e.name = index > 0 ? name + " #" + std::to_string(index) : name;
        e.elem = elem;
        mElements.push_back(std::move(e));
    }

    return static_cast<int>(mElements.size());
}

//------------------------------------------------------------------------
void AlsaMixer::collectDescriptors()
{
    mFds.clear();
    if (!mHandle)
        return;

    const int count = snd_mixer_poll_descriptors_count(mHandle);
    if (count <= 0)
        return;

    std::vector<struct pollfd> pfds(static_cast<size_t>(count));
    const int got = snd_mixer_poll_descriptors(mHandle, pfds.data(), static_cast<unsigned>(count));
    if (got <= 0)
        return;

    for (int i = 0; i < got; ++i) {
        if (pfds[static_cast<size_t>(i)].fd >= 0)
            mFds.push_back(pfds[static_cast<size_t>(i)].fd);
    }
}

void AlsaMixer::handleEvents()
{
    if (!mHandle)
        return;
    // Drains ALSA's queue and runs its per-element callbacks. We register none, so this is here to
    // consume the events -- an unread descriptor stays readable and the select() loop would spin.
    snd_mixer_handle_events(mHandle);
    if (onChanged)
        onChanged();
}

//------------------------------------------------------------------------
int AlsaMixer::volume(const Element &e) const
{
    if (!e.elem || e.kind != Kind::Slider)
        return 0;

    long min = 0, max = 0, value = 0;
    if (e.isCapture) {
        snd_mixer_selem_get_capture_volume_range(e.elem, &min, &max);
        snd_mixer_selem_get_capture_volume(e.elem, SND_MIXER_SCHN_FRONT_LEFT, &value);
    } else {
        snd_mixer_selem_get_playback_volume_range(e.elem, &min, &max);
        snd_mixer_selem_get_playback_volume(e.elem, SND_MIXER_SCHN_FRONT_LEFT, &value);
    }
    if (max <= min)
        return 0;
    const long pct = (value - min) * 100 / (max - min);
    return static_cast<int>(std::clamp<long>(pct, 0, 100));
}

void AlsaMixer::setVolume(const Element &e, int percent)
{
    if (!e.elem || e.kind != Kind::Slider)
        return;
    const int p = std::clamp(percent, 0, 100);

    long min = 0, max = 0;
    if (e.isCapture) {
        snd_mixer_selem_get_capture_volume_range(e.elem, &min, &max);
        if (max <= min)
            return;
        snd_mixer_selem_set_capture_volume_all(e.elem, min + (max - min) * p / 100);
    } else {
        snd_mixer_selem_get_playback_volume_range(e.elem, &min, &max);
        if (max <= min)
            return;
        snd_mixer_selem_set_playback_volume_all(e.elem, min + (max - min) * p / 100);
    }
}

//------------------------------------------------------------------------
bool AlsaMixer::switchOn(const Element &e) const
{
    if (!e.elem)
        return false;
    int sw = 1;
    if (e.isCapture)
        snd_mixer_selem_get_capture_switch(e.elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
    else
        snd_mixer_selem_get_playback_switch(e.elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
    return sw != 0;
}

void AlsaMixer::setSwitchOn(const Element &e, bool on)
{
    if (!e.elem)
        return;
    if (e.isCapture)
        snd_mixer_selem_set_capture_switch_all(e.elem, on ? 1 : 0);
    else
        snd_mixer_selem_set_playback_switch_all(e.elem, on ? 1 : 0);
}

//------------------------------------------------------------------------
std::vector<std::string> AlsaMixer::enumItems(const Element &e) const
{
    std::vector<std::string> items;
    if (!e.elem || e.kind != Kind::Enum)
        return items;

    const int n = snd_mixer_selem_get_enum_items(e.elem);
    for (int i = 0; i < n; ++i) {
        char item[64];
        if (snd_mixer_selem_get_enum_item_name(e.elem, static_cast<unsigned>(i), sizeof(item),
                                               item) == 0) {
            item[sizeof(item) - 1] = '\0';
            items.emplace_back(item);
        }
    }
    return items;
}

int AlsaMixer::enumIndex(const Element &e) const
{
    if (!e.elem || e.kind != Kind::Enum)
        return -1;
    unsigned int active = 0;
    if (snd_mixer_selem_get_enum_item(e.elem, SND_MIXER_SCHN_FRONT_LEFT, &active) != 0)
        return -1;
    return static_cast<int>(active);
}

void AlsaMixer::setEnumIndex(const Element &e, int index)
{
    if (!e.elem || e.kind != Kind::Enum || index < 0)
        return;
    // Applied to every channel the element exposes; unsupported ones just fail, which is what the
    // GTK build did and is why the return value is ignored. An enum is a single hardware selector
    // that ALSA happens to present per-channel.
    for (int c = 0; c <= SND_MIXER_SCHN_LAST; ++c) {
        snd_mixer_selem_set_enum_item(e.elem, static_cast<snd_mixer_selem_channel_id_t>(c),
                                      static_cast<unsigned>(index));
    }
}

} // namespace jackbridge

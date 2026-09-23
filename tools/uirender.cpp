// uirender -- compose the real panels offline and AUDIT EVERY STRING AGAINST THE REAL SLOT IT
// LANDS IN.
//
// geometry.h's static_asserts can hold a clearance between two rectangles, because both are
// compile-time numbers. They cannot hold a LABEL inside its slot: how wide "IEC958 (S/PDIF)"
// renders depends on the face, the size and the rasteriser, none of which exist at compile time.
// So the layout is checked in two halves -- geometry.h asserts the rectangles, and this measures
// the text that goes in them, with the bundled faces, against the rects Panel::layout() actually
// assigned.
//
// A STRING WIDER THAN ITS SLOT IS NOT A CRASH, which is the whole reason this tool exists.
// Canvas::clipToWidth truncates it with an ellipsis and the window still draws, so the failure is
// silent, it is invisible on a machine whose fonts happen to be narrow enough, and the first person
// to see "IEC958 (S/PD..." is a user on a different fontconfig.
//
// IT RENDERS THE MIXER PAGE IN BOTH CURATION MODES, and that is the point of having it at all.
// A curated internal codec and an uncurated USB interface expose very different element sets, and
// ONLY ONE OF THE TWO IS ON SCREEN ON ANY GIVEN MACHINE -- handing the panel fabricated data is the
// only way to see both. panel.h sets out the three roles a checkbox plays ("Mute", "Enable", and a
// switch-only control's own name such as "IEC958"), all of which these scenes carry.
//
// The internal scene also pins the ORDERING, which is the fault the single grid can have and the
// two-zone layout could not: its switch-only control and its dropdown carry element indices that
// fall BETWEEN the strips', so a page that appends each widget kind in turn puts them at the foot
// instead of among the sliders. That is invisible to a compiler and obvious in a picture.
//
// IT AUDITS BOTH BINARIES. mxeq's five pages, and jack-graph's toolbar, status line, graph canvas
// and About card. The one thing it cannot compose is jack-graph's SettingsPanel, and the reason is
// worth stating rather than leaving as a gap: that panel takes a JackServerControl&, and
// JackServerControl links libjack and libasound -- so pulling it in here would drag both into a
// tool whose whole claim is that it needs neither. Constructing one would also run `aplay -l` and
// `ps`, which would make the audit's content whatever this machine happens to have plugged in,
// which is the opposite of what an audit is for. So its FIXED strings -- every label, pill, group
// title, note line and combo item, which is where an overflow can actually happen -- are measured
// against its geometry below, and its device names are left to clipToWidth, as a name off the
// hardware always was.
//
// NO X SERVER, NO SOUND CARD, NO BLUETOOTH ADAPTER. It links src/gfx, src/mxeq/panel.o,
// jack-graph's two cairo-only panels, the pure-data model types they need, and
// src/platform/respath.o -- and nothing else, which is what the Makefile's "the gfx layer does not
// link X11" rule is for. If this tool ever needs libX11, libasound or libjack to build, something
// has reached across that line.
//
// Text metrics are scale-invariant in logical units -- Canvas sets CAIRO_HINT_METRICS_OFF, so a
// string's width at scale s is exactly s times its width at scale 1 -- so the AUDIT RUNS ONCE while
// the PNGs are written at each scale, because rasterisation at 0.75x genuinely is not rasterisation
// at 2x and the pictures are what a human looks at.
//
// Exit status is 0 only if every string fits and every codepoint has a glyph.
//
// Usage: uirender [--out <dir>]

#include "chrome.h"
#include "gfx/fontstack.h"
#include "gfx/palette.h"
#include "graphgeometry.h"
#include "graphpanel.h"
#include "mxeq/geometry.h"
#include "mxeq/panel.h"
#include "platform/respath.h"

#include <cairo/cairo.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace jackbridge;

namespace
{

int gFailures = 0;

//------------------------------------------------------------------------
// The worst case each slot has to hold.
//
// These are not invented. The mixer's names come from the codec, so the widest ones are the widest
// an allowed pattern in AlsaMixer::kInternalAllow can produce once ALSA's index disambiguation has
// appended " #1" to a duplicate -- which is a real pair of controls on the test machine, not a
// hypothetical. "IEC958 (S/PDIF)" is in here because some codecs spell that control that way and it
// is the widest thing that can ever land in a switch-row cell.
const char *const kWidestStripLabels[] = {
    "Front Mic Boost #1", "Internal Mic Boost", "Surround Playback",
    "IEC958 (S/PDIF) #1", "Headphone Mic Jack",
};

const char *const kWidestSwitchLabels[] = {
    "IEC958 (S/PDIF)", "IEC958 (S/PDIF) #1", "Capture #1", "Capture", "IEC958",
};

// The two labels the strip-row checkbox can carry. Pinned by name because the slot they share
// (geo::kMixSwitchW) was sized for them and for nothing else.
const char *const kStripSwitchLabels[] = {"Mute", "Enable"};

// An enum row's label sits in the strips' label column, so the widest control name a codec can put
// on a dropdown is bound by geo::kMixLabelW just as a strip's is.
const char *const kWidestEnumLabels[] = {
    "Input Source", "Input Source #1", "Clock Source", "Digital Source",
};

// Every message the app can put in the strip at the foot of the page, collected from devices.cpp,
// recorder.cpp, steam.cpp and app.cpp. These replaced eight gtk_message_dialog_new sites, and a
// modal dialog could be any size it liked -- so this is the one place where a full sentence has to
// be shown to fit rather than assumed to.
const char *const kMessages[] = {
    "Could not run the routing helper.",
    "Bluetooth ports did not appear.",
    "Enter a filename to record to.",
    "Steam bridge started.",
    "Stopping the Steam bridge...",
    "JACK is not running. Start it from jack-graph's JACK Settings window first.",
    "No Bluetooth device selected. Open the Bluetooth page, connect a device, and try again.",
    // Bluez::hintMessage(), one per reason it names, plus its catch-all with the LONGEST reason
    // string src/error.h can put in front of it. These are the messages a failed Pair or Connect
    // shows, and until they were listed here none of them had ever been measured.
    "Connect failed: br-connection-page-timeout  The device did not answer. Check it is switched "
    "on, in range, and not connected to another device such as a phone.",
    "Connect failed: br-connection-adapter-not-powered  The Bluetooth adapter is switched off.",
    "Connect failed: br-connection-profile-unavailable  No audio profile in common. Check that "
    "bluealsad is running.",
    "Connect failed: br-connection-key-missing  The pairing keys no longer match. Remove the "
    "device and pair it again.",
    "Connect failed: br-connection-refused  The device refused. Remove it and pair it again.",
    "Connect failed: br-connection-concurrent-connection-limit  Check: membership of the "
    "'audio' (and 'bluetooth') group, 90-jack-bridge-bluetooth.rules, adapter on, device in "
    "range.",
    "Could not start arecord. Check that it is installed and that jackd is running.",
    "Recording failed. Check that jackd is running and that the ALSA `jack` PCM is configured.",
    "Could not start pulse-jack-bridge. Check that it is installed: sudo ./contrib/install.sh",
    "Routing helper missing: /usr/local/lib/jack-bridge/jack-route-select"
    " -- run sudo ./contrib/install.sh",
    "Bluetooth output ready: 00:1A:7D:DA:71:13 (bluealsa:playback_1/2)",
    "Saved /home/someone/Music/Alsa Sound Connect-2026-09-23-141530.wav",
};

// The multi-line blocks. Each line is measured on its own, because multilineText() draws them that
// way -- a block that fits "on average" still clips the one long line in it.
const char *const kMixerPlaceholders[] = {
    "No mixer controls were found on this card.",
    "Audio may still work; the sliders are unavailable.",
    "Check: cat /proc/asound/cards, then alsamixer",
    "Mixer controls are not available for HDMI output.",
    "Mixer controls are not available for Bluetooth output.",
    "The USB audio interface is not connected.",
    "Could not open the mixer for card 2.",
};

const char *const kSteamNote[] = {
    "Steam does not work through apulse. This bridge is the workaround:",
    "a minimal PulseAudio server that creates one JACK client,",
    "pulse_bridge:playback_1/2. Other apps do not need it.",
};

// A Bluetooth name is ATTACKER-CONTROLLED and arbitrarily long: BlueZ will hand over whatever the
// remote device advertises, and btmodel.cpp's sanitizeUtf8() guarantees it is valid UTF-8 and
// nothing else. So the list is audited with a name nobody would choose, and the square brackets are
// deliberate -- the GTK build round-tripped these flags through the label text and a device calling
// itself "My [Paired] Speaker" corrupted its own row.
const char *const kWidestBtName = "My [Paired] Bluetooth Speaker (Living Room) 2";

//------------------------------------------------------------------------
// EVERY CHARACTER THE PANEL DRAWS MUST EXIST IN THE FACE THAT DRAWS IT.
//
// This is a separate question from whether the text fits, and checking only the latter is how a
// hole in a label reaches a user. cairo's toy text API -- cairo_show_text on one cairo_ft face --
// has NO FONT FALLBACK: a character the face lacks renders as glyph 0, which in Roboto is a
// zero-width nothing. It therefore measures as fitting, draws as a gap, and looks on the
// developer's machine exactly like it looks in the screenshot they are about to ship. GTK hid this
// completely, because Pango falls back per character across the whole installed font set; that is
// why the GTK build could put any codepoint in a label and this one cannot.
void checkGlyphs(const FontStack &fonts, const char *what, const std::string &text, Font font)
{
    FT_Face face =
        static_cast<FT_Face>(font == Font::Title ? fonts.titleFtFace() : fonts.bodyFtFace());
    if (!face)
        return; // a toy fallback; load() already refused to proceed on that

    // Minimal UTF-8 decode. The strings reaching here are literals in this repository plus the
    // fabricated Bluetooth name, so it only has to handle well-formed input -- a name off the air
    // has been through btmodel's validator by the time the panel sees it.
    const unsigned char *p = reinterpret_cast<const unsigned char *>(text.c_str());
    while (*p) {
        unsigned cp = *p;
        int len = 1;
        if ((*p & 0xE0) == 0xC0) {
            cp = *p & 0x1Fu;
            len = 2;
        } else if ((*p & 0xF0) == 0xE0) {
            cp = *p & 0x0Fu;
            len = 3;
        } else if ((*p & 0xF8) == 0xF0) {
            cp = *p & 0x07u;
            len = 4;
        }
        for (int i = 1; i < len; ++i) {
            if ((p[i] & 0xC0) != 0x80) {
                len = i;
                break;
            }
            cp = (cp << 6) | (p[i] & 0x3Fu);
        }
        if (cp >= 0x20 && FT_Get_Char_Index(face, cp) == 0) {
            fprintf(stderr,
                    "uirender: %s uses U+%04X, which %s has no glyph for -- it will draw as a "
                    "gap: \"%s\"\n",
                    what, cp, face->family_name ? face->family_name : "the bundled face",
                    text.c_str());
            ++gFailures;
        }
        p += len;
    }
}

void checkFits(Canvas &c, const char *what, const std::string &text, float slot, Font font,
               float size)
{
    c.setFont(font);
    c.setFontSize(size);
    const float w = c.stringWidth(text.c_str());
    if (w > slot) {
        fprintf(stderr, "uirender: %s does not fit: \"%s\" needs %.1f, slot is %.1f\n", what,
                text.c_str(), static_cast<double>(w), static_cast<double>(slot));
        ++gFailures;
    }
}

// A checkbox or radio: the label starts after the indicator and its gap, and ends at the rect's
// right edge. Measured off the widget's OWN rect rather than off a constant, so a layout change
// that narrows a cell is caught here instead of at the next release.
void checkToggle(Canvas &c, const char *what, const Toggle &t)
{
    checkFits(c, what, t.label, t.textMaxW(), Font::Body, geo::kBodySize);
}

void checkPill(Canvas &c, const char *what, const Pill &p)
{
    const float need = Pill::widthFor(c, p.label.c_str());
    if (need > p.rect.w) {
        fprintf(stderr,
                "uirender: %s does not fit: \"%s\" needs %.1f including its padding, the pill is "
                "%.1f wide\n",
                what, p.label.c_str(), static_cast<double>(need), static_cast<double>(p.rect.w));
        ++gFailures;
    }
}

void checkPillToggle(Canvas &c, const char *what, const PillToggle &t)
{
    // widgets.cpp draws the label from rect.x up to the knob, less one pill gap.
    const float slot = std::max(0.0f, t.knobRect().x - geo::kPillGap - t.rect.x);
    checkFits(c, what, t.label, slot, Font::Body, geo::kBodySize);
}

// A combo, closed and open. Both slots are audited because they are different widths: the closed
// control loses the arrow gutter and the popup loses the tick gutter, and an item can fit one and
// not the other.
void checkCombo(Canvas &c, const char *what, const Combo &combo)
{
    const Rect r = combo.rect();
    const float closedSlot = std::max(0.0f, r.w - geo::kComboArrowW - geo::kComboPadX);
    const float popupW = std::max(r.w, geo::kPopupTickW + 40.0f);
    const float popupSlot = std::max(0.0f, popupW - geo::kPopupTickW - geo::kComboPadX);
    for (const ComboItem &it : combo.items()) {
        checkFits(c, what, it.label, closedSlot, Font::Body, geo::kBodySize);
        checkFits(c, what, it.label, popupSlot, Font::Body, geo::kPopupTextSize);
    }
}

//------------------------------------------------------------------------
// Scene building. Every scene is fabricated rather than read off this machine: what is plugged in
// here is not what is plugged in anywhere else, and the layouts that matter most are the ones this
// machine cannot show.

Panel::Strip makeStrip(const char *label, int value, const char *switchLabel, bool on, int element)
{
    Panel::Strip s;
    s.label = label;
    s.slider.value = value;
    s.element = element;
    if (switchLabel) {
        s.hasSwitch = true;
        s.switchBox.label = switchLabel;
        s.switchBox.shape = Toggle::Shape::Check;
        s.switchBox.on = on;
    }
    return s;
}

Panel::SwitchCell makeSwitchCell(const char *label, bool on, int element)
{
    Panel::SwitchCell s;
    s.toggle.label = label;
    s.toggle.shape = Toggle::Shape::Check;
    s.toggle.on = on;
    s.element = element;
    return s;
}

Panel::EnumCell makeEnumCell(const char *label, std::vector<ComboItem> items, int element)
{
    Panel::EnumCell e;
    e.label = label;
    e.element = element;
    e.combo.setItems(std::move(items));
    e.combo.setIndex(0);
    e.element = element;
    return e;
}

// THE INTERNAL CARD. One grid, as every card now is. What this scene pins that the USB one cannot
// is the INTERLEAVING: the switch-only control and the dropdown carry element indices that fall
// BETWEEN the strips', so they must be drawn between them. Appending each widget kind in turn --
// which is what the old two-zone layout did -- puts them at the foot of the page instead, and the
// picture is the only thing that catches it.
void sceneInternal(Panel &p)
{
    std::vector<Panel::Strip> strips;
    strips.push_back(makeStrip("Master", 72, "Mute", false, 0));
    strips.push_back(makeStrip("Headphone", 55, "Mute", false, 1));
    strips.push_back(makeStrip("Headphone #1", 55, "Mute", true, 2));
    strips.push_back(makeStrip("PCM", 100, nullptr, false, 3));
    strips.push_back(makeStrip("Front Mic Boost #1", 18, nullptr, false, 4));
    // A capture element with both a volume and a switch: the gain slider AND its own inline
    // "Enable", on every card now. There is no second placement to test any more.
    strips.push_back(makeStrip("Capture", 64, "Enable", true, 5));
    // Element 6 and 8 sit among the strips, not after them: 7 and 9 follow below.
    strips.push_back(makeStrip("Line Boost", 30, nullptr, false, 7));
    strips.push_back(makeStrip("Rear Mic", 0, "Mute", true, 9));
    p.setMixer(std::move(strips), {makeSwitchCell("IEC958 (S/PDIF)", false, 6)},
               {makeEnumCell("Input Source",
                             {{"Rear Mic", "Rear Mic"},
                              {"Front Mic", "Front Mic"},
                              {"Line", "Line"},
                              {"Internal Mic", "Internal Mic"}},
                             8)});
}

// A USB INTERFACE. Same layout as the internal card now -- what still differs between them is
// CURATION, not arrangement, and that is what this scene is for: an uncurated card shows every
// control it has, with names no allow-list ever predicted.
void sceneUsb(Panel &p)
{
    std::vector<Panel::Strip> strips;
    strips.push_back(makeStrip("PCM", 80, "Mute", false, 0));
    strips.push_back(makeStrip("PCM #1", 80, "Mute", false, 1));
    strips.push_back(makeStrip("Line A", 50, nullptr, false, 2));
    strips.push_back(makeStrip("Line B", 50, nullptr, false, 3));
    strips.push_back(makeStrip("Mic", 30, "Enable", true, 4));
    strips.push_back(makeStrip("Mic #1", 30, "Enable", false, 5));
    strips.push_back(makeStrip("Input 1", 45, "Enable", true, 6));
    strips.push_back(makeStrip("Input 2", 45, "Enable", true, 7));
    p.setMixer(std::move(strips), {makeSwitchCell("Auto Gain Control", false, 8)},
               {makeEnumCell("Clock Source", {{"Internal", "Internal"}, {"S/PDIF", "S/PDIF"}}, 9)});
}

// The card that overflows the window. An uncurated USB interface can expose far more controls than
// fit, and the mixer is the one page that scrolls rather than growing the window off the screen --
// so the scroll hint and the clipped viewport need a picture too.
void sceneOverflow(Panel &p)
{
    std::vector<Panel::Strip> strips;
    for (int i = 0; i < 40; ++i) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Input %d", i + 1);
        strips.push_back(makeStrip(buf, (i * 7) % 101, i % 3 == 0 ? "Enable" : nullptr, i % 2 == 0,
                                   i));
    }
    p.setMixer(std::move(strips), {}, {});
}

std::vector<Panel::DeviceEntry> devices()
{
    // The detail line under each radio is the one thing here with no bound of its own: it carries a
    // card id or a MAC or a reason, all of them from outside the program.
    std::vector<Panel::DeviceEntry> v(4);
    const char *labels[] = {"Internal", "USB", "HDMI", "Bluetooth"};
    const char *details[] = {
        "card 0: HDA Intel PCH (hw:0,0)",
        "no USB interface connected",
        "card 1: HDA Intel PCH HDMI/DP,pcm=3 Digital Out",
        "BlueALSA is not installed or not running",
    };
    for (int i = 0; i < 4; ++i) {
        v[static_cast<size_t>(i)].label = labels[i];
        v[static_cast<size_t>(i)].detail = details[i];
        v[static_cast<size_t>(i)].radio.label = labels[i];
        v[static_cast<size_t>(i)].radio.shape = Toggle::Shape::Radio;
        v[static_cast<size_t>(i)].enabled = i != 1;
    }
    return v;
}

std::vector<ListRow> btRows()
{
    std::vector<ListRow> rows;
    // Three chips at once is the widest a row gets, and it is a real state: a paired, trusted,
    // connected speaker is what the Bluetooth page looks like once it has worked.
    rows.push_back({"/org/bluez/hci0/dev_00_1A_7D_DA_71_13",
                    kWidestBtName,
                    {{"Connected", pal::kAccent}, {"Paired", pal::kDimColor},
                     {"Trusted", pal::kGold}},
                    true});
    rows.push_back({"/org/bluez/hci0/dev_11_22_33_44_55_66",
                    "WH-1000XM4",
                    {{"Paired", pal::kDimColor}},
                    true});
    rows.push_back({"/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF", "00:1A:7D:DA:71:13", {}, true});
    for (int i = 0; i < 8; ++i) {
        char id[64];
        char name[64];
        snprintf(id, sizeof(id), "/org/bluez/hci0/dev_00_00_00_00_00_%02X", i);
        snprintf(name, sizeof(name), "Unknown device %d", i + 1);
        rows.push_back({id, name, {}, true});
    }
    return rows;
}

void sceneCommon(Panel &p)
{
    p.setDevices(devices(), 0);
    p.setRecorderCombos({{"Mono", "1"}, {"Stereo", "2"}},
                        {{"44100 Hz", "44100"}, {"48000 Hz", "48000"}});
    p.setRecorderFilename("Alsa Sound Connect-2026-09-23-141530.wav");
    p.setRecorderState(false, "Idle");
    p.setBluetoothDevices(btRows());
    p.setBluetoothState(true, true, true);
    p.setBluetoothSelectionState(true, true, true, true, true);
    p.setSteamState(false, "Steam bridge is not running");
}

//------------------------------------------------------------------------
// The audit. It runs against a LAID-OUT panel, so every slot below is the rect the panel really
// assigned rather than a constant re-derived here -- which is the mistake that lets a layout change
// pass an audit it should have failed.
void auditPanel(Canvas &c, Panel &p, const char *scene)
{
    char what[192];

    for (int i = 0; i < geo::kPageCount; ++i) {
        p.setPage(static_cast<Panel::Page>(i));
        p.layout();

        // The tab bar is laid out on every page, so it is checked once per page -- which costs
        // nothing and means a tab label is never audited only on the page it happens to open.
        snprintf(what, sizeof(what), "[%s] a tab label", scene);
        for (int t = 0; t < geo::kPageCount; ++t) {
            const char *label = Panel::pageLabel(static_cast<Panel::Page>(t));
            checkFits(c, what, label, geo::kTabW - 2.0f * geo::kPillPadX, Font::Body,
                      geo::kPillTextSize);
        }

        switch (static_cast<Panel::Page>(i)) {
            case Panel::Page::Mixer: {
                for (const Panel::Strip &s : p.mixerStrips()) {
                    snprintf(what, sizeof(what), "[%s] a mixer strip label", scene);
                    checkFits(c, what, s.label, geo::kMixLabelW, Font::Body, geo::kMixLabelSize);
                    if (s.hasSwitch) {
                        snprintf(what, sizeof(what), "[%s] a strip switch label (role 1/2)", scene);
                        checkToggle(c, what, s.switchBox);
                    }
                }
                for (const Panel::SwitchCell &s : p.mixerSwitches()) {
                    snprintf(what, sizeof(what), "[%s] a switch-row label (role 3/4)", scene);
                    checkToggle(c, what, s.toggle);
                }
                for (const Panel::EnumCell &e : p.mixerEnums()) {
                    snprintf(what, sizeof(what), "[%s] an enum cell label", scene);
                    checkFits(c, what, e.label, geo::kMixEnumLabelW, Font::Body, geo::kBodySize);
                    snprintf(what, sizeof(what), "[%s] an enum item", scene);
                    checkCombo(c, what, e.combo);
                }
                break;
            }

            case Panel::Page::Devices:
                // Nothing on this page is readable back through the panel, so it is audited from
                // the same constants layoutDevices() uses: the radio spans the content width and
                // the detail line is indented past the indicator.
                for (const Panel::DeviceEntry &d : devices()) {
                    snprintf(what, sizeof(what), "[%s] a device radio label", scene);
                    checkFits(c, what, d.label, geo::kContentW - geo::kDevDetailX, Font::Body,
                              geo::kBodySize);
                    snprintf(what, sizeof(what), "[%s] a device detail line", scene);
                    checkFits(c, what, d.detail, geo::kContentW - geo::kDevDetailX, Font::Body,
                              geo::kDevDetailSize);
                }
                break;

            case Panel::Page::Recorder:
                snprintf(what, sizeof(what), "[%s] a recorder row label", scene);
                for (const char *l : {"File", "Channels", "Rate"})
                    checkFits(c, what, l, geo::kRecLabelW, Font::Body, geo::kBodySize);
                snprintf(what, sizeof(what), "[%s] a recorder button", scene);
                for (const char *l : {"Record", "Stop"}) {
                    Pill pill;
                    pill.label = l;
                    pill.rect = Rect(0.0f, 0.0f, (geo::kContentW - geo::kPillGap) * 0.5f,
                                     geo::kPillH);
                    checkPill(c, what, pill);
                }
                snprintf(what, sizeof(what), "[%s] the recorder status line", scene);
                for (const char *l : {"Idle", "Recording... 59:59",
                                      "Saved /home/someone/Music/Alsa Sound "
                                      "Connect-2026-09-23-141530.wav"})
                    checkFits(c, what, l, geo::kContentW, Font::Body, geo::kRecStatusSize);
                break;

            case Panel::Page::Bluetooth: {
                snprintf(what, sizeof(what), "[%s] the Discoverable toggle", scene);
                {
                    PillToggle t;
                    t.label = "Discoverable";
                    // layoutBluetooth's own arithmetic: two 80-unit pills and two gaps taken off
                    // the content width.
                    t.rect = Rect(geo::kMargin, 0.0f,
                                  geo::kContentW - 160.0f - 2.0f * geo::kPillGap, geo::kPillH);
                    checkPillToggle(c, what, t);
                }
                snprintf(what, sizeof(what), "[%s] a scan button", scene);
                for (const char *l : {"Scan", "Stop"}) {
                    Pill pill;
                    pill.label = l;
                    pill.rect = Rect(0.0f, 0.0f, 80.0f, geo::kPillH);
                    checkPill(c, what, pill);
                }
                snprintf(what, sizeof(what), "[%s] a Bluetooth action", scene);
                for (const char *l : {"Pair", "Trust", "Connect", "Remove", "Set as Output"}) {
                    Pill pill;
                    pill.label = l;
                    pill.rect = Rect(0.0f, 0.0f, geo::kBtActionW, geo::kPillH);
                    checkPill(c, what, pill);
                }
                // A list row: the chips are laid out from the right and the name gets whatever is
                // left, so the name's slot depends on how many badges the row carries. Three is
                // the most, and three is what a working speaker shows.
                float chipsW = 0.0f;
                for (const char *chip : {"Connected", "Paired", "Trusted"})
                    chipsW += chipWidth(c, chip) + geo::kChipGap;
                const float nameSlot = std::max(
                    0.0f, geo::kContentW - 2.0f * geo::kListPadX - chipsW - geo::kChipGap);
                snprintf(what, sizeof(what), "[%s] a Bluetooth device name beside three badges",
                         scene);
                // NOT a hard failure, and this is the one slot where that is right: a device name
                // is whatever the remote end advertises, it has no upper bound, and clipping it is
                // the correct behaviour rather than a layout bug. What is checked is that the slot
                // is wide enough to be USEFUL -- enough for a name someone would recognise.
                if (nameSlot < 140.0f) {
                    fprintf(stderr,
                            "uirender: %s has only %.1f units left for the name, which is too "
                            "little to recognise a device by\n",
                            what, static_cast<double>(nameSlot));
                    ++gFailures;
                }
                snprintf(what, sizeof(what), "[%s] the Bluetooth-unavailable notice", scene);
                for (const char *l : {"Bluetooth is unavailable.",
                                      "Check that bluetoothd is running."})
                    checkFits(c, what, l, geo::kContentW - 2.0f * geo::kListPadX, Font::Body,
                              geo::kMixMessageSize);
                break;
            }

            case Panel::Page::Steam:
                snprintf(what, sizeof(what), "[%s] the Steam toggle", scene);
                {
                    PillToggle t;
                    t.label = "Steam gaming mode";
                    t.rect = Rect(geo::kMargin, 0.0f, geo::kContentW, geo::kPillToggleH);
                    checkPillToggle(c, what, t);
                }
                snprintf(what, sizeof(what), "[%s] the Steam status line", scene);
                for (const char *l : {"Steam bridge is not running", "Steam bridge running",
                                      "Steam bridge is starting..."})
                    checkFits(c, what, l, geo::kContentW, Font::Body, geo::kRecStatusSize);
                snprintf(what, sizeof(what), "[%s] a line of the Steam note", scene);
                for (const char *l : kSteamNote)
                    checkFits(c, what, l, geo::kContentW, Font::Body, geo::kSteamNoteSize);
                break;
        }
    }

    p.setPage(Panel::Page::Mixer);
    p.layout();
}

//------------------------------------------------------------------------
// jack-graph.
//
// THE GRAPH'S OWN CONTENT IS NOT AUDITED FOR WIDTH, and that is deliberate. A client box's header
// is a JACK client name and a port row is a JACK port name; both are whatever the running programs
// call themselves, both are clipped with an ellipsis by graphpanel.cpp on purpose, and the GTK
// build's Pango layout ellipsised them too. What IS audited is everything this repository chose the
// wording of: the toolbar pills, the status line, the About card and the settings window's labels.

// The status line at its longest: update_status_bar() joins five facts with " | " and the numbers
// are the widest they plausibly get -- a 192 kHz server with a six-figure xrun count, and the
// longest string JackServerControl::get_status() returns.
const char *const kWidestStatus =
    "JACK: connected | Buffer: 8192 frames | Rate: 192000 Hz | Xruns: 999999 | "
    "Server: Running | ALSA MIDI: connected";

// The About card's four strings, copied from chrome.cpp. Duplicated here for the same reason
// kMessages above is: the audit has to hold the literal, and a literal held in two places that
// disagree is caught by the picture this tool writes.
const char *const kAboutStrings[] = {"Jack Graph", "Version 0.1.0",
                                     "JACK and ALSA port connection manager", "GPL-3.0+"};

// Everything the JACK Settings window draws that this repository chose the wording of. From
// settingspanel.cpp; see the note in the header about why the panel itself is not composed here.
const char *const kSetGroupTitles[] = {"JACK Server", "Audio", "MIDI"};
const char *const kSetLabels[] = {"Interface:", "Sample Rate:", "Frames/Period:",
                                  "Periods/Buffer:", "MIDI Driver:"};
const char *const kSetStatusLabels[] = {"Status: Running", "Status: Stopped"};
const char *const kSetPills[] = {"Start", "Stop", "Apply Live", "Close"};
const char *const kSetNotes[] = {"Apply Live changes frames/period on the running server.",
                                 "Everything else needs Stop then Start."};
// The combo item lists, with the same tokens SettingsPanel's constructor sets.
const char *const kSetRates[] = {"44100", "48000", "88200", "96000", "192000"};
const char *const kSetFrames[] = {"64", "128", "256", "512", "1024", "2048"};
const char *const kSetPeriods[] = {"2", "3", "4", "5", "6", "7", "8"};
const char *const kSetMidi[] = {"None", "ALSA SEQ"};
// The longest thing mLiveStatus can hold, which is drawn wrapped over kSetStatusLines lines.
const char *const kSetLiveStatus =
    "Frames/period is now 2048. The interface cannot change it while running, so the server was "
    "restarted -- JACK clients will have reconnected.";

// A graph with something in every shape the panel can draw: two clients with audio ports, one with
// MIDI, a stereo pair to exercise pairStereoPorts, and a name longer than a box.
void sceneGraph(GraphPanel &g)
{
    struct PortSpec {
        const char *name;
        PortType type;
        PortDirection dir;
    };
    static const PortSpec kPorts[] = {
        {"system:capture_1", PortType::AUDIO, PortDirection::OUTPUT},
        {"system:capture_2", PortType::AUDIO, PortDirection::OUTPUT},
        {"system:playback_1", PortType::AUDIO, PortDirection::INPUT},
        {"system:playback_2", PortType::AUDIO, PortDirection::INPUT},
        {"system:midi_capture_1", PortType::MIDI, PortDirection::OUTPUT},
        {"system:midi_playback_1", PortType::MIDI, PortDirection::INPUT},
        {"alsa_pcm:playback_1", PortType::AUDIO, PortDirection::OUTPUT},
        {"alsa_pcm:playback_2", PortType::AUDIO, PortDirection::OUTPUT},
        {"pulse_bridge:playback_1", PortType::AUDIO, PortDirection::OUTPUT},
        {"pulse_bridge:playback_2", PortType::AUDIO, PortDirection::OUTPUT},
        {"usb_out:playback_1", PortType::AUDIO, PortDirection::INPUT},
        {"usb_out:playback_2", PortType::AUDIO, PortDirection::INPUT},
        // A stereo pair in the "<base>" / "<base>R" spelling pairStereoPorts exists for, and a
        // client name wider than a box, which is what clipToWidth is there to survive.
        {"REAPER (A Very Long Session Name):out", PortType::AUDIO, PortDirection::OUTPUT},
        {"REAPER (A Very Long Session Name):outR", PortType::AUDIO, PortDirection::OUTPUT},
    };

    std::vector<std::shared_ptr<Node>> made;
    for (const PortSpec &p : kPorts) {
        auto n = std::make_shared<Node>(p.name, p.type, p.dir);
        made.push_back(n);
        g.addNode(n);
    }
    // Two cables, so the connector curve is in the picture as well as the boxes.
    g.addConnection(std::make_shared<Connection>(made[6], made[2], PortType::AUDIO));
    g.addConnection(std::make_shared<Connection>(made[7], made[3], PortType::AUDIO));
    g.layout(false);
}

// Compose the graph window the way jack-graph's App::draw does -- ground, then the canvas, then the
// chrome over it. Four lines, repeated here rather than reached for, because App links libjack.
void drawGraphWindow(Canvas &c, Chrome &chrome, const GraphPanel &graph)
{
    c.setColor(pal::kBgColor);
    c.fillRect(c.bounds());
    chrome.layout(c);
    graph.draw(c);
    chrome.draw(c);
}

void auditGraph(Canvas &c, Chrome &chrome)
{
    static const Tool kTools[] = {Tool::Refresh,  Tool::ZoomOut,  Tool::ZoomIn, Tool::ZoomNormal,
                                  Tool::Fit,      Tool::Settings, Tool::About};

    float total = geo::kGraphMargin;
    for (Tool t : kTools) {
        const Pill *p = chrome.pill(t);
        if (!p) {
            fprintf(stderr, "uirender: the toolbar has no pill for tool %d\n", static_cast<int>(t));
            ++gFailures;
            continue;
        }
        checkPill(c, "a toolbar pill", *p);
        total += p->rect.w + geo::kToolbarGap;
    }
    total += geo::kGraphMargin - geo::kToolbarGap;

    // THE TOOLBAR AT THE NARROWEST THE WINDOW GETS. Every other rectangle in this window grows with
    // it, so this is the only width that can run out -- and it runs out silently, by drawing the
    // last pill off the edge.
    if (total > geo::kGraphMinW) {
        fprintf(stderr,
                "uirender: the toolbar needs %.1f but the smallest window is %.1f wide\n",
                static_cast<double>(total), static_cast<double>(geo::kGraphMinW));
        ++gFailures;
    }

    // The status line, at the narrowest window, in the row Chrome::draw puts it in.
    checkFits(c, "the status line", kWidestStatus, geo::kGraphMinW - 2.0f * geo::kGraphMargin,
              Font::Body, geo::kStatusSize);
}

void auditAbout(Canvas &c)
{
    const float w = geo::kAboutW - 2.0f * geo::kSetMargin;
    checkFits(c, "the About title", kAboutStrings[0], w, Font::Title, geo::kAboutTitleSize);
    for (int i = 1; i < 4; ++i)
        checkFits(c, "an About line", kAboutStrings[i], w, Font::Body, geo::kAboutTextSize);

    Pill close;
    close.label = "Close";
    close.rect = Rect(0.0f, 0.0f, geo::kSetButtonW, geo::kPillH);
    checkPill(c, "the About Close pill", close);
}

// The JACK Settings window's fixed strings, against the slots settingspanel.cpp draws them in.
void auditSettings(Canvas &c)
{
    for (const char *t : kSetGroupTitles)
        checkFits(c, "a settings group title", t, geo::kSetContentW - 2.0f * geo::kGroupTitleX,
                  Font::Body, geo::kGroupTitleSize);

    for (const char *t : kSetLabels)
        checkFits(c, "a settings field label", t, geo::kSetLabelW, Font::Body, geo::kBodySize);

    // The server status shares its row with Start and Stop, so its slot is what is left of the
    // group's inner width once both pills and the gap between them are taken off.
    const float statusSlot = geo::kSetContentW - 2.0f * geo::kSetGroupPad -
                             2.0f * geo::kSetButtonW - geo::kPillGap - geo::kSetLabelGap;
    for (const char *t : kSetStatusLabels)
        checkFits(c, "the server status", t, statusSlot, Font::Body, geo::kBodySize);

    // Start, Stop and Close are kSetButtonW; Apply Live is kSetApplyW.
    for (const char *t : kSetPills) {
        Pill p;
        p.label = t;
        p.rect = Rect(0.0f, 0.0f,
                      strcmp(t, "Apply Live") == 0 ? geo::kSetApplyW : geo::kSetButtonW,
                      geo::kPillH);
        checkPill(c, "a settings pill", p);
    }

    for (const char *t : kSetNotes)
        checkFits(c, "a settings note line", t, geo::kSetContentW - geo::kSetGroupPad, Font::Body,
                  geo::kSetNoteSize);

    // The combos. The Interface combo is the wide one -- it spans the Apply Live column too -- and
    // its items are device names off the machine, so only the four fixed lists are measured.
    const float comboSlot = geo::kSetComboW - geo::kComboArrowW - geo::kComboPadX;
    const float popupSlot = geo::kSetComboW - geo::kPopupTickW - geo::kComboPadX;
    for (const char *t : kSetRates) {
        checkFits(c, "a sample rate", t, comboSlot, Font::Body, geo::kBodySize);
        checkFits(c, "a sample rate", t, popupSlot, Font::Body, geo::kPopupTextSize);
    }
    for (const char *t : kSetFrames) {
        checkFits(c, "a frames/period item", t, comboSlot, Font::Body, geo::kBodySize);
        checkFits(c, "a frames/period item", t, popupSlot, Font::Body, geo::kPopupTextSize);
    }
    for (const char *t : kSetPeriods) {
        checkFits(c, "a periods/buffer item", t, comboSlot, Font::Body, geo::kBodySize);
        checkFits(c, "a periods/buffer item", t, popupSlot, Font::Body, geo::kPopupTextSize);
    }
    for (const char *t : kSetMidi) {
        checkFits(c, "a MIDI driver item", t, comboSlot, Font::Body, geo::kBodySize);
        checkFits(c, "a MIDI driver item", t, popupSlot, Font::Body, geo::kPopupTextSize);
    }

    // The live-status line, wrapped over kSetStatusLines. The same 90% allowance the mxeq message
    // strip uses, and for the same reason: word wrapping cannot use a line's last few units.
    const float statusLine = geo::kSetContentW;
    checkFits(c, "the longest live status", kSetLiveStatus,
              statusLine * (static_cast<float>(geo::kSetStatusLines) - 0.10f), Font::Body,
              geo::kSetStatusSize);
}

//------------------------------------------------------------------------
bool render(Panel &p, float scale, const std::string &outPath)
{
    p.layout();
    const int pw = static_cast<int>(geo::kWinW * scale + 0.5f);
    const int ph = static_cast<int>(p.height() * scale + 0.5f);

    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);
    if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(s);
        return false;
    }
    cairo_t *cr = cairo_create(s);
    // The scale goes exactly where X11Window::paint puts it, and nowhere else.
    cairo_scale(cr, scale, scale);
    {
        FontStack fonts;
        fonts.load(resourceDir());
        Canvas c(cr, &fonts, geo::kWinW, p.height());
        p.draw(c);
    }
    cairo_destroy(cr);
    const bool ok = cairo_surface_write_to_png(s, outPath.c_str()) == CAIRO_STATUS_SUCCESS;
    cairo_surface_destroy(s);
    return ok;
}

// Every page of one scene, at one scale.
void renderPages(Panel &p, const char *scene, float scale, const std::string &out)
{
    static const char *const kPageNames[] = {"mixer", "devices", "recorder", "bluetooth", "steam"};
    for (int i = 0; i < geo::kPageCount; ++i) {
        p.setPage(static_cast<Panel::Page>(i));
        char buf[512];
        snprintf(buf, sizeof(buf), "%s/%s-%s@%.2fx.png", out.c_str(), scene, kPageNames[i],
                 static_cast<double>(scale));
        if (!render(p, scale, buf)) {
            fprintf(stderr, "uirender: could not write %s\n", buf);
            ++gFailures;
        }
    }
}

// The graph window at one size and one scale. Composed into an image surface exactly as
// X11Window::paint would, which is what makes the picture worth looking at.
bool renderGraph(Chrome &chrome, GraphPanel &graph, float w, float h, float scale,
                 const std::string &outPath)
{
    chrome.setWindow(Rect(0.0f, 0.0f, w, h));
    graph.setRect(chrome.canvasRect());
    // AFTER the rect, not before: fitToWindow() measures against the viewport it was last given, so
    // fitting at the old size and then shrinking the window is how the picture ends up clipped.
    graph.fitToWindow();

    const int pw = static_cast<int>(w * scale + 0.5f);
    const int ph = static_cast<int>(h * scale + 0.5f);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);
    if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(s);
        return false;
    }
    cairo_t *cr = cairo_create(s);
    cairo_scale(cr, scale, scale);
    {
        FontStack fonts;
        fonts.load(resourceDir());
        Canvas c(cr, &fonts, w, h);
        drawGraphWindow(c, chrome, graph);
    }
    cairo_destroy(cr);
    const bool ok = cairo_surface_write_to_png(s, outPath.c_str()) == CAIRO_STATUS_SUCCESS;
    cairo_surface_destroy(s);
    return ok;
}

bool renderAbout(AboutCard &about, float scale, const std::string &outPath)
{
    const float h = about.layout();
    const int pw = static_cast<int>(geo::kAboutW * scale + 0.5f);
    const int ph = static_cast<int>(h * scale + 0.5f);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);
    if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(s);
        return false;
    }
    cairo_t *cr = cairo_create(s);
    cairo_scale(cr, scale, scale);
    {
        FontStack fonts;
        fonts.load(resourceDir());
        Canvas c(cr, &fonts, geo::kAboutW, h);
        about.draw(c);
    }
    cairo_destroy(cr);
    const bool ok = cairo_surface_write_to_png(s, outPath.c_str()) == CAIRO_STATUS_SUCCESS;
    cairo_surface_destroy(s);
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    std::string out = ".";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            out = argv[++i];
    }

    // Create the output directory rather than failing on every write: this is a developer tool and
    // "mkdir first" is not a useful thing to have to remember.
    mkdir(out.c_str(), 0755);

    FontStack fonts;
    if (!fonts.load(resourceDir())) {
        // Measuring text against a substituted system face proves nothing about what a user with
        // the bundled fonts will see, so this is fatal here even though the window survives it.
        fprintf(stderr, "uirender: the bundled fonts are missing from %s; the audit would be "
                        "meaningless\n",
                resourceDir().c_str());
        return 1;
    }

    // --- glyph coverage, and the strings with no widget of their own ---
    {
        cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 8, 8);
        cairo_t *cr = cairo_create(s);
        Canvas c(cr, &fonts, geo::kWinW, geo::kWinMaxH);

        // Coverage first: a string with a hole in it is wrong whatever its width.
        checkGlyphs(fonts, "the window title", "ALSA SOUND CONNECT", Font::Title);
        for (int i = 0; i < geo::kPageCount; ++i)
            checkGlyphs(fonts, "a tab label", Panel::pageLabel(static_cast<Panel::Page>(i)),
                        Font::Body);
        for (const char *t : kWidestStripLabels)
            checkGlyphs(fonts, "a mixer strip label", t, Font::Body);
        for (const char *t : kWidestSwitchLabels)
            checkGlyphs(fonts, "a switch label", t, Font::Body);
        for (const char *t : kStripSwitchLabels)
            checkGlyphs(fonts, "a strip switch label", t, Font::Body);
        for (const char *t : kWidestEnumLabels)
            checkGlyphs(fonts, "an enum row label", t, Font::Body);
        for (const char *t : kMessages)
            checkGlyphs(fonts, "a message", t, Font::Body);
        for (const char *t : kMixerPlaceholders)
            checkGlyphs(fonts, "a mixer placeholder line", t, Font::Body);
        for (const char *t : kSteamNote)
            checkGlyphs(fonts, "a line of the Steam note", t, Font::Body);
        checkGlyphs(fonts, "a Bluetooth device name", kWidestBtName, Font::Body);
        for (const char *t : {"Pair", "Trust", "Connect", "Remove", "Set as Output", "Scan",
                              "Stop", "Record", "Discoverable", "Steam gaming mode", "File",
                              "Channels", "Rate", "Mono", "Stereo", "44100 Hz", "48000 Hz",
                              "Connected", "Paired", "Trusted", "Input Source", "Idle"})
            checkGlyphs(fonts, "a panel string", t, Font::Body);
        // jack-graph's own strings, in both windows.
        for (const char *t : {"Refresh", "Zoom -", "Zoom +", "Zoom 1:1", "Fit", "JACK Settings",
                              "About"})
            checkGlyphs(fonts, "a toolbar pill", t, Font::Body);
        checkGlyphs(fonts, "the status line", kWidestStatus, Font::Body);
        checkGlyphs(fonts, "the About title", kAboutStrings[0], Font::Title);
        for (int i = 1; i < 4; ++i)
            checkGlyphs(fonts, "an About line", kAboutStrings[i], Font::Body);
        for (const char *t : kSetGroupTitles)
            checkGlyphs(fonts, "a settings group title", t, Font::Body);
        for (const char *t : kSetLabels)
            checkGlyphs(fonts, "a settings field label", t, Font::Body);
        for (const char *t : kSetStatusLabels)
            checkGlyphs(fonts, "the server status", t, Font::Body);
        for (const char *t : kSetPills)
            checkGlyphs(fonts, "a settings pill", t, Font::Body);
        for (const char *t : kSetNotes)
            checkGlyphs(fonts, "a settings note line", t, Font::Body);
        for (const char *t : kSetMidi)
            checkGlyphs(fonts, "a MIDI driver item", t, Font::Body);
        checkGlyphs(fonts, "the longest live status", kSetLiveStatus, Font::Body);

        // clipToWidth appends U+2026 whenever anything is truncated, so the ellipsis itself has to
        // exist or an elided string ends in a gap -- which is the failure this whole tool is about,
        // appearing in the mechanism meant to soften it.
        checkGlyphs(fonts, "the truncation ellipsis", "\xE2\x80\xA6", Font::Body);

        // The title, in the display face at the display size.
        checkFits(c, "the window title", "ALSA SOUND CONNECT", geo::kContentW, Font::Title,
                  geo::kTitleSize);

        // The widest names a codec can hand the mixer. Checked against the slots directly as well
        // as through the scenes below, because no scene can carry every one of them at once.
        for (const char *t : kWidestStripLabels)
            checkFits(c, "the widest mixer strip label", t, geo::kMixLabelW, Font::Body,
                      geo::kMixLabelSize);
        // A switch-only control's row holds an indicator, its gap and the element's own name,
        // across the label and control columns together.
        const float switchRowSlot = geo::kMixLabelW + geo::kMixLabelGap + geo::kMixGrooveW -
                                    geo::kIndicatorSize - geo::kIndicatorGap;
        for (const char *t : kWidestSwitchLabels)
            checkFits(c, "the widest switch-only row label", t, switchRowSlot, Font::Body,
                      geo::kBodySize);
        // And the strip-row checkbox, whose slot was sized for exactly these two words.
        const float stripSwitchSlot = geo::kMixSwitchW - geo::kIndicatorSize - geo::kIndicatorGap;
        for (const char *t : kStripSwitchLabels)
            checkFits(c, "a strip switch label (Mute/Enable)", t, stripSwitchSlot, Font::Body,
                      geo::kBodySize);
        // An enum row's label shares the strips' label column, so it is bound by that.
        for (const char *t : kWidestEnumLabels)
            checkFits(c, "the widest enum row label", t, geo::kMixLabelW, Font::Body,
                      geo::kMixLabelSize);

        // The message strip. panel.h explains why this replaced eight modal dialogs; a modal dialog
        // could be any size it liked, and this one line cannot, so every message it can carry is
        // measured against it.
        // Measured against the WHOLE strip -- kStripLines lines of it -- because panel.cpp wraps on
        // word boundaries. The slack this leaves is not generous: word wrapping cannot use a line's
        // last few units, so a message measuring just under two full lines can still spill. Allowing
        // 90% of the second line is the margin that covers that, and it is deliberately not 100%.
        const float messageLine = geo::kContentW - 2.0f * geo::kFieldPadX;
        const float messageSlot = messageLine * (static_cast<float>(geo::kStripLines) - 0.10f);
        for (const char *t : kMessages)
            checkFits(c, "a message", t, messageSlot, Font::Body, geo::kStripTextSize);

        for (const char *t : kMixerPlaceholders)
            checkFits(c, "a mixer placeholder line", t, geo::kContentW, Font::Body,
                      geo::kMixMessageSize);

        // The scroll hint on an overflowing mixer page, at its widest.
        checkFits(c, "the mixer scroll hint", "scroll for more (999 controls)", geo::kContentW,
                  Font::Body, geo::kMixValueSize);

        // The JACK Settings window, measured rather than composed -- see the header.
        auditSettings(c);

        cairo_destroy(cr);
        cairo_surface_destroy(s);
    }

    // --- the scenes: audited against their real rects, then drawn ---
    const float kScales[] = {geo::kScaleMin, 1.0f, geo::kScaleMax};

    {
        cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 8, 8);
        cairo_t *cr = cairo_create(s);
        Canvas c(cr, &fonts, geo::kWinW, geo::kWinMaxH);

        // The internal card, both mixer layouts, and the overflowing card.
        {
            Panel p;
            sceneCommon(p);
            sceneInternal(p);
            auditPanel(c, p, "internal");
            for (float scale : kScales)
                renderPages(p, "internal", scale, out);
        }
        {
            Panel p;
            sceneCommon(p);
            sceneUsb(p);
            auditPanel(c, p, "usb");
            for (float scale : kScales)
                renderPages(p, "usb", scale, out);
        }
        {
            Panel p;
            sceneCommon(p);
            sceneOverflow(p);
            auditPanel(c, p, "overflow");
            renderPages(p, "overflow", 1.0f, out);
        }

        cairo_destroy(cr);
        cairo_surface_destroy(s);
    }

    // The mixer placeholder: HDMI has no mixer, and neither does a codec with nothing presentable
    // on it. The page has to explain itself rather than look broken.
    {
        Panel p;
        sceneCommon(p);
        p.setMixer({}, {}, {});
        p.setMixerPlaceholder("Mixer controls are not available for HDMI output.\n"
                              "Audio is routed through hdmi_out:playback_1/2.");
        p.setPage(Panel::Page::Mixer);
        render(p, 1.0f, out + "/placeholder-hdmi@1.00x.png");

        p.setMixerPlaceholder("");
        render(p, 1.0f, out + "/placeholder-empty@1.00x.png");
    }

    // A message on the longest page there is, so the strip is seen where it is tightest: it is laid
    // out below the page and the window is clamped to geo::kWinMaxH.
    {
        Panel p;
        sceneCommon(p);
        sceneInternal(p);
        p.showMessage("Recording failed. Check that jackd is running and that the ALSA `jack` PCM "
                      "is configured.",
                      true);
        p.setPage(Panel::Page::Recorder);
        render(p, 1.0f, out + "/message-error@1.00x.png");
        p.showMessage("Bluetooth output ready: 00:1A:7D:DA:71:13 (bluealsa:playback_1/2)", false);
        p.setPage(Panel::Page::Bluetooth);
        render(p, 1.0f, out + "/message-ok@1.00x.png");
        // THE MESSAGE THAT MADE THE STRIP TWO LINES TALL. geometry.h records why; this is the
        // picture, so the wrap point is something a reviewer can look at rather than reason about.
        p.showMessage("Routing helper missing: /usr/local/lib/jack-bridge/jack-route-select"
                      " -- run sudo ./contrib/install.sh",
                      true);
        p.setPage(Panel::Page::Devices);
        render(p, 1.0f, out + "/message-wrapped@1.00x.png");
    }

    // bluetoothd stopped: the page has to stay up and inert, with the notice over the list. This is
    // the second half of the plan's on-machine check 9, and it is the state the placeholder BlueZ
    // client puts the panel in today.
    {
        Panel p;
        sceneCommon(p);
        sceneInternal(p);
        p.setBluetoothDevices({});
        p.setBluetoothState(false, false, false);
        p.setBluetoothSelectionState(false, false, false, false, false);
        p.setPage(Panel::Page::Bluetooth);
        render(p, 1.0f, out + "/bluetooth-unavailable@1.00x.png");
    }

    // An open combo, because the popup draws over what is below it and that overlap is not visible
    // in any of the pictures above.
    {
        Panel p;
        sceneCommon(p);
        sceneInternal(p);
        p.setPage(Panel::Page::Recorder);
        p.layout();
        p.press(0.0f, 0.0f, 1); // clears any stale press state before the synthetic one below
        p.release(0.0f, 0.0f, 1);
        // Click the Rate combo where it was laid out, which opens it.
        const float fieldX = geo::kMargin + geo::kRecLabelW + geo::kMixLabelGap + 20.0f;
        const float rateY = geo::kPageY + geo::kFieldH + geo::kRecRowGap + geo::kComboH +
                            geo::kRecRowGap + geo::kComboH * 0.5f;
        p.press(fieldX, rateY, 1);
        p.release(fieldX, rateY, 1);
        render(p, 1.0f, out + "/combo-open@1.00x.png");
    }

    //--------------------------------------------------------------------
    // jack-graph: the toolbar and status line audited against the rects Chrome::layout() assigned,
    // then the whole window drawn at three scales and at the smallest size it can be.
    {
        Chrome chrome;
        GraphPanel graph;
        sceneGraph(graph);

        cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 8, 8);
        cairo_t *cr = cairo_create(s);
        {
            Canvas c(cr, &fonts, geo::kGraphW, geo::kGraphH);
            chrome.setWindow(Rect(0.0f, 0.0f, geo::kGraphW, geo::kGraphH));
            chrome.layout(c);
            chrome.setStatus(kWidestStatus);
            auditGraph(c, chrome);
            auditAbout(c);
        }
        cairo_destroy(cr);
        cairo_surface_destroy(s);

        for (float scale : kScales) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s/graph@%.2fx.png", out.c_str(),
                     static_cast<double>(scale));
            if (!renderGraph(chrome, graph, geo::kGraphW, geo::kGraphH, scale, buf)) {
                fprintf(stderr, "uirender: could not write %s\n", buf);
                ++gFailures;
            }
        }
        // THE SMALLEST WINDOW, which is the one the toolbar can run out of room in.
        if (!renderGraph(chrome, graph, geo::kGraphMinW, geo::kGraphMinH, 1.0f,
                         out + "/graph-min@1.00x.png")) {
            fprintf(stderr, "uirender: could not write the smallest graph window\n");
            ++gFailures;
        }
        // And an empty one: JACK stopped, nothing to draw but the chrome saying so.
        {
            Chrome bare;
            GraphPanel none;
            bare.setStatus("JACK: not connected | Server: Stopped");
            none.layout(false);
            if (!renderGraph(bare, none, geo::kGraphW, geo::kGraphH, 1.0f,
                             out + "/graph-empty@1.00x.png")) {
                fprintf(stderr, "uirender: could not write the empty graph window\n");
                ++gFailures;
            }
        }
    }

    // The About card, which is its own dialog window.
    {
        AboutCard about;
        for (float scale : kScales) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s/about@%.2fx.png", out.c_str(),
                     static_cast<double>(scale));
            if (!renderAbout(about, scale, buf)) {
                fprintf(stderr, "uirender: could not write %s\n", buf);
                ++gFailures;
            }
        }
    }

    if (gFailures > 0) {
        fprintf(stderr, "uirender: %d layout problem(s)\n", gFailures);
        return 1;
    }
    printf("uirender: layout audit passed; images written to %s\n", out.c_str());
    return 0;
}

// Where everything in the mxeq window goes.
//
// EVERY CONSTANT IS IN LOGICAL UNITS. One cairo_scale is applied when the frame is composed
// (platform/x11window.cpp) and nothing here has a scale factor baked into it. A label's slot is the
// same number of units at 0.75x and at 2x, which is what makes one layout audit stand for all three.
//
// This opens the same `geo` namespace as gfx/metrics.h and adds to it. The split is: WHAT A CONTROL
// IS lives there and is shared with jack-graph, WHERE IT GOES lives here and is mxeq's alone.
//
// THE HEIGHTS ARE COMPUTED, NOT GUESSED. What this replaces was six hand-tuned constants added up
// by on_any_expander_toggled():
//
//     #define EXPANDER_HEADER_HEIGHT 5      #define MIXER_CONTENT_HEIGHT 320
//     #define REC_CONTENT_HEIGHT 100        #define BT_CONTENT_HEIGHT 150
//     #define DEV_CONTENT_HEIGHT 50         #define STEAM_CONTENT_HEIGHT 65
//     #define WINDOW_BASE_HEIGHT 40
//
// Those were estimates of what GTK would allocate, and the mixer's 320 was an estimate of a panel
// whose real height depends on how many controls the sound card has -- which is a number no
// constant can hold. Here each page's height is a function of its own content and the window
// follows it, with static_asserts below standing in for the eye that used to check.
//
// WHAT THE static_asserts CANNOT COVER is a string that is too wide for its slot, because that
// depends on the font. tools/uirender measures the real strings against these slots with the real
// bundled faces, and that is the other half of this file.

#pragma once

#include "gfx/metrics.h"

namespace jackbridge
{
namespace geo
{

//--- the window -------------------------------------------------------------
// Wider than Audio-Gui's 440 and narrower than the GTK mxeq's 600. The width is set by the mixer
// strip: a label slot that holds "Front Mic Boost" and "Input Source #1" (both real on the test
// machine), a groove long enough that 1% of travel is more than one pixel, and a labelled switch
// at the end.
constexpr float kWinW = 560.0f;
constexpr float kMargin = 14.0f;
constexpr float kContentW = kWinW - 2.0f * kMargin;

// The window never gets shorter than the chrome plus one row of something, and never taller than
// this. A curated internal card has a dozen or so controls; an uncurated USB interface can have
// many more, and past kMaxVisibleH the page scrolls rather than the window growing off the screen.
// 760 leaves room for a titlebar and a panel on a 900-tall display, which is the smallest laptop
// screen this is likely to meet.
constexpr float kWinMinH = 240.0f;
constexpr float kWinMaxH = 760.0f;

//--- the title and tab bar --------------------------------------------------
constexpr float kTitleSize = 15.0f;
constexpr float kTitleY = 10.0f; // top of the title's band
constexpr float kTitleH = 22.0f;

constexpr int kPageCount = 5; // Mixer, Devices, Recorder, Bluetooth, Steam
constexpr float kTabBarY = kTitleY + kTitleH + 6.0f;
constexpr float kTabH = kPillH;
constexpr float kTabGap = kPillGap;
// Equal thirds-of-a-row: every tab the same width, because a tab bar whose tabs change width when
// the labels change is a tab bar whose targets move.
constexpr float kTabW =
    (kContentW - static_cast<float>(kPageCount - 1) * kTabGap) / static_cast<float>(kPageCount);

// Where a page's own content starts. The rule drawn under the tab bar is at kPageRuleY.
constexpr float kPageRuleY = kTabBarY + kTabH + 8.0f;
constexpr float kPageY = kPageRuleY + 10.0f;

// Below the page: the message strip, which replaces eight gtk_message_dialog_new + gtk_dialog_run
// sites. Contributes nothing when there is no message.
//
// IT IS TWO LINES TALL, AND THAT IS NOT A STYLE CHOICE. A modal dialog could be whatever size its
// text needed; this strip cannot, and tools/uirender found the consequence: the longest message the
// program can produce is
//
//     "Routing helper missing: /usr/local/lib/jack-bridge/jack-route-select
//      -- run sudo ./contrib/install.sh"
//
// which needs 524 units against a single line's 512. That message exists to tell the user a path and
// a command, so truncating it to "...jack-route-se..." would delete the only two pieces of
// information in it. The path is also not bounded by anything here -- it is a compiled-in prefix --
// so no single-line slot can be made wide enough on principle. The strip therefore wraps on word
// boundaries into kStripLines lines, and the height below is what that costs.
constexpr int kStripLines = 2;
constexpr float kStripLineH = 15.0f;
constexpr float kStripPadY = 6.0f;
constexpr float kStripH = static_cast<float>(kStripLines) * kStripLineH + 2.0f * kStripPadY;
constexpr float kStripGapAbove = 8.0f;
constexpr float kStripTextSize = 12.0f;
static_assert(kStripLineH > nominalAscent(kStripTextSize) + nominalDescent(kStripTextSize),
              "a message strip line is shorter than the text in it");

constexpr float kPageBottomPad = 12.0f;

// The height of the whole window for a page `pageH` units tall, with or without a message.
constexpr float windowH(float pageH, bool hasMessage)
{
    return kPageY + pageH + (hasMessage ? kStripGapAbove + kStripH : 0.0f) + kPageBottomPad;
}

//--- mixer page -------------------------------------------------------------
// One horizontal strip per control that has a volume. The GTK build drew vertical sliders in a
// wrapping 8-column grid; horizontal strips read better for a long ALSA name, and they make the
// page's height a linear function of the control count, which is what the tab layout needs.
constexpr float kMixRowH = 30.0f;
constexpr float kMixRowGap = 2.0f;
constexpr float kMixLabelW = 130.0f;
constexpr float kMixLabelGap = 8.0f;
constexpr float kMixValueW = 34.0f; // the numeric readout after the groove
constexpr float kMixValueGap = 6.0f;
constexpr float kMixLabelSize = kBodySize;
constexpr float kMixValueSize = 11.0f;

// The labelled switch at the right end of a strip: "Mute" on a playback control, "Enable" on a USB
// capture control. Wide enough for the indicator, its gap and the longer of the two words -- and
// uirender pins both against it, because this is the slot the four-checkbox-role decision hangs on.
constexpr float kMixSwitchW = 74.0f;
constexpr float kMixSwitchGap = 10.0f;

constexpr float kMixGrooveW =
    kContentW - kMixLabelW - kMixLabelGap - kMixValueW - kMixValueGap - kMixSwitchW - kMixSwitchGap;
static_assert(kMixGrooveW > 120.0f, "the mixer groove is too short to set a level with");
static_assert(kMixSwitchW > kIndicatorSize + kIndicatorGap + 30.0f,
              "the strip's switch slot cannot hold an indicator and a word");
static_assert(kMixRowH >= 2.0f * kThumbR + 4.0f, "a mixer row cannot hold its slider thumb");
static_assert(kMixRowH > kIndicatorSize + 2.0f, "a mixer row cannot hold its checkbox indicator");

// The divider and the switch row beneath the strips. INTERNAL CARD ONLY (AlsaMixer::usesSwitchRow).
// Both contribute ZERO HEIGHT when nothing landed in them, which is mixer_sync_switch_row()'s rule
// carried across from a pair of gtk_widget_set_visible calls into the layout itself.
constexpr float kMixDividerGap = 10.0f;
constexpr float kMixSwitchRowH = 24.0f;
constexpr float kMixSwitchRowGap = 6.0f;
// A switch-row cell holds a checkbox with an ALSA element's own name in it -- "IEC958", "Capture",
// "IEC958 (S/PDIF)" on a codec that spells it that way -- or a labelled dropdown. Two per row.
constexpr float kMixSwitchCellW = (kContentW - kMixSwitchRowGap) / 2.0f;

// An enum row is taller: it is a label and a combo, not a checkbox.
constexpr float kMixEnumRowH = kComboH + 4.0f;
constexpr float kMixEnumLabelW = 110.0f;
constexpr float kMixEnumGap = 8.0f;
static_assert(kMixEnumLabelW + kMixEnumGap + 120.0f < kMixSwitchCellW,
              "an enum cell cannot hold its label and a usable dropdown");

// The "scroll for more" line under an overflowing mixer page. IT GETS A BAND OF ITS OWN rather
// than being drawn over the last visible strip: over a strip it reads as a rendering fault, and the
// strip it lands on is one whose label it hides.
constexpr float kMixHintH = 14.0f;
constexpr float kMixHintSize = kMixValueSize;
static_assert(kMixHintH > nominalAscent(kMixHintSize) + nominalDescent(kMixHintSize),
              "the mixer scroll hint's band is shorter than the text in it");

// When a card has no controls at all -- HDMI and Bluetooth have no mixer, and an exotic codec may
// have nothing presentable -- the page draws an explanation instead.
constexpr float kMixMessageH = 96.0f;
constexpr float kMixMessageSize = 12.0f;
constexpr float kMixMessageLineH = 17.0f;

//--- devices page -----------------------------------------------------------
// Four radios: Internal, USB, HDMI, Bluetooth. One per row, because each carries a detail line
// underneath saying which card it resolved to or why it is unavailable -- which the GTK build had
// nowhere to put.
constexpr int kDeviceCount = 4;
constexpr float kDevRowH = 26.0f;
constexpr float kDevDetailH = 15.0f;
constexpr float kDevDetailSize = 11.0f;
constexpr float kDevRowGap = 6.0f;
constexpr float kDevDetailX = kIndicatorSize + kIndicatorGap;

constexpr float devicesPageH()
{
    return static_cast<float>(kDeviceCount) * (kDevRowH + kDevDetailH) +
           static_cast<float>(kDeviceCount - 1) * kDevRowGap;
}

//--- recorder page ----------------------------------------------------------
constexpr float kRecLabelW = 84.0f;
constexpr float kRecRowGap = 10.0f;
constexpr float kRecFieldW = kContentW - kRecLabelW - kMixLabelGap;
constexpr float kRecComboW = 150.0f;
constexpr float kRecStatusH = 20.0f;
constexpr float kRecStatusSize = 12.0f;

constexpr float recorderPageH()
{
    // filename field, channels combo, rate combo, the button row, the status line.
    return kFieldH + kRecRowGap + kComboH + kRecRowGap + kComboH + kRecRowGap + kPillH +
           kRecRowGap + kRecStatusH;
}
static_assert(kRecFieldW > 200.0f, "the recorder filename field is too narrow to read a name in");

//--- bluetooth page ---------------------------------------------------------
// The device list, then two rows of pills. The list is sized in WHOLE ROWS so a half row is never
// the bottom of the viewport -- a half row reads as a rendering fault rather than as more list.
constexpr int kBtVisibleRows = 6;
constexpr float kBtListH = static_cast<float>(kBtVisibleRows) * kListRowH;
constexpr float kBtRowGap = 8.0f;
constexpr int kBtActionCount = 5; // Pair, Trust, Connect, Remove, Set as Output

// A TIGHTER GAP THAN kPillGap, and tools/uirender is why. Five equal pills across the content width
// at the usual 6-unit gap give 101.6 units each, and "Set as Output" needs 101.8 with the pill's own
// padding -- so the widest of the five labels overflowed by two tenths of a unit, which clipToWidth
// would have silently turned into "Set as Outpu...". The label is the GTK build's own
// (mxeq.c:1264) and renaming a button during a port is not a fix, so the gap gives up two units
// instead.
constexpr float kBtActionGap = 4.0f;
constexpr float kBtActionW =
    (kContentW - static_cast<float>(kBtActionCount - 1) * kBtActionGap) /
    static_cast<float>(kBtActionCount);

constexpr float bluetoothPageH()
{
    // Discoverable toggle + Scan/Stop row, the list, the action row.
    return kPillH + kBtRowGap + kBtListH + kBtRowGap + kPillH;
}
// 102 rather than a round 70: the bound that matters is the widest label, "Set as Output", which
// measures 101.8 in the bundled face. This cannot assert the text width -- that needs a rasteriser --
// so it asserts the number uirender measured against, and uirender re-measures it on every run.
static_assert(kBtActionW > 102.0f, "a Bluetooth action pill is too narrow for \"Set as Output\"");

//--- steam page -------------------------------------------------------------
constexpr float kSteamRowGap = 10.0f;
constexpr float kSteamNoteSize = 12.0f;
constexpr float kSteamNoteLineH = 17.0f;
constexpr int kSteamNoteLines = 3;

constexpr float steamPageH()
{
    return kPillToggleH + kSteamRowGap + kRecStatusH + kSteamRowGap +
           static_cast<float>(kSteamNoteLines) * kSteamNoteLineH;
}

//--- clearances, asserted ---------------------------------------------------
static_assert(kTabW > 90.0f, "a tab is too narrow for a word like BLUETOOTH");
static_assert(kPageY > kTabBarY + kTabH, "the page would be drawn over the tab bar");
static_assert(windowH(devicesPageH(), true) <= kWinMaxH,
              "the devices page plus a message does not fit the tallest window");
static_assert(windowH(recorderPageH(), true) <= kWinMaxH,
              "the recorder page plus a message does not fit the tallest window");
static_assert(windowH(bluetoothPageH(), true) <= kWinMaxH,
              "the bluetooth page plus a message does not fit the tallest window");
static_assert(windowH(steamPageH(), true) <= kWinMaxH,
              "the steam page plus a message does not fit the tallest window");
static_assert(windowH(kMixMessageH, true) <= kWinMaxH,
              "the mixer placeholder does not fit the tallest window");
// The mixer page is the one page with no upper bound on its content -- the card decides how many
// controls there are -- so it is the one page that scrolls. Everything else is asserted to fit.
static_assert(windowH(kMixRowH * 3.0f, false) < kWinMaxH,
              "three mixer strips should never need the tallest window");

} // namespace geo
} // namespace jackbridge

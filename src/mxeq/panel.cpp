// See panel.h.

#include "panel.h"

#include "geometry.h"
#include "gfx/ink.h"
#include "gfx/palette.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace jackbridge
{

namespace
{

// The five Bluetooth action pills, in order. Indices match Panel::mBtActions.
enum BtAction { kPair = 0, kTrust = 1, kConnect = 2, kRemove = 3, kSetOutput = 4 };
const char *const kBtActionLabels[] = {"Pair", "Trust", "Connect", "Remove", "Set as Output"};

// Draw one line of text left-aligned at a baseline derived from the row.
void textLine(Canvas &c, float x, float cy, const char *s, uint32_t rgb, float size, float maxW)
{
    if (!s || !*s)
        return;
    c.setFont(Font::Body);
    c.setFontSize(size);
    c.setColor(rgb);
    const std::string t = c.clipToWidth(s, maxW);
    c.drawString(t.c_str(), x, cy + size * geo::kLabelBaselineBias);
}

// Draw a string that has newlines in it, one line per row. The placeholder messages and the Steam
// note are multi-line, and cairo's toy API draws exactly what it is given -- a '\n' is a glyph it
// does not have, so without splitting here the whole message renders as one line with a box in it.
float multilineText(Canvas &c, const Rect &area, const std::string &text, uint32_t rgb, float size,
                    float lineH, bool centred)
{
    c.setFont(Font::Body);
    c.setFontSize(size);
    c.setColor(rgb);

    float y = area.y + lineH * 0.5f;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos)
            eol = text.size();
        const std::string line = c.clipToWidth(text.substr(pos, eol - pos), area.w);
        const float x = centred ? area.centerX() - c.stringWidth(line.c_str()) * 0.5f : area.x;
        c.drawString(line.c_str(), x, y + size * geo::kLabelBaselineBias);
        y += lineH;
        if (eol == text.size())
            break;
        pos = eol + 1;
    }
    return y - area.y;
}

} // namespace

//------------------------------------------------------------------------
const char *Panel::pageLabel(Page p)
{
    switch (p) {
        case Page::Mixer:
            return "MIXER";
        case Page::Devices:
            return "DEVICES";
        case Page::Recorder:
            return "RECORD";
        case Page::Bluetooth:
            return "BLUETOOTH";
        case Page::Steam:
            return "STEAM";
    }
    return "";
}

void Panel::repaint()
{
    if (onNeedsRepaint)
        onNeedsRepaint();
}

void Panel::setPage(Page p)
{
    if (mPage == p)
        return;
    mPage = p;
    // A combo left open on the page you are leaving would keep drawing its popup over the new one.
    mRecChannels.close();
    mRecRate.close();
    for (EnumCell &e : mEnums)
        e.combo.close();
    clearHover();
    layout();
    repaint();

    // LAST, and after mPage has moved: the handler asks the panel how tall it is now.
    if (cb.pageChanged)
        cb.pageChanged(mPage);
}

//------------------------------------------------------------------------
void Panel::setMixer(std::vector<Strip> strips, std::vector<SwitchCell> switches,
                     std::vector<EnumCell> enums, bool useSwitchRow)
{
    mStrips = std::move(strips);
    mSwitches = std::move(switches);
    mEnums = std::move(enums);
    mUseSwitchRow = useSwitchRow;
    mMixerPlaceholder.clear();
    mMixerScroll = 0.0f;
    mDragStrip = -1;
    clearHover();
    layout();
    repaint();
}

void Panel::setMixerPlaceholder(const std::string &text)
{
    mStrips.clear();
    mSwitches.clear();
    mEnums.clear();
    mMixerPlaceholder = text;
    mMixerScroll = 0.0f;
    mDragStrip = -1;
    clearHover();
    layout();
    repaint();
}

void Panel::setDevices(std::vector<DeviceEntry> entries, int selected)
{
    mDevices = std::move(entries);
    mDeviceSelected = selected;
    for (size_t i = 0; i < mDevices.size(); ++i) {
        mDevices[i].radio.shape = Toggle::Shape::Radio;
        mDevices[i].radio.label = mDevices[i].label;
        mDevices[i].radio.on = static_cast<int>(i) == selected;
        mDevices[i].radio.enabled = mDevices[i].enabled;
    }
    layout();
    repaint();
}

void Panel::setRecorderCombos(std::vector<ComboItem> channels, std::vector<ComboItem> rates)
{
    mRecChannels.setItems(std::move(channels));
    mRecRate.setItems(std::move(rates));
    // Stereo and 48000: the defaults the GTK build's combos opened on, which are also what the ALSA
    // `jack` PCM is almost always running at.
    mRecChannels.setIndex(1);
    mRecRate.setIndex(1);
    repaint();
}

void Panel::setRecorderState(bool recording, const std::string &status)
{
    mRecording = recording;
    mRecStatus = status;
    // The filename and the format cannot change mid-recording: arecord already has them, and a
    // field that accepts a new name while the old one is being written is a field that lies.
    mRecFilename.setEnabled(!recording);
    mRecChannels.setEnabled(!recording);
    mRecRate.setEnabled(!recording);
    repaint();
}

void Panel::setRecorderFilename(const std::string &s)
{
    mRecFilename.setText(s);
    repaint();
}

int Panel::recorderChannels() const
{
    return mRecChannels.value() == "1" ? 1 : 2;
}

int Panel::recorderRate() const
{
    return mRecRate.value() == "44100" ? 44100 : 48000;
}

void Panel::setBluetoothDevices(std::vector<ListRow> rows)
{
    mBtList.setRows(std::move(rows));
    repaint();
}

void Panel::setBluetoothState(bool adapterReady, bool discoverable, bool discovering)
{
    mBtAdapterReady = adapterReady;
    mBtDiscovering = discovering;
    mBtDiscoverable.on = discoverable;
    mBtDiscoverable.enabled = adapterReady;
    repaint();
}

void Panel::setBluetoothSelectionState(bool haveSelection, bool paired, bool trusted,
                                       bool connected)
{
    mBtHaveSelection = haveSelection;
    mBtPaired = paired;
    mBtTrusted = trusted;
    mBtConnected = connected;
    repaint();
}

void Panel::setSteamState(bool active, const std::string &status)
{
    mSteamToggle.on = active;
    mSteamStatus = status;
    repaint();
}

void Panel::showMessage(const std::string &text, bool isError)
{
    mMessage = text;
    mMessageIsError = isError;
    layout(); // the strip changes the window's height
    repaint();
}

void Panel::clearMessage()
{
    if (mMessage.empty())
        return;
    mMessage.clear();
    layout();
    repaint();
}

//------------------------------------------------------------------------
float Panel::mixerContentH() const
{
    if (!mMixerPlaceholder.empty())
        return geo::kMixMessageH;
    if (mStrips.empty() && mSwitches.empty() && mEnums.empty())
        return geo::kMixMessageH;

    float h = static_cast<float>(mStrips.size()) * (geo::kMixRowH + geo::kMixRowGap);

    // THE DIVIDER AND THE SWITCH ROW CONTRIBUTE ZERO HEIGHT WHEN EMPTY. This is
    // mixer_sync_switch_row()'s rule, which was two gtk_widget_set_visible calls, expressed as a
    // layout condition instead.
    const bool anyAux = !mSwitches.empty() || !mEnums.empty();
    if (anyAux) {
        if (mUseSwitchRow)
            h += geo::kMixDividerGap;

        // Two cells per row, checkboxes first, then the enum rows which are taller.
        const int switchRows = (static_cast<int>(mSwitches.size()) + 1) / 2;
        h += static_cast<float>(switchRows) * (geo::kMixSwitchRowH + geo::kMixSwitchRowGap);
        const int enumRows = (static_cast<int>(mEnums.size()) + 1) / 2;
        h += static_cast<float>(enumRows) * (geo::kMixEnumRowH + geo::kMixSwitchRowGap);
    }
    return h;
}

bool Panel::mixerScrollable() const
{
    return mPage == Page::Mixer && mMixerViewport.h > 0.0f &&
           mixerContentH() > mMixerViewport.h + 0.5f;
}

//------------------------------------------------------------------------
float Panel::layout()
{
    // The tab bar is the same on every page, so it is laid out once here rather than in each
    // layoutX().
    for (int i = 0; i < geo::kPageCount; ++i) {
        mTabs[i].rect = Rect(geo::kMargin + static_cast<float>(i) * (geo::kTabW + geo::kTabGap),
                             geo::kTabBarY, geo::kTabW, geo::kTabH);
        mTabs[i].label = pageLabel(static_cast<Page>(i));
        mTabs[i].active = static_cast<Page>(i) == mPage;
        mTabs[i].enabled = true;
    }

    float pageH = 0.0f;
    switch (mPage) {
        case Page::Mixer: {
            // The mixer is the one page whose content is unbounded: the card decides how many
            // controls there are. So it gets whatever height is left up to the window's maximum and
            // scrolls past that, rather than growing the window off the screen.
            const float want = mixerContentH();
            const float avail = geo::kWinMaxH - geo::kPageY - geo::kPageBottomPad -
                                (mMessage.empty() ? 0.0f : geo::kStripGapAbove + geo::kStripH);
            // When it overflows, the last band of the page belongs to the "scroll for more" line
            // and the viewport stops above it -- so the hint never lands on a strip's own label.
            const bool overflows = want > avail;
            const float viewportH = overflows ? avail - geo::kMixHintH : want;
            pageH = overflows ? avail : want;
            mMixerViewport = Rect(geo::kMargin, geo::kPageY, geo::kContentW, viewportH);
            // Re-clamp the scroll: the viewport may have grown, and a scroll offset past the end
            // leaves a page of empty space below the last strip.
            mMixerScroll = std::clamp(mMixerScroll, 0.0f, std::max(0.0f, want - viewportH));
            layoutMixer(geo::kPageY - mMixerScroll);
            break;
        }
        case Page::Devices:
            pageH = geo::devicesPageH();
            layoutDevices(geo::kPageY);
            break;
        case Page::Recorder:
            pageH = geo::recorderPageH();
            layoutRecorder(geo::kPageY);
            break;
        case Page::Bluetooth:
            pageH = geo::bluetoothPageH();
            layoutBluetooth(geo::kPageY);
            break;
        case Page::Steam:
            pageH = geo::steamPageH();
            layoutSteam(geo::kPageY);
            break;
    }

    if (!mMessage.empty()) {
        mMessageRect = Rect(geo::kMargin, geo::kPageY + pageH + geo::kStripGapAbove,
                            geo::kContentW, geo::kStripH);
    } else {
        mMessageRect = Rect();
    }

    mHeight = std::clamp(geo::windowH(pageH, !mMessage.empty()), geo::kWinMinH, geo::kWinMaxH);
    return mHeight;
}

//------------------------------------------------------------------------
void Panel::layoutMixer(float y)
{
    const float x = geo::kMargin;

    for (Strip &s : mStrips) {
        const float grooveX = x + geo::kMixLabelW + geo::kMixLabelGap;
        s.slider.rect = Rect(grooveX, y, geo::kMixGrooveW, geo::kMixRowH);

        // The labelled switch at the right end: role 1 (Mute) or role 2 (Enable). The rect is the
        // whole cell, indicator and word together, because a 16-unit box is not a target.
        const float switchX = x + geo::kContentW - geo::kMixSwitchW;
        s.switchBox.rect = Rect(switchX, y, geo::kMixSwitchW, geo::kMixRowH);

        y += geo::kMixRowH + geo::kMixRowGap;
    }

    const bool anyAux = !mSwitches.empty() || !mEnums.empty();
    if (!anyAux)
        return;

    if (mUseSwitchRow)
        y += geo::kMixDividerGap;

    // Roles 3 and 4: a labelled checkbox per cell, two cells to a row, each labelled with the
    // element's own ALSA name.
    for (size_t i = 0; i < mSwitches.size(); ++i) {
        const int col = static_cast<int>(i) % 2;
        const float cx = x + static_cast<float>(col) * (geo::kMixSwitchCellW + geo::kMixSwitchRowGap);
        mSwitches[i].toggle.rect = Rect(cx, y, geo::kMixSwitchCellW, geo::kMixSwitchRowH);
        if (col == 1 || i + 1 == mSwitches.size())
            y += geo::kMixSwitchRowH + geo::kMixSwitchRowGap;
    }

    for (size_t i = 0; i < mEnums.size(); ++i) {
        const int col = static_cast<int>(i) % 2;
        const float cx = x + static_cast<float>(col) * (geo::kMixSwitchCellW + geo::kMixSwitchRowGap);
        const float comboX = cx + geo::kMixEnumLabelW + geo::kMixEnumGap;
        const float comboW = geo::kMixSwitchCellW - geo::kMixEnumLabelW - geo::kMixEnumGap;
        mEnums[i].combo.setRect(
            Rect(comboX, y + (geo::kMixEnumRowH - geo::kComboH) * 0.5f, comboW, geo::kComboH));
        if (col == 1 || i + 1 == mEnums.size())
            y += geo::kMixEnumRowH + geo::kMixSwitchRowGap;
    }
}

//------------------------------------------------------------------------
void Panel::layoutDevices(float y)
{
    for (DeviceEntry &d : mDevices) {
        d.radio.rect = Rect(geo::kMargin, y, geo::kContentW, geo::kDevRowH);
        y += geo::kDevRowH + geo::kDevDetailH + geo::kDevRowGap;
    }
}

//------------------------------------------------------------------------
void Panel::layoutRecorder(float y)
{
    const float fieldX = geo::kMargin + geo::kRecLabelW + geo::kMixLabelGap;
    mRecFilename.configure(Rect(fieldX, y, geo::kRecFieldW, geo::kFieldH),
                           "Alsa Sound Connect-....wav", false);
    y += geo::kFieldH + geo::kRecRowGap;

    mRecChannels.setRect(Rect(fieldX, y, geo::kRecComboW, geo::kComboH));
    y += geo::kComboH + geo::kRecRowGap;

    mRecRate.setRect(Rect(fieldX, y, geo::kRecComboW, geo::kComboH));
    y += geo::kComboH + geo::kRecRowGap;

    // Record and Stop, side by side. Fixed halves rather than measured widths: the two labels never
    // change, and a button that moves when its label changes is a button whose target moves.
    const float pillW = (geo::kContentW - geo::kPillGap) * 0.5f;
    mRecRecord.rect = Rect(geo::kMargin, y, pillW, geo::kPillH);
    mRecRecord.label = "Record";
    mRecStop.rect = Rect(geo::kMargin + pillW + geo::kPillGap, y, pillW, geo::kPillH);
    mRecStop.label = "Stop";
}

//------------------------------------------------------------------------
void Panel::layoutBluetooth(float y)
{
    // Discoverable on the left, Scan and Stop on the right.
    const float scanW = 80.0f;
    mBtScan.rect = Rect(geo::kMargin + geo::kContentW - 2.0f * scanW - geo::kPillGap, y, scanW,
                        geo::kPillH);
    mBtScan.label = "Scan";
    mBtStopScan.rect = Rect(geo::kMargin + geo::kContentW - scanW, y, scanW, geo::kPillH);
    mBtStopScan.label = "Stop";

    mBtDiscoverable.rect = Rect(geo::kMargin, y, geo::kContentW - 2.0f * scanW - 2.0f * geo::kPillGap,
                                geo::kPillH);
    mBtDiscoverable.label = "Discoverable";

    y += geo::kPillH + geo::kBtRowGap;

    mBtList.setRect(Rect(geo::kMargin, y, geo::kContentW, geo::kBtListH));
    y += geo::kBtListH + geo::kBtRowGap;

    for (int i = 0; i < geo::kBtActionCount; ++i) {
        mBtActions[i].rect =
            Rect(geo::kMargin + static_cast<float>(i) * (geo::kBtActionW + geo::kPillGap), y,
                 geo::kBtActionW, geo::kPillH);
        mBtActions[i].label = kBtActionLabels[i];
    }

    // THE GATING. Each action is drawn disabled rather than hidden when it cannot work, and the
    // conditions are the ones the GTK build checked with gui_bt_get_device_state before each call:
    // Trust and Connect both require Paired, and it reported "Device is not paired" as an error
    // dialog AFTER the click. Refusing before the click says the same thing without a dialog.
    mBtActions[kPair].enabled = mBtHaveSelection && !mBtPaired;
    mBtActions[kTrust].enabled = mBtHaveSelection && mBtPaired && !mBtTrusted;
    mBtActions[kConnect].enabled = mBtHaveSelection && mBtPaired && !mBtConnected;
    mBtActions[kRemove].enabled = mBtHaveSelection;
    mBtActions[kSetOutput].enabled = mBtHaveSelection && mBtConnected;

    mBtScan.enabled = mBtAdapterReady && !mBtDiscovering;
    mBtStopScan.enabled = mBtAdapterReady && mBtDiscovering;
    mBtScan.active = mBtDiscovering;
}

//------------------------------------------------------------------------
void Panel::layoutSteam(float y)
{
    mSteamToggle.rect = Rect(geo::kMargin, y, geo::kContentW, geo::kPillToggleH);
    mSteamToggle.label = "Steam gaming mode";
}

//------------------------------------------------------------------------
void Panel::draw(Canvas &c) const
{
    c.setColor(pal::kBgColor);
    c.fillRect(c.bounds());

    // The title, in the display face. Michroma at 15 is what the rest of the family puts at the top
    // of a window.
    c.setFont(Font::Title);
    c.setFontSize(geo::kTitleSize);
    c.setColor(pal::kTextColor);
    {
        const std::string t = c.clipToWidth("ALSA SOUND CONNECT", geo::kContentW);
        c.drawString(t.c_str(), geo::kMargin,
                     geo::kTitleY + geo::kTitleH * 0.5f + geo::kTitleSize * geo::kLabelBaselineBias);
    }

    for (int i = 0; i < geo::kPageCount; ++i)
        mTabs[i].draw(c);

    // The rule under the tab bar: gold, faint, the family's piping.
    c.setColor(pal::kGold, kHairlineAlpha);
    c.setPenSize(1.0f);
    c.strokeLine(geo::kMargin, geo::kPageRuleY, geo::kMargin + geo::kContentW, geo::kPageRuleY);

    switch (mPage) {
        case Page::Mixer:
            drawMixer(c);
            break;
        case Page::Devices:
            drawDevices(c);
            break;
        case Page::Recorder:
            drawRecorder(c);
            break;
        case Page::Bluetooth:
            drawBluetooth(c);
            break;
        case Page::Steam:
            drawSteam(c);
            break;
    }

    if (!mMessage.empty()) {
        const uint32_t rgb = mMessageIsError ? pal::kErrorColor : pal::kAccent;
        c.setColor(rgb, 40);
        c.fillRoundRect(mMessageRect, geo::kFieldRadius);
        c.setColor(rgb, kOutlineAlphaIdle);
        c.setPenSize(1.0f);
        c.strokeRoundRect(mMessageRect, geo::kFieldRadius);
        drawWrappedText(c,
                    Rect(mMessageRect.x + geo::kFieldPadX, mMessageRect.y + geo::kStripPadY,
                         mMessageRect.w - 2.0f * geo::kFieldPadX,
                         mMessageRect.h - 2.0f * geo::kStripPadY),
                    mMessage, rgb, geo::kStripTextSize, geo::kStripLineH, geo::kStripLines);
    }

    // The combo popups go LAST, over everything, because a popup that opened near the foot of the
    // page has to draw over what is under it. Only one can be open at a time.
    for (const EnumCell &e : mEnums) {
        if (e.combo.isOpen())
            e.combo.drawPopup(c);
    }
    if (mRecChannels.isOpen())
        mRecChannels.drawPopup(c);
    if (mRecRate.isOpen())
        mRecRate.drawPopup(c);
}

//------------------------------------------------------------------------
void Panel::drawMixer(Canvas &c) const
{
    if (!mMixerPlaceholder.empty() || (mStrips.empty() && mSwitches.empty() && mEnums.empty())) {
        const std::string text =
            mMixerPlaceholder.empty()
                ? std::string("No mixer controls were found on this card.\n"
                              "Audio may still work; the sliders are unavailable.\n"
                              "Check: cat /proc/asound/cards, then alsamixer")
                : mMixerPlaceholder;
        const Rect area(geo::kMargin, geo::kPageY, geo::kContentW, geo::kMixMessageH);
        multilineText(c, area, text, pal::kDimColor, geo::kMixMessageSize, geo::kMixMessageLineH,
                      true);
        return;
    }

    // CLIPPED TO THE VIEWPORT, so a scrolled strip is cut at the page's edge rather than drawn over
    // the message strip below it.
    c.pushClip(mMixerViewport);

    for (const Strip &s : mStrips) {
        // Skip a strip entirely outside the viewport: at 30 units a row, a USB interface with forty
        // controls would otherwise measure and clip forty labels to draw six.
        if (s.slider.rect.bottom() < mMixerViewport.y ||
            s.slider.rect.y > mMixerViewport.bottom())
            continue;

        textLine(c, geo::kMargin, s.slider.rect.centerY(), s.label.c_str(), pal::kTextColor,
                 geo::kMixLabelSize, geo::kMixLabelW);

        s.slider.draw(c);

        // The numeric readout, right-aligned in its slot so the digits do not shift as the value
        // crosses 10 and 100.
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", s.slider.value);
        c.setFont(Font::Body);
        c.setFontSize(geo::kMixValueSize);
        c.setColor(s.slider.enabled ? pal::kDimColor : pal::kDisabledColor);
        const float vx = s.slider.rect.right() + geo::kMixValueGap + geo::kMixValueW -
                         c.stringWidth(buf);
        c.drawString(buf, vx,
                     s.slider.rect.centerY() + geo::kMixValueSize * geo::kLabelBaselineBias);

        if (s.hasSwitch)
            s.switchBox.draw(c);
    }

    const bool anyAux = !mSwitches.empty() || !mEnums.empty();
    if (anyAux && mUseSwitchRow && !mStrips.empty()) {
        // The divider. Only on the internal card's two-zone layout, and only when something landed
        // below it -- which is mixer_sync_switch_row()'s rule.
        const float dy = mStrips.back().slider.rect.bottom() + geo::kMixDividerGap * 0.5f;
        c.setColor(pal::kGold, kHairlineAlpha);
        c.setPenSize(1.0f);
        c.strokeLine(geo::kMargin, dy, geo::kMargin + geo::kContentW, dy);
    }

    for (const SwitchCell &s : mSwitches)
        s.toggle.draw(c);

    for (const EnumCell &e : mEnums) {
        const Rect &r = e.combo.rect();
        textLine(c, geo::kMargin + (r.x > geo::kMargin + geo::kMixSwitchCellW
                                        ? geo::kMixSwitchCellW + geo::kMixSwitchRowGap
                                        : 0.0f),
                 r.centerY(), e.label.c_str(), pal::kDimColor, geo::kBodySize,
                 geo::kMixEnumLabelW);
        e.combo.drawClosed(c);
    }

    c.popClip();

    // The scroll hint, outside the clip: a card with more controls than fit needs to say so, and
    // there is no scrollbar on this page because the wheel is the only way to move it.
    if (mixerScrollable()) {
        const float total = mixerContentH();
        char buf[64];
        snprintf(buf, sizeof(buf), "scroll for more (%d controls)",
                 static_cast<int>(mStrips.size() + mSwitches.size() + mEnums.size()));
        c.setFont(Font::Body);
        c.setFontSize(geo::kMixHintSize);
        c.setColor(pal::kGold, 150);
        const float w = c.stringWidth(buf);
        c.drawString(buf, geo::kMargin + geo::kContentW - w,
                     mMixerViewport.bottom() + geo::kMixHintH * 0.5f +
                         geo::kMixHintSize * geo::kLabelBaselineBias);
        (void)total;
    }
}

//------------------------------------------------------------------------
void Panel::drawDevices(Canvas &c) const
{
    for (const DeviceEntry &d : mDevices) {
        d.radio.draw(c);
        // The detail line under each radio, indented past the indicator so it reads as belonging to
        // it. The GTK build had nowhere to put this.
        textLine(c, geo::kMargin + geo::kDevDetailX,
                 d.radio.rect.bottom() + geo::kDevDetailH * 0.5f, d.detail.c_str(),
                 d.enabled ? pal::kDimColor : pal::kDisabledColor, geo::kDevDetailSize,
                 geo::kContentW - geo::kDevDetailX);
    }
}

//------------------------------------------------------------------------
void Panel::drawRecorder(Canvas &c) const
{
    textLine(c, geo::kMargin, mRecFilename.rect().centerY(), "File", pal::kDimColor,
             geo::kBodySize, geo::kRecLabelW);
    mRecFilename.draw(c);

    textLine(c, geo::kMargin, mRecChannels.rect().centerY(), "Channels", pal::kDimColor,
             geo::kBodySize, geo::kRecLabelW);
    mRecChannels.drawClosed(c);

    textLine(c, geo::kMargin, mRecRate.rect().centerY(), "Rate", pal::kDimColor, geo::kBodySize,
             geo::kRecLabelW);
    mRecRate.drawClosed(c);

    mRecRecord.draw(c);
    mRecStop.draw(c);

    // The status line: "Idle", or "Recording... MM:SS", or where the file went.
    textLine(c, geo::kMargin, mRecRecord.rect.bottom() + geo::kRecRowGap + geo::kRecStatusH * 0.5f,
             mRecStatus.c_str(), mRecording ? pal::kAccent : pal::kDimColor, geo::kRecStatusSize,
             geo::kContentW);
}

//------------------------------------------------------------------------
void Panel::drawBluetooth(Canvas &c) const
{
    mBtDiscoverable.draw(c);
    mBtScan.draw(c);
    mBtStopScan.draw(c);
    mBtList.draw(c);
    for (int i = 0; i < geo::kBtActionCount; ++i)
        mBtActions[i].draw(c);

    if (!mBtAdapterReady) {
        // Drawn OVER the list, because an inert list with no explanation reads as a list with no
        // devices in it -- and "bluez is not running" is a different problem from "nothing in range".
        const Rect r = mBtList.rect();
        c.setColor(pal::kBgColor, 210);
        c.fillRoundRect(r, geo::kListRadius);
        multilineText(c, Rect(r.x + geo::kListPadX, r.centerY() - geo::kMixMessageLineH,
                              r.w - 2.0f * geo::kListPadX, geo::kMixMessageLineH * 2.0f),
                      "Bluetooth is unavailable.\nCheck that bluetoothd is running.",
                      pal::kDimColor, geo::kMixMessageSize, geo::kMixMessageLineH, true);
    }
}

//------------------------------------------------------------------------
void Panel::drawSteam(Canvas &c) const
{
    mSteamToggle.draw(c);

    float y = mSteamToggle.rect.bottom() + geo::kSteamRowGap;
    textLine(c, geo::kMargin, y + geo::kRecStatusH * 0.5f, mSteamStatus.c_str(),
             mSteamToggle.on ? pal::kAccent : pal::kDimColor, geo::kRecStatusSize,
             geo::kContentW);
    y += geo::kRecStatusH + geo::kSteamRowGap;

    multilineText(c,
                  Rect(geo::kMargin, y, geo::kContentW,
                       static_cast<float>(geo::kSteamNoteLines) * geo::kSteamNoteLineH),
                  "Steam does not work through apulse. This bridge is the workaround:\n"
                  "a minimal PulseAudio server that creates one JACK client,\n"
                  "pulse_bridge:playback_1/2. Other apps do not need it.",
                  pal::kDimColor, geo::kSteamNoteSize, geo::kSteamNoteLineH, false);
}

//------------------------------------------------------------------------
Panel::Target Panel::targetAt(float x, float y, int &index) const
{
    index = -1;

    // An open popup takes everything, including a click outside it -- which closes it and is
    // SWALLOWED, so the click does not also press whatever is under it. combo.h records that rule.
    for (size_t i = 0; i < mEnums.size(); ++i) {
        if (mEnums[i].combo.isOpen()) {
            index = static_cast<int>(i);
            return Target::EnumCombo;
        }
    }
    if (mRecChannels.isOpen())
        return Target::RecChannels;
    if (mRecRate.isOpen())
        return Target::RecRate;

    if (!mMessage.empty() && mMessageRect.contains(x, y))
        return Target::Message;

    for (int i = 0; i < geo::kPageCount; ++i) {
        if (mTabs[i].hit(x, y)) {
            index = i;
            return Target::Tab;
        }
    }

    switch (mPage) {
        case Page::Mixer: {
            if (!mMixerViewport.contains(x, y))
                break;
            for (size_t i = 0; i < mStrips.size(); ++i) {
                if (mStrips[i].hasSwitch && mStrips[i].switchBox.hit(x, y)) {
                    index = static_cast<int>(i);
                    return Target::StripSwitch;
                }
                if (mStrips[i].slider.hit(x, y)) {
                    index = static_cast<int>(i);
                    return Target::Slider;
                }
            }
            for (size_t i = 0; i < mSwitches.size(); ++i) {
                if (mSwitches[i].toggle.hit(x, y)) {
                    index = static_cast<int>(i);
                    return Target::SwitchCell;
                }
            }
            for (size_t i = 0; i < mEnums.size(); ++i) {
                if (mEnums[i].combo.hitClosed(x, y)) {
                    index = static_cast<int>(i);
                    return Target::EnumCombo;
                }
            }
            break;
        }
        case Page::Devices:
            for (size_t i = 0; i < mDevices.size(); ++i) {
                if (mDevices[i].radio.hit(x, y)) {
                    index = static_cast<int>(i);
                    return Target::DeviceRadio;
                }
            }
            break;
        case Page::Recorder:
            if (mRecRecord.hit(x, y))
                return Target::RecordButton;
            if (mRecStop.hit(x, y))
                return Target::StopButton;
            if (mRecChannels.hitClosed(x, y))
                return Target::RecChannels;
            if (mRecRate.hitClosed(x, y))
                return Target::RecRate;
            if (mRecFilename.rect().contains(x, y))
                return Target::RecFilename;
            break;
        case Page::Bluetooth:
            if (mBtDiscoverable.hit(x, y))
                return Target::BtDiscoverable;
            if (mBtScan.hit(x, y))
                return Target::BtScan;
            if (mBtStopScan.hit(x, y))
                return Target::BtStop;
            if (mBtList.rect().contains(x, y))
                return Target::BtList;
            for (int i = 0; i < geo::kBtActionCount; ++i) {
                if (mBtActions[i].hit(x, y)) {
                    index = i;
                    return Target::BtAction;
                }
            }
            break;
        case Page::Steam:
            if (mSteamToggle.hit(x, y))
                return Target::SteamToggle;
            break;
    }

    return Target::Nothing;
}

//------------------------------------------------------------------------
void Panel::clearHover()
{
    mHoverTarget = Target::Nothing;
    mHoverIndex = -1;
    for (int i = 0; i < geo::kPageCount; ++i)
        mTabs[i].hovered = false;
    for (Strip &s : mStrips) {
        s.slider.hovered = false;
        s.switchBox.hovered = false;
    }
    for (SwitchCell &s : mSwitches)
        s.toggle.hovered = false;
    for (EnumCell &e : mEnums)
        e.combo.hovered = false;
    for (DeviceEntry &d : mDevices)
        d.radio.hovered = false;
    mRecRecord.hovered = false;
    mRecStop.hovered = false;
    mRecChannels.hovered = false;
    mRecRate.hovered = false;
    mRecFilename.setHovered(false);
    mBtDiscoverable.hovered = false;
    mBtScan.hovered = false;
    mBtStopScan.hovered = false;
    for (int i = 0; i < geo::kBtActionCount; ++i)
        mBtActions[i].hovered = false;
    mSteamToggle.hovered = false;
}

//------------------------------------------------------------------------
void Panel::motion(float x, float y)
{
    // Dragging a slider takes priority over everything: the pointer may well be outside the groove
    // by now, and the thumb should still follow it.
    if (mDragStrip >= 0 && mDragStrip < static_cast<int>(mStrips.size())) {
        Strip &s = mStrips[static_cast<size_t>(mDragStrip)];
        const int v = s.slider.valueAt(x);
        if (v != s.slider.value) {
            s.slider.value = v;
            if (cb.setVolume)
                cb.setVolume(s.element, v);
            repaint();
        }
        return;
    }

    // An open popup gets the motion, so its own row highlight follows the pointer.
    for (EnumCell &e : mEnums) {
        if (e.combo.isOpen()) {
            e.combo.motion(x, y);
            repaint();
            return;
        }
    }
    if (mRecChannels.isOpen()) {
        mRecChannels.motion(x, y);
        repaint();
        return;
    }
    if (mRecRate.isOpen()) {
        mRecRate.motion(x, y);
        repaint();
        return;
    }

    if (mPage == Page::Bluetooth)
        mBtList.motion(x, y);

    int index = -1;
    const Target t = targetAt(x, y, index);
    if (t == mHoverTarget && index == mHoverIndex)
        return; // nothing changed: a pointer crossing the window costs no repaints

    clearHover();
    mHoverTarget = t;
    mHoverIndex = index;

    switch (t) {
        case Target::Tab:
            mTabs[index].hovered = true;
            break;
        case Target::Slider:
            mStrips[static_cast<size_t>(index)].slider.hovered = true;
            break;
        case Target::StripSwitch:
            mStrips[static_cast<size_t>(index)].switchBox.hovered = true;
            break;
        case Target::SwitchCell:
            mSwitches[static_cast<size_t>(index)].toggle.hovered = true;
            break;
        case Target::EnumCombo:
            mEnums[static_cast<size_t>(index)].combo.hovered = true;
            break;
        case Target::DeviceRadio:
            mDevices[static_cast<size_t>(index)].radio.hovered = true;
            break;
        case Target::RecordButton:
            mRecRecord.hovered = true;
            break;
        case Target::StopButton:
            mRecStop.hovered = true;
            break;
        case Target::RecChannels:
            mRecChannels.hovered = true;
            break;
        case Target::RecRate:
            mRecRate.hovered = true;
            break;
        case Target::RecFilename:
            mRecFilename.setHovered(true);
            break;
        case Target::BtDiscoverable:
            mBtDiscoverable.hovered = true;
            break;
        case Target::BtScan:
            mBtScan.hovered = true;
            break;
        case Target::BtStop:
            mBtStopScan.hovered = true;
            break;
        case Target::BtAction:
            mBtActions[index].hovered = true;
            break;
        case Target::SteamToggle:
            mSteamToggle.hovered = true;
            break;
        case Target::BtList:
        case Target::Message:
        case Target::Nothing:
            break;
    }
    repaint();
}

//------------------------------------------------------------------------
void Panel::press(float x, float y, int button)
{
    if (button != 1)
        return;

    int index = -1;
    const Target t = targetAt(x, y, index);

    // A popup's click is handled on PRESS, not on release, because that is what combo.h's
    // open/click/close contract expects -- and because a popup row is a menu row, where press and
    // release on different rows should not commit either.
    if (t == Target::EnumCombo && index >= 0 && mEnums[static_cast<size_t>(index)].combo.isOpen()) {
        Combo &combo = mEnums[static_cast<size_t>(index)].combo;
        if (combo.click(x, y)) {
            if (cb.setEnum)
                cb.setEnum(mEnums[static_cast<size_t>(index)].element, combo.index());
        }
        repaint();
        return;
    }
    if (t == Target::RecChannels && mRecChannels.isOpen()) {
        mRecChannels.click(x, y);
        repaint();
        return;
    }
    if (t == Target::RecRate && mRecRate.isOpen()) {
        mRecRate.click(x, y);
        repaint();
        return;
    }

    mPressTarget = t;
    mPressIndex = index;

    // A slider is the one control that acts on press: grabbing the thumb has to move it there, or a
    // click on the groove does nothing until you let go.
    if (t == Target::Slider) {
        mDragStrip = index;
        Strip &s = mStrips[static_cast<size_t>(index)];
        const int v = s.slider.valueAt(x);
        s.slider.dragging = true;
        if (v != s.slider.value) {
            s.slider.value = v;
            if (cb.setVolume)
                cb.setVolume(s.element, v);
        }
        repaint();
        return;
    }

    if (t == Target::BtList) {
        mBtList.press(x, y);
        repaint();
        return;
    }

    if (t == Target::RecFilename) {
        mRecFilename.handleClick(x, y);
        repaint();
        return;
    }
}

//------------------------------------------------------------------------
void Panel::release(float x, float y, int button)
{
    if (button != 1)
        return;

    if (mDragStrip >= 0) {
        if (mDragStrip < static_cast<int>(mStrips.size()))
            mStrips[static_cast<size_t>(mDragStrip)].slider.dragging = false;
        mDragStrip = -1;
        repaint();
        return;
    }

    if (mPressTarget == Target::BtList) {
        mBtList.release(x, y);
        mPressTarget = Target::Nothing;
        if (cb.deviceSelected)
            cb.deviceSelected(mBtList.selectedId());
        repaint();
        return;
    }

    int index = -1;
    const Target t = targetAt(x, y, index);

    const Target pressed = mPressTarget;
    const int pressedIndex = mPressIndex;
    mPressTarget = Target::Nothing;
    mPressIndex = -1;

    // PAIRED: act only if the release is on the same thing the press armed.
    if (t != pressed || index != pressedIndex)
        return;

    switch (t) {
        case Target::Tab:
            // setPage fires cb.pageChanged itself, AFTER mPage has moved. Firing it here instead
            // told the app the new page while the panel still held the old one, so the height it
            // computed was one tab behind -- every page was drawn at its predecessor's height.
            setPage(static_cast<Page>(index));
            return;

        case Target::StripSwitch: {
            Strip &s = mStrips[static_cast<size_t>(index)];
            s.switchBox.on = !s.switchBox.on;
            // THE INVERSION LIVES HERE. A box labelled "Mute" that is ticked means the element's
            // playback switch is OFF; a box labelled "Enable" that is ticked means its capture
            // switch is ON. The panel is what knows which word it drew, so the panel is what
            // inverts -- the GTK build had two handlers differing only in this.
            const bool alsaOn = s.switchBox.label == "Mute" ? !s.switchBox.on : s.switchBox.on;
            if (cb.setSwitch)
                cb.setSwitch(s.element, alsaOn);
            repaint();
            return;
        }

        case Target::SwitchCell: {
            SwitchCell &s = mSwitches[static_cast<size_t>(index)];
            s.toggle.on = !s.toggle.on;
            // Roles 3 and 4 are NOT inverted: the box carries the element's own name, so ticked
            // means on. This is the plain on_switch_toggled path.
            if (cb.setSwitch)
                cb.setSwitch(s.element, s.toggle.on);
            repaint();
            return;
        }

        case Target::EnumCombo:
            mEnums[static_cast<size_t>(index)].combo.open(
                Rect(0, 0, geo::kWinW, mHeight));
            repaint();
            return;

        case Target::DeviceRadio:
            if (cb.selectDevice)
                cb.selectDevice(index);
            return;

        case Target::RecordButton:
            if (cb.startRecording)
                cb.startRecording();
            return;
        case Target::StopButton:
            if (cb.stopRecording)
                cb.stopRecording();
            return;
        case Target::RecChannels:
            mRecChannels.open(Rect(0, 0, geo::kWinW, mHeight));
            repaint();
            return;
        case Target::RecRate:
            mRecRate.open(Rect(0, 0, geo::kWinW, mHeight));
            repaint();
            return;

        case Target::BtDiscoverable:
            if (cb.setDiscoverable)
                cb.setDiscoverable(!mBtDiscoverable.on);
            return;
        case Target::BtScan:
            if (cb.startScan)
                cb.startScan();
            return;
        case Target::BtStop:
            if (cb.stopScan)
                cb.stopScan();
            return;
        case Target::BtAction: {
            const std::string path = mBtList.selectedId();
            if (path.empty())
                return;
            switch (index) {
                case kPair:
                    if (cb.pairDevice)
                        cb.pairDevice(path);
                    return;
                case kTrust:
                    if (cb.trustDevice)
                        cb.trustDevice(path);
                    return;
                case kConnect:
                    if (cb.connectDevice)
                        cb.connectDevice(path);
                    return;
                case kRemove:
                    if (cb.removeDevice)
                        cb.removeDevice(path);
                    return;
                case kSetOutput:
                    if (cb.setBtOutput)
                        cb.setBtOutput(path);
                    return;
                default:
                    return;
            }
        }

        case Target::SteamToggle:
            if (cb.toggleSteam)
                cb.toggleSteam();
            return;

        case Target::Message:
            clearMessage();
            return;

        case Target::Slider:
        case Target::RecFilename:
        case Target::BtList:
        case Target::Nothing:
            return;
    }
}

//------------------------------------------------------------------------
bool Panel::key(Key k, const char *text, int len, unsigned state)
{
    (void)state;

    // An open popup gets the keyboard first.
    for (EnumCell &e : mEnums) {
        if (e.combo.isOpen()) {
            const bool handled = e.combo.key(k);
            if (handled && !e.combo.isOpen() && cb.setEnum)
                cb.setEnum(e.element, e.combo.index());
            repaint();
            return handled;
        }
    }
    if (mRecChannels.isOpen()) {
        const bool handled = mRecChannels.key(k);
        repaint();
        return handled;
    }
    if (mRecRate.isOpen()) {
        const bool handled = mRecRate.key(k);
        repaint();
        return handled;
    }

    // The filename field, when it has focus. THIS IS WHY THE WINDOW OPENS AN INPUT METHOD: a
    // filename can contain an accented character, and `text` is what XIM produced.
    if (mPage == Page::Recorder && mRecFilename.focused()) {
        if (mRecFilename.handleKey(k, text, len)) {
            repaint();
            return true;
        }
        if (k == Key::Enter) {
            if (cb.startRecording)
                cb.startRecording();
            return true;
        }
    }

    if (mPage == Page::Bluetooth && mBtList.key(k)) {
        if (cb.deviceSelected)
            cb.deviceSelected(mBtList.selectedId());
        repaint();
        return true;
    }

    // Tab and Shift+Tab move between pages, which is the one keyboard shortcut a tabbed window
    // should have.
    if (k == Key::Tab || k == Key::BackTab) {
        const int n = geo::kPageCount;
        const int cur = static_cast<int>(mPage);
        const int next = k == Key::Tab ? (cur + 1) % n : (cur + n - 1) % n;
        setPage(static_cast<Page>(next));
        if (cb.pageChanged)
            cb.pageChanged(mPage);
        return true;
    }

    // Escape clears a message rather than closing the window, if there is one to clear.
    if (k == Key::Escape && !mMessage.empty()) {
        clearMessage();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
bool Panel::scroll(float x, float y, int dir)
{
    if (mPage == Page::Bluetooth && mBtList.scroll(x, y, dir)) {
        repaint();
        return true;
    }

    if (mPage == Page::Mixer && mixerScrollable() && mMixerViewport.contains(x, y)) {
        const float maxScroll = std::max(0.0f, mixerContentH() - mMixerViewport.h);
        const float before = mMixerScroll;
        mMixerScroll =
            std::clamp(mMixerScroll + static_cast<float>(dir) * geo::kMixRowH * 3.0f, 0.0f,
                       maxScroll);
        if (mMixerScroll != before) {
            // Re-layout rather than offsetting at draw time, so the hit tests move with the ink.
            // A scrolled panel whose rects stayed put is a panel where clicking a slider adjusts a
            // different one.
            layout();
            repaint();
        }
        return true;
    }
    return false;
}

} // namespace jackbridge

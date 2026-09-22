// See listview.h.

#include "listview.h"

#include "ink.h"
#include "metrics.h"
#include "widgets.h"

#include <algorithm>
#include <cmath>

namespace jackbridge
{

namespace
{

inline float snap(float v)
{
    return std::floor(v + 0.5f);
}

// One wheel notch. Three rows rather than one: the Bluetooth list is 26 units a row and a
// one-row notch makes a list of a dozen devices feel stuck.
constexpr float kWheelStep = geo::kListRowH * 3.0f;

} // namespace

//------------------------------------------------------------------------
void ListView::setRect(const Rect &r)
{
    mRect = r;
    clampScroll();
}

void ListView::setRows(std::vector<ListRow> rows)
{
    // Keep the selection on the same THING. See the header: the GTK build updated rows in place
    // keyed on the object path, so its selection survived a refresh, and this is what keeps that
    // true now that the model is rebuilt rather than mutated.
    const std::string keep = selectedId();

    mRows = std::move(rows);
    mHover = -1;
    mArmed = -1;

    mSel = -1;
    if (!keep.empty()) {
        for (size_t i = 0; i < mRows.size(); ++i) {
            if (mRows[i].id == keep) {
                mSel = static_cast<int>(i);
                break;
            }
        }
    }

    clampScroll();
    if (mSel >= 0)
        scrollIntoView(mSel);
}

std::string ListView::selectedId() const
{
    if (mSel < 0 || mSel >= static_cast<int>(mRows.size()))
        return std::string();
    return mRows[static_cast<size_t>(mSel)].id;
}

void ListView::setSelected(int index)
{
    if (index < -1 || index >= static_cast<int>(mRows.size()))
        return;
    if (index == mSel)
        return;
    mSel = index;
    if (mSel >= 0)
        scrollIntoView(mSel);
}

//------------------------------------------------------------------------
float ListView::contentH() const
{
    return static_cast<float>(mRows.size()) * geo::kListRowH;
}

int ListView::visibleRows() const
{
    if (geo::kListRowH <= 0.0f)
        return 0;
    return static_cast<int>(mRect.h / geo::kListRowH);
}

bool ListView::scrollable() const
{
    return contentH() > mRect.h + 0.5f;
}

void ListView::clampScroll()
{
    const float maxScroll = std::max(0.0f, contentH() - mRect.h);
    mScroll = std::clamp(mScroll, 0.0f, maxScroll);
}

void ListView::scrollIntoView(int index)
{
    if (index < 0 || index >= static_cast<int>(mRows.size()))
        return;
    const float top = static_cast<float>(index) * geo::kListRowH;
    const float bottom = top + geo::kListRowH;
    if (top < mScroll)
        mScroll = top;
    else if (bottom > mScroll + mRect.h)
        mScroll = bottom - mRect.h;
    clampScroll();
}

//------------------------------------------------------------------------
Rect ListView::rowRect(int index) const
{
    const float y = mRect.y + static_cast<float>(index) * geo::kListRowH - mScroll;
    const float w = scrollable() ? mRect.w - geo::kScrollbarW - geo::kScrollbarInset : mRect.w;
    return Rect(mRect.x, snap(y), w, geo::kListRowH);
}

int ListView::rowAt(float x, float y) const
{
    if (!mRect.contains(x, y))
        return -1;
    // The scrollbar gutter is not a row: a click there scrolls and must not also select.
    if (scrollable() && x >= mRect.right() - geo::kScrollbarW - geo::kScrollbarInset)
        return -1;
    if (geo::kListRowH <= 0.0f)
        return -1;
    const int i = static_cast<int>((y - mRect.y + mScroll) / geo::kListRowH);
    if (i < 0 || i >= static_cast<int>(mRows.size()))
        return -1;
    return i;
}

//------------------------------------------------------------------------
void ListView::draw(Canvas &c) const
{
    // The viewport: a well, so the list reads as sunk into the page like a slider groove or a
    // text field rather than floating on it.
    c.setColor(pal::kWellColor);
    c.fillRoundRect(mRect, geo::kListRadius);

    // Clip BEFORE the rows, not after: a row scrolled half out of the viewport is cut at the
    // viewport's edge, and without this it would draw over whatever is laid out below the list.
    c.pushClip(mRect);

    const int first = std::max(0, static_cast<int>(mScroll / geo::kListRowH));
    const int last = std::min(static_cast<int>(mRows.size()),
                              first + visibleRows() + 2); // +2: partial rows top and bottom

    for (int i = first; i < last; ++i) {
        const ListRow &row = mRows[static_cast<size_t>(i)];
        const Rect r = rowRect(i);

        if (i == mSel) {
            c.setColor(pal::kAccent, 45);
            c.fillRoundRect(r.inset(1.0f), geo::kListRadius);
            c.setColor(pal::kAccent, kOutlineAlphaIdle);
            c.setPenSize(1.0f);
            c.strokeRoundRect(r.inset(1.0f), geo::kListRadius);
        } else if (i == mHover && row.enabled) {
            c.setColor(pal::kFaceColor);
            c.fillRoundRect(r.inset(1.0f), geo::kListRadius);
        }

        // The chips are laid out from the RIGHT, and the name is clipped to whatever is left, so
        // a long device name loses its tail rather than running under its own state badges.
        float chipsW = 0.0f;
        for (const ListChip &chip : row.chips) {
            if (chip.text.empty())
                continue;
            chipsW += chipWidth(c, chip.text.c_str()) + geo::kChipGap;
        }

        float cx = r.right() - geo::kListPadX - chipsW + geo::kChipGap;
        for (const ListChip &chip : row.chips) {
            if (chip.text.empty())
                continue;
            const uint32_t rgb = row.enabled ? chip.rgb : pal::kDisabledColor;
            cx += drawChip(c, cx, r.centerY(), chip.text.c_str(), rgb) + geo::kChipGap;
        }

        c.setFont(Font::Body);
        c.setFontSize(geo::kListTextSize);
        c.setColor(!row.enabled ? pal::kDisabledColor
                                : (i == mSel ? pal::kTextColor : pal::kDimColor));
        const float nameW =
            std::max(0.0f, r.w - 2.0f * geo::kListPadX - chipsW - geo::kChipGap);
        const std::string name = c.clipToWidth(row.label, nameW);
        c.drawString(name.c_str(), r.x + geo::kListPadX,
                     r.centerY() + geo::kListTextSize * geo::kLabelBaselineBias);
    }

    c.popClip();

    // The scrollbar, drawn only when there is something to scroll. Inside the viewport's right
    // edge rather than beside it, so the list's rect is the whole control and the panel does not
    // have to reserve a gutter that is usually empty.
    if (scrollable()) {
        const float trackX = mRect.right() - geo::kScrollbarW - geo::kScrollbarInset;
        const Rect track(trackX, mRect.y + geo::kScrollbarInset, geo::kScrollbarW,
                         mRect.h - 2.0f * geo::kScrollbarInset);
        c.setColor(pal::kBgColor);
        c.fillRoundRect(track, geo::kScrollbarW * 0.5f);

        const float frac = mRect.h / contentH();
        const float thumbH = std::max(geo::kScrollbarW * 2.0f, track.h * frac);
        const float maxScroll = contentH() - mRect.h;
        const float t = maxScroll > 0.0f ? mScroll / maxScroll : 0.0f;
        const Rect thumb(track.x, track.y + (track.h - thumbH) * t, track.w, thumbH);
        c.setColor(mDragBar ? pal::kAccent : pal::kDimColor, mDragBar ? 255 : kOutlineAlphaIdle);
        c.fillRoundRect(thumb, geo::kScrollbarW * 0.5f);
    }

    // The 1px outline last, over the rows, so a selected row's own outline cannot sit on top of
    // the viewport's edge.
    c.setColor(pal::kBorderColor, kOutlineAlphaIdle);
    c.setPenSize(1.0f);
    c.strokeRoundRect(mRect, geo::kListRadius);
}

//------------------------------------------------------------------------
void ListView::motion(float x, float y)
{
    if (mDragBar) {
        const float maxScroll = std::max(0.0f, contentH() - mRect.h);
        const float frac = contentH() > 0.0f ? mRect.h / contentH() : 1.0f;
        const float thumbH = std::max(geo::kScrollbarW * 2.0f, mRect.h * frac);
        const float travel = mRect.h - thumbH;
        if (travel > 0.0f)
            mScroll = mDragScroll0 + (y - mDragY0) / travel * maxScroll;
        clampScroll();
        return;
    }
    mHover = rowAt(x, y);
}

bool ListView::press(float x, float y)
{
    if (!mRect.contains(x, y))
        return false;

    if (scrollable() && x >= mRect.right() - geo::kScrollbarW - geo::kScrollbarInset) {
        mDragBar = true;
        mDragY0 = y;
        mDragScroll0 = mScroll;
        return true;
    }

    mArmed = rowAt(x, y);
    return true;
}

bool ListView::release(float x, float y)
{
    if (mDragBar) {
        mDragBar = false;
        return true;
    }

    const int armed = mArmed;
    mArmed = -1;
    if (armed < 0)
        return false;

    // Only if the release is still on the row the press armed. Menu's rule: a press that slid
    // off does not act.
    if (rowAt(x, y) != armed)
        return false;

    const ListRow &row = mRows[static_cast<size_t>(armed)];
    if (!row.enabled)
        return true; // consumed, but a disabled row does not become the selection

    if (armed != mSel) {
        mSel = armed;
        if (select)
            select(mSel);
    } else if (activate) {
        // A second click on the row that is already selected is the activate gesture. Simpler
        // than tracking double-click timing, and it is what a list of four Bluetooth devices
        // actually wants: click to select, click again to act.
        activate(mSel);
    }
    return true;
}

bool ListView::scroll(float x, float y, int dir)
{
    if (!mRect.contains(x, y))
        return false;
    if (!scrollable())
        return true; // inside the list: consumed, so the page behind does not also scroll
    mScroll += static_cast<float>(dir) * kWheelStep;
    clampScroll();
    mHover = rowAt(x, y);
    return true;
}

bool ListView::key(Key k)
{
    if (mRows.empty())
        return false;

    switch (k) {
        case Key::Up:
            if (mSel > 0) {
                mSel--;
                scrollIntoView(mSel);
                if (select)
                    select(mSel);
            }
            return true;
        case Key::Down:
            if (mSel + 1 < static_cast<int>(mRows.size())) {
                mSel++;
                scrollIntoView(mSel);
                if (select)
                    select(mSel);
            }
            return true;
        case Key::Home:
            if (mSel != 0) {
                mSel = 0;
                scrollIntoView(mSel);
                if (select)
                    select(mSel);
            }
            return true;
        case Key::End: {
            const int lastRow = static_cast<int>(mRows.size()) - 1;
            if (mSel != lastRow) {
                mSel = lastRow;
                scrollIntoView(mSel);
                if (select)
                    select(mSel);
            }
            return true;
        }
        case Key::Enter:
            if (mSel >= 0 && activate)
                activate(mSel);
            return mSel >= 0;
        default:
            return false;
    }
}

} // namespace jackbridge

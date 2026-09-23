// ListView -- a scrollable list with one selected row, per-row state chips, and nothing else.
//
// NEW IN THIS PROJECT. Nothing in the family has one: simple-login-gui's Menu is a transient
// popup with no scrollbar and no persistent selection, and Audio-Gui's longest list is a combo's
// popup, which is also transient and also cannot scroll. So this is written rather than ported,
// and it deliberately follows Menu's idioms -- rowAt() as the single source of truth for which
// row a point is in, and a press paired with a release -- so it reads as a sibling.
//
// It replaces the GtkTreeView + GtkListStore the Bluetooth panel used, and that replacement is
// the reason three functions in the GTK build cease to exist:
//
//   * A ROW IS NOT A STRING. GtkListStore held two strings per row, so the device's state had to
//     be encoded into its visible name -- " [Paired]", " [Trusted]", " [Connected]" appended on
//     write and stripped off again on read by strip_state_markers(), with a "* " prefix parsed
//     the same way. A device that calls itself "My [Paired] Speaker" corrupts its own row under
//     that scheme. Here the name is a string and the state is a vector of chips, and they never
//     meet.
//
//   * A ROW HAS AN IDENTITY. `id` is the BlueZ object path, and setRows() re-finds the selected
//     row by it. This PRESERVES the GTK behaviour rather than changing it: gui_bt_add_device()
//     looked each device up by object_path and updated the row in place, so the GtkTreeView's
//     selection survived a mid-scan refresh. Rebuilding a plain vector every time
//     InterfacesAdded fires would lose it, and losing it means the selection moves under the
//     pointer while somebody is reaching for Pair.
//
//   * A ROW CAN BE DISABLED. Drawn, never hidden -- palette.h states the rule.
//
// LIKE EVERY CONTROL HERE IT CARRIES ITS RECT (widgets.h says why), and it clips its rows to it
// with Canvas::pushClip, so a half-scrolled row is cut off at the viewport edge instead of
// drawing over the buttons underneath.

#pragma once

#include "canvas.h"
#include "keys.h"
#include "palette.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

// One state badge on a row. Not interactive: see widgets.h's drawChip.
struct ListChip {
    std::string text;
    uint32_t rgb = pal::kDimColor;
};

struct ListRow {
    // The caller's stable identity for this row -- the BlueZ object path, for the one list this
    // project has. Never shown. setRows() uses it to keep the selection on the same THING rather
    // than on the same index.
    std::string id;
    std::string label;
    std::vector<ListChip> chips;
    bool enabled = true;
};

class ListView
{
public:
    // Fired when the selected row changes, by click or by key, with the new index or -1. Fired
    // AFTER the selection has moved, so a handler is free to read selected() and re-gate the
    // buttons beside the list -- which is what the GTK build did from the tree's "changed"
    // signal.
    std::function<void(int index)> select;

    // A double click, or Return on the selected row. The Bluetooth panel maps this to its
    // primary action for the row's state, which is what a double click did before.
    std::function<void(int index)> activate;

    void setRect(const Rect &r);
    Rect rect() const
    {
        return mRect;
    }

    // Replaces the rows, keeping the selection on the same `id` if it is still present and
    // clearing it if it is not. Does NOT fire select(): the caller is the one rebuilding the
    // model and already knows.
    void setRows(std::vector<ListRow> rows);

    const std::vector<ListRow> &rows() const
    {
        return mRows;
    }

    int selected() const
    {
        return mSel;
    }
    // The selected row's id, or an empty string. This is what the panel passes to BlueZ, because
    // an index into a list that BlueZ is concurrently adding to is not an identity.
    std::string selectedId() const;
    void setSelected(int index);

    void draw(Canvas &c) const;

    //--- input ---------------------------------------------------------
    void motion(float x, float y);
    // True when the press landed inside the viewport, so the panel knows the click was consumed.
    bool press(float x, float y);
    // Completes a click only if the release is on the SAME row the press armed -- Menu's rule,
    // and the reason a press that slides off a row does not select it.
    bool release(float x, float y);
    // `dir` is -1 for a wheel notch up and +1 for down, as X11Window::Callbacks::scroll gives it.
    bool scroll(float x, float y, int dir);
    bool key(Key k);

    //--- geometry, exposed for the layout audit ------------------------
    // Which row index a point is in, or -1. The single source of truth: draw(), press() and
    // release() all go through it, so the painter and the hit test cannot drift apart.
    int rowAt(float x, float y) const;
    Rect rowRect(int index) const;
    // How many whole rows the viewport shows. The panel sizes the viewport from this.
    int visibleRows() const;
    // True when the content is taller than the viewport, i.e. when a scrollbar is drawn.
    bool scrollable() const;

private:
    void clampScroll();
    void scrollIntoView(int index);
    float contentH() const;

    Rect mRect;
    std::vector<ListRow> mRows;

    // In logical units from the top of the content, NOT in rows: a wheel notch moves by a fixed
    // distance and a drag moves by however far the pointer went, and neither is a whole row.
    float mScroll = 0.0f;

    int mSel = -1;
    int mHover = -1;
    // The row a press landed on, paired with the release that completes the click.
    int mArmed = -1;

    // Dragging the scrollbar, not the rows: the list scrolls with the wheel and with the bar,
    // and a press on a row is a selection rather than the start of a kinetic scroll. Dragging
    // rows would make selecting one on a touchpad a gamble.
    bool mDragBar = false;
    float mDragY0 = 0.0f;
    float mDragScroll0 = 0.0f;
};

} // namespace jackbridge

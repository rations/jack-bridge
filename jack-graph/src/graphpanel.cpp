// See graphpanel.h.

#include "graphpanel.h"

#include "graphgeometry.h"
#include "gfx/ink.h"
#include "gfx/palette.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

namespace jackbridge
{

namespace
{

// Move each "<base>R" port to sit immediately after its "<base>" port within the same column. Pure
// relocation -- all other ports keep their order, so MIDI stays on top and unrelated clients are
// unaffected. O(n^2) but n is tiny.
//
// MOVED ACROSS VERBATIM from GraphCanvas.cpp. It is what puts a stereo pair next to each other
// instead of at opposite ends of a box, and the reason it is a relocation rather than a sort is
// that a sort would reorder everything else too.
void pairStereoPorts(std::vector<std::shared_ptr<Node>> &ports)
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < ports.size(); ++i) {
            const std::string &nm = ports[i]->name; // full "client:port"
            if (nm.empty() || nm.back() != 'R')
                continue;
            std::string base = nm.substr(0, nm.size() - 1); // drop trailing 'R'

            size_t b = SIZE_MAX;
            for (size_t j = 0; j < ports.size(); ++j) {
                if (ports[j]->name == base) {
                    b = j;
                    break;
                }
            }
            if (b == SIZE_MAX)
                continue; // no matching base -- leave it alone
            if (i == b + 1)
                continue; // already right after its base

            auto node = ports[i];
            ports.erase(ports.begin() + static_cast<long>(i));
            if (b > i)
                --b; // base shifted left by the erase
            ports.insert(ports.begin() + static_cast<long>(b) + 1, node);
            changed = true; // indices moved -- rescan
            break;
        }
    }
}

constexpr float kBoxRadius = 6.0f;
constexpr float kPortRadius = 6.0f;
constexpr float kPortBgRadius = 3.0f;
constexpr float kHeaderTextSize = 11.0f;
constexpr float kPortTextSize = 9.0f;
constexpr float kLayoutMargin = 20.0f;
constexpr float kBoxRowGap = 10.0f;
constexpr float kBoxColGap = 250.0f;

} // namespace

//------------------------------------------------------------------------
void GraphPanel::setRect(const Rect &r)
{
    mRect = r;
}

void GraphPanel::clear()
{
    mNodes.clear();
    mConnections.clear();
    mClientBoxes.clear();
    repaint();
}

void GraphPanel::addNode(std::shared_ptr<Node> node)
{
    mNodes.push_back(std::move(node));
}

void GraphPanel::addConnection(std::shared_ptr<Connection> conn)
{
    mConnections.push_back(std::move(conn));
}

void GraphPanel::removeAll()
{
    for (const ClientBox &box : mClientBoxes)
        mSavedPositions[box.client_name] = {box.x, box.y};
    mNodes.clear();
    mConnections.clear();
    mClientBoxes.clear();

    // NOT A DANGLING POINTER WAITING TO HAPPEN. mMovingBoxPtr points into mClientBoxes, which has
    // just been emptied; a refresh landing mid-drag would otherwise leave the next motion event
    // writing through it. GraphCanvas got away with this because GTK delivered the refresh from a
    // timer that could not interleave with a drag; there is no such guarantee here.
    mMovingBox = false;
    mMovingBoxPtr = nullptr;

    repaint();
}

void GraphPanel::setZoom(double zoom)
{
    mZoom = std::max(geo::kZoomMin, std::min(zoom, geo::kZoomMax));
    repaint();
}

//------------------------------------------------------------------------
void GraphPanel::fitToWindow()
{
    if (mClientBoxes.empty())
        return;

    double minX = mClientBoxes[0].x;
    double minY = mClientBoxes[0].y;
    double maxX = mClientBoxes[0].x + mClientBoxes[0].width;
    double maxY = mClientBoxes[0].y + mClientBoxes[0].height;

    for (const ClientBox &box : mClientBoxes) {
        minX = std::min(minX, box.x);
        minY = std::min(minY, box.y);
        maxX = std::max(maxX, box.x + box.width);
        maxY = std::max(maxY, box.y + box.height);
    }

    const double contentW = maxX - minX;
    const double contentH = maxY - minY;
    if (contentW <= 0.0 || contentH <= 0.0)
        return;

    // THE VIEWPORT IS THIS PANEL'S OWN RECT. GraphCanvas asked get_parent()->get_allocation() for
    // it, which is the one line of that function that could not come across: there is no parent
    // widget, and the window tells the panel its rect on every ConfigureNotify instead.
    const double viewW = mRect.w;
    const double viewH = mRect.h;
    if (viewW <= 1.0 || viewH <= 1.0)
        return;

    const double newZoom = std::min((viewW - geo::kFitMargin * 2.0) / contentW,
                                    (viewH - geo::kFitMargin * 2.0) / contentH);
    mZoom = std::max(geo::kZoomMin, std::min(newZoom, geo::kFitZoomMax));

    // screen = canvas * zoom + pan  =>  pan = screen_centre - canvas_centre * zoom
    mPanX = viewW / 2.0 - ((minX + maxX) / 2.0) * mZoom;
    mPanY = viewH / 2.0 - ((minY + maxY) / 2.0) * mZoom;

    repaint();
}

//------------------------------------------------------------------------
void GraphPanel::buildClientBoxes()
{
    mClientBoxes.clear();

    std::map<std::string, std::unique_ptr<ClientBox>> boxes;

    for (auto &node : mNodes) {
        const std::string cname = node->client_name;
        if (cname.empty())
            continue;
        if (boxes.find(cname) == boxes.end())
            boxes[cname] = std::make_unique<ClientBox>(cname, node->is_alsa);
        boxes[cname]->add_port(node);
    }

    for (auto &entry : boxes) {
        pairStereoPorts(entry.second->inputs);
        pairStereoPorts(entry.second->outputs);
        mClientBoxes.push_back(std::move(*entry.second));
    }
}

void GraphPanel::positionPorts(ClientBox *box) const
{
    for (size_t i = 0; i < box->inputs.size(); ++i) {
        box->inputs[i]->x = box->x + ClientBox::SIDE_PAD;
        box->inputs[i]->y = box->y + ClientBox::HEADER_HEIGHT + ClientBox::PORT_PAD +
                            static_cast<double>(i) *
                                (ClientBox::PORT_HEIGHT + ClientBox::PORT_PAD);
        box->inputs[i]->width = ClientBox::COL_WIDTH;
        box->inputs[i]->height = ClientBox::PORT_HEIGHT;
    }
    for (size_t i = 0; i < box->outputs.size(); ++i) {
        box->outputs[i]->x =
            box->x + ClientBox::SIDE_PAD + ClientBox::COL_WIDTH + ClientBox::SIDE_PAD;
        box->outputs[i]->y = box->y + ClientBox::HEADER_HEIGHT + ClientBox::PORT_PAD +
                             static_cast<double>(i) *
                                 (ClientBox::PORT_HEIGHT + ClientBox::PORT_PAD);
        box->outputs[i]->width = ClientBox::COL_WIDTH;
        box->outputs[i]->height = ClientBox::PORT_HEIGHT;
    }
}

void GraphPanel::layout(bool preservePositions)
{
    if (preservePositions) {
        for (const ClientBox &box : mClientBoxes)
            mSavedPositions[box.client_name] = {box.x, box.y};
    }

    buildClientBoxes();

    // Split boxes by signal role:
    //   sources -- OUTPUT ports only  -> left column  (system:capture, apps)
    //   sinks   -- INPUT ports only   -> right column (system:playback, hdmi_out, bluealsa)
    //   mixed   -- both directions    -> middle column
    std::vector<ClientBox *> sources, sinks, mixed;
    for (ClientBox &box : mClientBoxes) {
        if (!box.outputs.empty() && box.inputs.empty())
            sources.push_back(&box);
        else if (box.outputs.empty() && !box.inputs.empty())
            sinks.push_back(&box);
        else
            mixed.push_back(&box);
    }

    // Sort each column so MIDI clients appear above audio clients.
    auto isMidi = [](const ClientBox *b) {
        for (const auto &p : b->inputs) {
            if (p->type == PortType::MIDI)
                return true;
        }
        for (const auto &p : b->outputs) {
            if (p->type == PortType::MIDI)
                return true;
        }
        return false;
    };
    auto midiFirst = [&](const ClientBox *a, const ClientBox *b) { return isMidi(a) > isMidi(b); };
    std::stable_sort(sources.begin(), sources.end(), midiFirst);
    std::stable_sort(sinks.begin(), sinks.end(), midiFirst);
    std::stable_sort(mixed.begin(), mixed.end(), midiFirst);

    double maxBoxWidth = 0.0;
    for (const ClientBox &box : mClientBoxes)
        maxBoxWidth = std::max(maxBoxWidth, box.width);

    const double leftX = kLayoutMargin;
    const double midX = kLayoutMargin + maxBoxWidth + kBoxColGap;
    const double rightX = kLayoutMargin + maxBoxWidth * 2.0 + kBoxColGap * 2.0;

    double leftY = kLayoutMargin;
    double midY = kLayoutMargin;
    double rightY = kLayoutMargin;

    // Pass 1: restore saved boxes at their saved positions and find the lowest occupied Y in each
    // column so new boxes never overlap them.
    auto placeSaved = [&](ClientBox *box, double &colFloor) {
        auto it = mSavedPositions.find(box->client_name);
        if (it == mSavedPositions.end())
            return;
        box->x = it->second.x;
        box->y = it->second.y;
        positionPorts(box);
        colFloor = std::max(colFloor, box->y + box->height + kBoxRowGap);
    };

    for (ClientBox *box : sources)
        placeSaved(box, leftY);
    for (ClientBox *box : mixed)
        placeSaved(box, midY);
    for (ClientBox *box : sinks)
        placeSaved(box, rightY);

    // Pass 2: place new (unsaved) boxes below all saved content in their column.
    auto placeNew = [&](ClientBox *box, double colX, double &colY) {
        if (mSavedPositions.count(box->client_name))
            return;
        box->x = colX;
        box->y = colY;
        positionPorts(box);
        colY += box->height + kBoxRowGap;
    };

    for (ClientBox *box : sources)
        placeNew(box, leftX, leftY);
    for (ClientBox *box : mixed)
        placeNew(box, midX, midY);
    for (ClientBox *box : sinks)
        placeNew(box, rightX, rightY);

    // GraphCanvas ended here by calling get_parent()->set_size_request() with the content bounds,
    // which grew the Gtk::ScrolledWindow's scrollable area. There is no scrolled window: this panel
    // pans itself and always did, so the request has nothing to ask and nothing to ask it of.

    repaint();
}

//------------------------------------------------------------------------
void GraphPanel::draw(Canvas &c) const
{
    c.setColor(pal::graph::kGround);
    c.fillRect(mRect);

    // CLIPPED TO THE VIEWPORT, then panned and zoomed. Both have to be inside the same save/restore
    // as the drawing, and the clip has to come first: a box dragged off the top of the canvas would
    // otherwise be drawn over the toolbar.
    c.pushClip(mRect);
    cairo_save(c.cr());
    cairo_translate(c.cr(), mRect.x + mPanX, mRect.y + mPanY);
    cairo_scale(c.cr(), mZoom, mZoom);

    // Connections UNDER the boxes, so a cable passing behind a client does not draw over its ports.
    for (const auto &conn : mConnections)
        drawConnection(c, *conn);

    for (const ClientBox &box : mClientBoxes)
        drawClientBox(c, box);

    if (mDragging && mDragSource)
        drawDragPreview(c);

    cairo_restore(c.cr());
    c.popClip();
}

void GraphPanel::drawClientBox(Canvas &c, const ClientBox &box) const
{
    const Rect r(static_cast<float>(box.x), static_cast<float>(box.y),
                 static_cast<float>(box.width), static_cast<float>(box.height));

    c.setColor(pal::graph::kBoxFill, 242); // 0.95
    c.fillRoundRect(r, kBoxRadius);
    c.setColor(pal::graph::kBoxBorder, 204); // 0.8
    c.setPenSize(1.5f);
    c.strokeRoundRect(r, kBoxRadius);

    // The rule under the header.
    c.setColor(pal::graph::kBoxBorder, 128); // 0.5
    c.setPenSize(1.0f);
    c.strokeLine(r.x, r.y + ClientBox::HEADER_HEIGHT, r.right(),
                 r.y + ClientBox::HEADER_HEIGHT);

    // The client name, centred in the header.
    //
    // NOT BOLD, and not in the display face. The old header was Pango WEIGHT_BOLD, and the bundled
    // stack has one weight of Roboto and Michroma -- which is a display face far too wide for a
    // 140-unit column. Drawing it in the brighter kHeaderText against the ports' dimmer kPortText
    // is what separates the two here, which is the distinction the weight was carrying.
    c.setFont(Font::Body);
    c.setFontSize(kHeaderTextSize);
    c.setColor(pal::graph::kHeaderText);
    {
        const std::string name = c.clipToWidth(box.client_name, r.w - 8.0f);
        const float w = c.stringWidth(name.c_str());
        c.drawString(name.c_str(), r.centerX() - w * 0.5f,
                     r.y + ClientBox::HEADER_HEIGHT * 0.5f +
                         kHeaderTextSize * geo::kLabelBaselineBias);
    }

    const float inX = r.x + ClientBox::SIDE_PAD;
    const float outX = r.x + ClientBox::SIDE_PAD + ClientBox::COL_WIDTH + ClientBox::SIDE_PAD;

    float portY = r.y + ClientBox::HEADER_HEIGHT + ClientBox::PORT_PAD;
    for (const auto &node : box.inputs) {
        drawPort(c, *node, inX, portY, false);
        portY += ClientBox::PORT_HEIGHT + ClientBox::PORT_PAD;
    }

    portY = r.y + ClientBox::HEADER_HEIGHT + ClientBox::PORT_PAD;
    for (const auto &node : box.outputs) {
        drawPort(c, *node, outX, portY, true);
        portY += ClientBox::PORT_HEIGHT + ClientBox::PORT_PAD;
    }
}

void GraphPanel::drawPort(Canvas &c, const Node &node, float x, float y, bool isOutput) const
{
    const float w = ClientBox::PORT_BG_WIDTH;
    const float h = ClientBox::PORT_HEIGHT;

    // BLUE IS AUDIO, GREEN IS MIDI, and it is the only thing on screen saying which a port is.
    const uint32_t rgb =
        node.type == PortType::AUDIO ? pal::graph::kPortAudio : pal::graph::kPortMidi;

    // An output port's background extends LEFT of its column, so the two columns' backgrounds meet
    // their own edge of the box and the dots sit on the outside.
    const float bgX =
        isOutput ? x - (ClientBox::PORT_BG_WIDTH - ClientBox::COL_WIDTH) : x;
    const Rect bg(bgX, y, w, h);

    c.setColor(rgb, 31); // 0.12
    c.fillRoundRect(bg, kPortBgRadius);

    const float dotX = isOutput ? bg.right() : bg.x;
    c.setColor(rgb, 242); // 0.95
    c.fillEllipse(dotX, y + h * 0.5f, kPortRadius, kPortRadius);

    c.setFont(Font::Body);
    c.setFontSize(kPortTextSize);
    c.setColor(pal::graph::kPortText);

    // clipToWidth replaces the hand-written truncation loop, which re-measured the string once per
    // character removed and appended "..." rather than an ellipsis. Same result, one measurement,
    // and it cuts on whole UTF-8 characters -- a JACK client may be named in any encoding it likes.
    const float maxTextW = w - kPortRadius * 2.0f - 12.0f;
    const std::string text = c.clipToWidth(node.display_name(), maxTextW);
    const float tw = c.stringWidth(text.c_str());
    const float textX = isOutput ? bg.right() - kPortRadius - 4.0f - tw
                                 : bg.x + kPortRadius + 4.0f;
    c.drawString(text.c_str(), textX, y + h * 0.5f + kPortTextSize * geo::kLabelBaselineBias);
}

void GraphPanel::drawConnection(Canvas &c, const Connection &conn) const
{
    const float x1 = static_cast<float>(conn.source->x + conn.source->width);
    const float y1 = static_cast<float>(conn.source->y + conn.source->height / 2.0);
    const float x2 = static_cast<float>(conn.destination->x);
    const float y2 = static_cast<float>(conn.destination->y + conn.destination->height / 2.0);

    c.setColor(pal::graph::kConnector, 128); // 0.5
    c.setPenSize(2.0f);
    // The slack the old curve_to used: half the horizontal distance, on both control points.
    c.strokeConnector(x1, y1, x2, y2, std::fabs(x2 - x1) * 0.5f);
}

void GraphPanel::drawDragPreview(Canvas &c) const
{
    const float x1 = static_cast<float>(mDragSource->x + mDragSource->width);
    const float y1 = static_cast<float>(mDragSource->y + mDragSource->height / 2.0);
    const float x2 = static_cast<float>(mDragCurrentX);
    const float y2 = static_cast<float>(mDragCurrentY);

    // DASHED, and that is the whole point of the stroke: it is what says this cable is proposed
    // rather than made. Drawing it solid would make an in-flight drag indistinguishable from a
    // connection that already exists.
    static const float kDashes[] = {5.0f, 3.0f};
    c.setColor(pal::graph::kPreview, 178); // 0.7
    c.setPenSize(2.0f);
    c.setDash(kDashes, 2);
    c.strokeConnector(x1, y1, x2, y2, std::fabs(x2 - x1) * 0.5f);
    c.clearDash();
}

//------------------------------------------------------------------------
std::shared_ptr<Node> GraphPanel::outputPortAt(double x, double y)
{
    for (ClientBox &box : mClientBoxes) {
        if (x >= box.x && x <= box.x + box.width && y >= box.y && y <= box.y + box.height)
            return box.find_output_at(x - box.x, y - box.y);
    }
    return nullptr;
}

std::shared_ptr<Node> GraphPanel::inputPortAt(double x, double y)
{
    for (ClientBox &box : mClientBoxes) {
        if (x >= box.x && x <= box.x + box.width && y >= box.y && y <= box.y + box.height)
            return box.find_input_at(x - box.x, y - box.y);
    }
    return nullptr;
}

ClientBox *GraphPanel::boxAt(double x, double y)
{
    for (ClientBox &box : mClientBoxes) {
        if (box.contains(x, y))
            return &box;
    }
    return nullptr;
}

//------------------------------------------------------------------------
bool GraphPanel::press(float wx, float wy, int button)
{
    if (!mRect.contains(wx, wy))
        return false;

    const double x = canvasX(wx);
    const double y = canvasY(wy);

    if (button == 1) {
        // A PORT FIRST, THEN A BOX, THEN THE GROUND. The order is the whole interaction: a port dot
        // sits inside its box, so testing the box first would make every port drag a box move.
        if (auto output = outputPortAt(x, y)) {
            mDragSource = output;
            mDragCurrentX = x;
            mDragCurrentY = y;
            mDragging = true;
            repaint();
            return true;
        }

        if (ClientBox *box = boxAt(x, y)) {
            mMovingBox = true;
            mMovingBoxPtr = box;
            mBoxOffsetX = x - box->x;
            mBoxOffsetY = y - box->y;
            return true;
        }

        mPanning = true;
        mPanStartX = wx;
        mPanStartY = wy;
        return true;
    }

    if (button == 3) {
        // RIGHT-CLICK ON A CABLE DISCONNECTS IT. The test is distance to the straight line between
        // the two endpoints rather than to the drawn curve, which is an approximation and was one
        // before the port too -- the curve never strays more than a few units from the chord at the
        // distances involved, and the 10-unit tolerance covers it.
        for (auto it = mConnections.begin(); it != mConnections.end(); ++it) {
            const auto &conn = *it;
            const double x1 = conn->source->x + conn->source->width;
            const double y1 = conn->source->y + conn->source->height / 2.0;
            const double x2 = conn->destination->x;
            const double y2 = conn->destination->y + conn->destination->height / 2.0;

            const double num = std::fabs((y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1);
            const double den = std::sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
            const double dist = den > 0.0 ? num / den : num;

            if (dist < 10.0 && x >= std::min(x1, x2) - 10.0 && x <= std::max(x1, x2) + 10.0) {
                if (onDisconnect)
                    onDisconnect(conn->source->full_name(), conn->destination->full_name());
                mConnections.erase(it);
                repaint();
                return true;
            }
        }
    }

    return false;
}

bool GraphPanel::release(float wx, float wy, int button)
{
    if (mDragging && mDragSource && button == 1) {
        const double x = canvasX(wx);
        const double y = canvasY(wy);

        auto target = inputPortAt(x, y);
        if (target && target != mDragSource) {
            if (onConnect)
                onConnect(mDragSource->full_name(), target->full_name());
            // Drawn immediately rather than waiting for JACK's port-registration callback to come
            // back round: the cable appears under the pointer that made it.
            mConnections.push_back(
                std::make_shared<Connection>(mDragSource, target, mDragSource->type));
        }

        mDragging = false;
        mDragSource = nullptr;
        repaint();
        return true;
    }

    if (mMovingBox) {
        mMovingBox = false;
        mMovingBoxPtr = nullptr;
        return true;
    }

    if (mPanning) {
        mPanning = false;
        return true;
    }

    return false;
}

bool GraphPanel::motion(float wx, float wy)
{
    if (mDragging) {
        mDragCurrentX = canvasX(wx);
        mDragCurrentY = canvasY(wy);
        repaint();
        return true;
    }

    if (mMovingBox && mMovingBoxPtr) {
        mMovingBoxPtr->x = canvasX(wx) - mBoxOffsetX;
        mMovingBoxPtr->y = canvasY(wy) - mBoxOffsetY;
        // The ports move with the box, and so do the cables, because a Connection reads its
        // endpoints out of the Nodes rather than caching them.
        positionPorts(mMovingBoxPtr);
        repaint();
        return true;
    }

    if (mPanning) {
        mPanX += wx - mPanStartX;
        mPanY += wy - mPanStartY;
        mPanStartX = wx;
        mPanStartY = wy;
        repaint();
        return true;
    }

    return false;
}

bool GraphPanel::scroll(float wx, float wy, int dir)
{
    if (!mRect.contains(wx, wy))
        return false;
    // NOT ZOOM-TO-CURSOR. on_scroll_event scaled about the canvas origin and this does the same:
    // the port you are pointing at moves under the pointer as you zoom. It is worth changing, but
    // not in the commit that swaps the toolkit -- a behaviour change hidden inside a port is a
    // behaviour change nobody reviewed.
    // dir is X11Window's convention: -1 is button 4, the wheel pushed AWAY from the user, which
    // zooms IN. GraphCanvas read GDK_SCROLL_UP for the same notch.
    setZoom(dir < 0 ? mZoom * geo::kZoomStepWheel : mZoom / geo::kZoomStepWheel);
    return true;
}

} // namespace jackbridge

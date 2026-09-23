// The JACK graph itself: client boxes, ports, connections, and the drags that make and break them.
//
// This is GraphCanvas with Gtk::DrawingArea taken out from under it. The LAYOUT, the hit testing,
// pair_stereo_ports(), build_client_boxes(), fit_to_window() and the saved-position logic are moved
// ACROSS UNCHANGED -- they were always pure geometry over Node and ClientBox, which is the single
// largest piece of luck in this port. What changed is the surface: it draws through gfx::Canvas
// instead of a Cairo::Context, it carries its own rect instead of asking a parent widget for an
// allocation, and it returns whether it consumed an event instead of chaining to a base class.
//
// CAIRO ONLY. No Xlib, no libjack, no gtkmm. The Makefile's rule about src/gfx not linking X11
// covers this file too, and for the same reason: it is what lets the layout be composed and
// audited with no X server.
//
// THE PAN AND ZOOM ARE THE PANEL'S OWN and always were -- m_pan_x/m_pan_y predate the port. The
// Gtk::ScrolledWindow that wrapped this was therefore redundant scrolling on top of scrolling, and
// it is deleted rather than replaced.

#pragma once

#include "ClientBox.hpp"
#include "Connection.hpp"
#include "Node.hpp"
#include "gfx/canvas.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace jackbridge
{

class GraphPanel
{
public:
    // A drag from an output port to an input port completed, or a right-click landed on a
    // connection. Both carry JACK's own full port names, which is what jack_connect() wants.
    std::function<void(const std::string &source, const std::string &dest)> onConnect;
    std::function<void(const std::string &source, const std::string &dest)> onDisconnect;

    // Something moved. The window turns this into one invalidate() per pass round its loop.
    std::function<void()> onNeedsRepaint;

    //--- where it is ----------------------------------------------------
    // The viewport, in window coordinates. Everything below is in CANVAS coordinates, which are
    // related by: window = rect.origin + pan + canvas * zoom.
    void setRect(const Rect &r);
    Rect rect() const
    {
        return mRect;
    }

    //--- content --------------------------------------------------------
    void clear();
    void addNode(std::shared_ptr<Node> node);
    void addConnection(std::shared_ptr<Connection> conn);
    // Drops everything, KEEPING each box's position under its client name, so a refresh does not
    // undo a layout the user arranged by hand.
    void removeAll();

    const std::vector<std::shared_ptr<Node>> &nodes() const
    {
        return mNodes;
    }

    //--- view -----------------------------------------------------------
    void setZoom(double zoom);
    double zoom() const
    {
        return mZoom;
    }
    void layout(bool preservePositions = false);
    void fitToWindow();

    //--- paint and input ------------------------------------------------
    void draw(Canvas &c) const;

    // Each returns true if it consumed the event. Coordinates are in WINDOW space; the panel maps
    // them itself, because the mapping is its own pan and zoom.
    bool press(float x, float y, int button);
    bool release(float x, float y, int button);
    bool motion(float x, float y);
    bool scroll(float x, float y, int dir);

private:
    void buildClientBoxes();
    void drawClientBox(Canvas &c, const ClientBox &box) const;
    void drawPort(Canvas &c, const Node &node, float x, float y, bool isOutput) const;
    void drawConnection(Canvas &c, const Connection &conn) const;
    void drawDragPreview(Canvas &c) const;
    void positionPorts(ClientBox *box) const;

    std::shared_ptr<Node> outputPortAt(double x, double y);
    std::shared_ptr<Node> inputPortAt(double x, double y);
    ClientBox *boxAt(double x, double y);

    // Window space to canvas space, and the inverse of what draw() sets up.
    double canvasX(float windowX) const
    {
        return (windowX - mRect.x - mPanX) / mZoom;
    }
    double canvasY(float windowY) const
    {
        return (windowY - mRect.y - mPanY) / mZoom;
    }
    void repaint() const
    {
        if (onNeedsRepaint)
            onNeedsRepaint();
    }

    Rect mRect;

    std::vector<std::shared_ptr<Node>> mNodes;
    std::vector<std::shared_ptr<Connection>> mConnections;
    std::vector<ClientBox> mClientBoxes;

    struct SavedPosition {
        double x, y;
    };
    // KEYED BY CLIENT NAME, and kept for the life of the window: a client that goes away and comes
    // back -- which is every JACK restart -- returns to where the user put it.
    std::map<std::string, SavedPosition> mSavedPositions;

    double mZoom = 1.0;
    double mPanX = 0.0;
    double mPanY = 0.0;

    bool mDragging = false;
    bool mPanning = false;
    bool mMovingBox = false;
    std::shared_ptr<Node> mDragSource;
    ClientBox *mMovingBoxPtr = nullptr;
    double mDragCurrentX = 0.0;
    double mDragCurrentY = 0.0;
    float mPanStartX = 0.0f;
    float mPanStartY = 0.0f;
    double mBoxOffsetX = 0.0;
    double mBoxOffsetY = 0.0;
};

} // namespace jackbridge

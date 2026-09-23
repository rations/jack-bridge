// xin -- DRIVE THE REAL WINDOWS. The other half of the verification story: tools/uirender proves a
// string fits its slot with no X server, and this proves the window in front of you answers a
// pointer. Between them they cover the two ways a hand-drawn panel fails -- a label that overflows
// silently, and a rectangle that draws in one place and hit-tests in another.
//
// WHY THIS EXISTS AT ALL: xdotool is not installed on the development machine and neither is
// xautomation, so the alternative was clicking by hand, which cannot be pasted into a bug report
// and cannot be repeated exactly. It was written to chase "the box is not moving" in jack-graph and
// is kept because that class of bug -- draw and hit-test disagreeing -- is the one this toolkit is
// most exposed to, every widget carrying its own rect.
//
// IT RAISES THE TARGET BEFORE IT CLICKS, and that is not a convenience. XTest puts events on the
// pointer, not into a window: the server delivers them to whatever is topmost at that root
// position. A click aimed at a window sitting under the editor goes to the editor, the panel under
// test never sees a ButtonPress, and the result reads exactly like a broken hit test. That is a
// full hour lost once already. `raise` is therefore implicit in click, drag and wheel, and is also
// available on its own; pass -R to suppress it when the stacking IS the thing being tested.
//
// COORDINATES ARE WINDOW-RELATIVE AND UNSCALED, which is the only coordinate space worth typing:
// they are the numbers a panel's geometry.h uses, so a rect printed by uirender can be clicked
// directly. XTranslateCoordinates turns them into root coordinates at the last moment. Note the
// unscaled part: X11Window divides by its scale factor before it calls a panel, so against a
// binary started with --scale 2 the window coordinate is twice the logical one.
//
// NOT A TEST RUNNER. It has no idea what should happen; it makes the input happen and leaves the
// judging to you, a screenshot, or the binary's own stderr. Keeping it that way is what keeps it
// under 300 lines and dependency-free.
//
// Usage:
//   xin list                            every top-level window, with its WM_CLASS and geometry
//   xin find <class>                    the window ids whose WM_CLASS res_name or res_class matches
//   xin under                           the window id currently under the pointer
//   xin raise <win>                     raise the window and give it the input focus
//   xin click <win> X Y [button]        press and release, button 1 by default
//   xin drag  <win> X0 Y0 X1 Y1 [steps] press, interpolate, release -- the box-move / pan test
//   xin wheel <win> X Y up|down [count] one notch per count; up is button 4
//   xin key   <win> <keysym-name>...    each name through XStringToKeysym, e.g. Escape Tab a
//   xin type  <win> <text>              one Latin-1 character at a time, for a text field
//   xin resize <win> W H                a ConfigureRequest, to exercise the layout at a new size
//   xin close <win>                     WM_DELETE_WINDOW, the polite close the panels handle
//
// <win> is an id as `list` prints it (0x2000001), or a WM_CLASS that `find` resolves to exactly one
// window -- so `xin drag jack-graph 300 200 500 300` works without looking an id up first.
//
// Build: make xin. It links X11 and Xtst and nothing else, it is never installed, and it is not
// part of `all` -- it is a development tool that needs a running X server to be worth building.

#define _POSIX_C_SOURCE 200809L

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // strcasecmp: WM_CLASS case is the application's business, not the caller's
#include <time.h>

// One pointer step, and one settle after a button transition. A panel that sets a dirty flag on
// press and paints on the next loop pass needs the loop to get a turn; without a pause the whole
// drag can arrive inside one select() wakeup, which is not what a hand does and not what the code
// under test is being asked about.
static const long kStepUs = 25000L;
static const long kSettleUs = 80000L;

static Display *gDpy;

static void nap(long us)
{
    struct timespec ts = {us / 1000000L, (us % 1000000L) * 1000L};
    nanosleep(&ts, NULL);
}

// Every top-level window, meaning every direct child of the root. A reparenting window manager puts
// its frame in between, so the application window is a grandchild -- hence the one level of
// recursion, which is as deep as any of these panels ever sits.
static int forEachTopLevel(void (*fn)(Window, void *), void *ctx)
{
    Window root = DefaultRootWindow(gDpy), parent, *kids = NULL;
    unsigned n = 0;
    if (!XQueryTree(gDpy, root, &root, &parent, &kids, &n))
        return 0;
    for (unsigned i = 0; i < n; ++i) {
        fn(kids[i], ctx);
        Window r2, p2, *g = NULL;
        unsigned m = 0;
        if (XQueryTree(gDpy, kids[i], &r2, &p2, &g, &m)) {
            for (unsigned j = 0; j < m; ++j)
                fn(g[j], ctx);
            if (g)
                XFree(g);
        }
    }
    if (kids)
        XFree(kids);
    return 1;
}

static int classMatches(Window w, const char *want)
{
    XClassHint ch;
    int hit = 0;
    if (!XGetClassHint(gDpy, w, &ch))
        return 0;
    if ((ch.res_name && !strcasecmp(ch.res_name, want)) ||
        (ch.res_class && !strcasecmp(ch.res_class, want)))
        hit = 1;
    if (ch.res_name)
        XFree(ch.res_name);
    if (ch.res_class)
        XFree(ch.res_class);
    return hit;
}

static void printWindow(Window w, void *ctx)
{
    (void)ctx;
    XWindowAttributes a;
    XClassHint ch = {NULL, NULL};
    char *name = NULL;
    if (!XGetWindowAttributes(gDpy, w, &a) || a.map_state != IsViewable)
        return;
    if (!XGetClassHint(gDpy, w, &ch))
        return; // No WM_CLASS: a frame or an override-redirect popup, not something to aim at.
    XFetchName(gDpy, w, &name);
    printf("0x%lx  %-14s %-14s %dx%d+%d+%d  %s\n", w, ch.res_name ? ch.res_name : "-",
           ch.res_class ? ch.res_class : "-", a.width, a.height, a.x, a.y, name ? name : "");
    if (name)
        XFree(name);
    if (ch.res_name)
        XFree(ch.res_name);
    if (ch.res_class)
        XFree(ch.res_class);
}

struct FindCtx {
    const char *want;
    Window found[16];
    int n;
};

static void collectMatch(Window w, void *ctx)
{
    struct FindCtx *f = ctx;
    XWindowAttributes a;
    if (!XGetWindowAttributes(gDpy, w, &a) || a.map_state != IsViewable)
        return;
    if (classMatches(w, f->want) && f->n < (int)(sizeof f->found / sizeof f->found[0]))
        f->found[f->n++] = w;
}

// An id, or a WM_CLASS that resolves to exactly one viewable window. Ambiguity is an error rather
// than a first match: two jack-graphs open and a silently chosen one is a confusing session.
static Window resolve(const char *spec)
{
    if (spec[0] == '0' && (spec[1] == 'x' || spec[1] == 'X'))
        return (Window)strtoul(spec, NULL, 16);

    struct FindCtx f = {spec, {0}, 0};
    forEachTopLevel(collectMatch, &f);
    if (f.n == 1)
        return f.found[0];
    if (f.n == 0)
        fprintf(stderr, "xin: no viewable window with WM_CLASS '%s'\n", spec);
    else
        fprintf(stderr, "xin: '%s' matches %d windows; name one by id (xin find %s)\n", spec, f.n,
                spec);
    exit(1);
}

static void raiseWindow(Window w)
{
    XRaiseWindow(gDpy, w);
    // Best effort, and deliberately not checked: a window that declines the focus (a dialog whose
    // parent holds it) is not a failure, and BadMatch here is normal for an unviewable window.
    XSetInputFocus(gDpy, w, RevertToPointerRoot, CurrentTime);
    XSync(gDpy, False);
    nap(kSettleUs);
}

static void moveTo(Window w, int x, int y)
{
    Window child;
    int rx = 0, ry = 0;
    XTranslateCoordinates(gDpy, w, DefaultRootWindow(gDpy), x, y, &rx, &ry, &child);
    XTestFakeMotionEvent(gDpy, -1, rx, ry, 0);
    XFlush(gDpy);
    nap(kStepUs);
}

static void button(unsigned b, Bool down)
{
    XTestFakeButtonEvent(gDpy, b, down, CurrentTime);
    XFlush(gDpy);
    nap(kSettleUs);
}

static void sendKeysym(KeySym ks)
{
    KeyCode kc = XKeysymToKeycode(gDpy, ks);
    if (kc == 0) {
        fprintf(stderr, "xin: no keycode for keysym 0x%lx on this layout\n", (unsigned long)ks);
        return;
    }
    XTestFakeKeyEvent(gDpy, kc, True, CurrentTime);
    XTestFakeKeyEvent(gDpy, kc, False, CurrentTime);
    XFlush(gDpy);
    nap(kStepUs);
}

static int usage(void)
{
    fputs("usage: xin [-R] <command> [args]\n"
          "  list | find <class> | under | raise <win>\n"
          "  click <win> X Y [button]        drag <win> X0 Y0 X1 Y1 [steps]\n"
          "  wheel <win> X Y up|down [n]     key <win> <keysym>...\n"
          "  type <win> <text>               resize <win> W H       close <win>\n",
          stderr);
    return 2;
}

int main(int argc, char **argv)
{
    int autoRaise = 1;
    if (argc > 1 && !strcmp(argv[1], "-R")) {
        autoRaise = 0;
        --argc;
        ++argv;
    }
    if (argc < 2)
        return usage();

    gDpy = XOpenDisplay(NULL);
    if (!gDpy) {
        fprintf(stderr, "xin: cannot open display %s\n", getenv("DISPLAY") ? getenv("DISPLAY") : "");
        return 1;
    }

    int major = 0, minor = 0, ev = 0, err = 0;
    if (!XTestQueryExtension(gDpy, &ev, &err, &major, &minor)) {
        fprintf(stderr, "xin: this X server has no XTEST extension; synthetic input is impossible\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (!strcmp(cmd, "list")) {
        forEachTopLevel(printWindow, NULL);
    } else if (!strcmp(cmd, "find") && argc >= 3) {
        struct FindCtx f = {argv[2], {0}, 0};
        forEachTopLevel(collectMatch, &f);
        for (int i = 0; i < f.n; ++i)
            printf("0x%lx\n", f.found[i]);
        if (f.n == 0)
            return 1;
    } else if (!strcmp(cmd, "under")) {
        Window root = DefaultRootWindow(gDpy), child = None;
        int rx, ry, wx, wy;
        unsigned mask;
        if (XQueryPointer(gDpy, root, &root, &child, &rx, &ry, &wx, &wy, &mask))
            printf("0x%lx at root (%d,%d)\n", child, rx, ry);
    } else if (!strcmp(cmd, "raise") && argc >= 3) {
        raiseWindow(resolve(argv[2]));
    } else if (!strcmp(cmd, "click") && argc >= 5) {
        Window w = resolve(argv[2]);
        unsigned b = argc >= 6 ? (unsigned)atoi(argv[5]) : 1u;
        if (autoRaise)
            raiseWindow(w);
        moveTo(w, atoi(argv[3]), atoi(argv[4]));
        button(b, True);
        button(b, False);
    } else if (!strcmp(cmd, "drag") && argc >= 7) {
        Window w = resolve(argv[2]);
        const int x0 = atoi(argv[3]), y0 = atoi(argv[4]);
        const int x1 = atoi(argv[5]), y1 = atoi(argv[6]);
        int steps = argc >= 8 ? atoi(argv[7]) : 12;
        if (steps < 1)
            steps = 1;
        if (autoRaise)
            raiseWindow(w);
        // The press happens at the start point AFTER the pointer is already there, because a panel
        // that reads the press coordinate out of the event and a panel that reads it out of the
        // last motion must both see the same point.
        moveTo(w, x0, y0);
        button(1, True);
        for (int i = 1; i <= steps; ++i)
            moveTo(w, x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        button(1, False);
    } else if (!strcmp(cmd, "wheel") && argc >= 6) {
        Window w = resolve(argv[2]);
        // Button 4 is the wheel pushed away from the user. X11Window turns that into dir = -1, and
        // jack-graph zooms in on it -- so `up` here is `zoom in` there.
        const unsigned b = strcmp(argv[5], "up") == 0 ? 4u : 5u;
        const int n = argc >= 7 ? atoi(argv[6]) : 1;
        if (autoRaise)
            raiseWindow(w);
        moveTo(w, atoi(argv[3]), atoi(argv[4]));
        for (int i = 0; i < n; ++i) {
            XTestFakeButtonEvent(gDpy, b, True, CurrentTime);
            XTestFakeButtonEvent(gDpy, b, False, CurrentTime);
            XFlush(gDpy);
            nap(kStepUs);
        }
    } else if (!strcmp(cmd, "key") && argc >= 4) {
        Window w = resolve(argv[2]);
        if (autoRaise)
            raiseWindow(w);
        for (int i = 3; i < argc; ++i) {
            KeySym ks = XStringToKeysym(argv[i]);
            if (ks == NoSymbol)
                fprintf(stderr, "xin: '%s' is not a keysym name\n", argv[i]);
            else
                sendKeysym(ks);
        }
    } else if (!strcmp(cmd, "type") && argc >= 4) {
        Window w = resolve(argv[2]);
        if (autoRaise)
            raiseWindow(w);
        // Latin-1 only, on purpose: a keysym for a character is one thing, a KEYCODE for it is
        // whatever the user's layout happens to carry, and remapping the keyboard to type one
        // character is more damage than a test tool should be able to do. The XIM path in the
        // recorder's filename field is verified by typing an accented character BY HAND with a
        // real dead key, which is the only honest test of it anyway.
        for (const unsigned char *p = (const unsigned char *)argv[3]; *p; ++p)
            sendKeysym((KeySym)*p);
    } else if (!strcmp(cmd, "resize") && argc >= 5) {
        Window w = resolve(argv[2]);
        // A ConfigureRequest, not a ConfigureNotify: the window manager owns the geometry, and
        // asking it is what a user dragging the frame does. A panel whose size hints pin an axis
        // will have that axis clamped here, which is the behaviour being checked.
        XResizeWindow(gDpy, w, (unsigned)atoi(argv[3]), (unsigned)atoi(argv[4]));
        XSync(gDpy, False);
    } else if (!strcmp(cmd, "close") && argc >= 3) {
        Window w = resolve(argv[2]);
        // WM_DELETE_WINDOW, the close every panel here handles. XDestroyWindow would pull the
        // window out from under the loop and prove nothing about the shutdown path.
        Atom proto = XInternAtom(gDpy, "WM_PROTOCOLS", False);
        Atom del = XInternAtom(gDpy, "WM_DELETE_WINDOW", False);
        XEvent e;
        memset(&e, 0, sizeof e);
        e.xclient.type = ClientMessage;
        e.xclient.window = w;
        e.xclient.message_type = proto;
        e.xclient.format = 32;
        e.xclient.data.l[0] = (long)del;
        e.xclient.data.l[1] = CurrentTime;
        XSendEvent(gDpy, w, False, NoEventMask, &e);
        XSync(gDpy, False);
    } else {
        XCloseDisplay(gDpy);
        return usage();
    }

    XSync(gDpy, False);
    XCloseDisplay(gDpy);
    return 0;
}

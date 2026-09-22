// The Bluetooth device list, as data.
//
// This is what replaces the GtkListStore, and the replacement deletes three functions from
// gui_bt.c outright. It is worth being precise about why, because all three were correct code
// working around a container that could only hold strings:
//
//   * A GtkListStore row was two strings: a label and an object path. The device's STATE therefore
//     had to be encoded into the label -- gui_bt.c:1248-1250 appended " [Paired]", " [Trusted]" and
//     " [Connected]" on every update, and strip_state_markers() (gui_bt.c:51) took them off again
//     before appending the new set, by memmove'ing over every occurrence of each token.
//
//     THAT ROUND TRIP IS LOSSY. A device that names itself "My [Paired] Speaker" -- and a Bluetooth
//     device chooses its own name -- has part of its name eaten the first time its state changes.
//     Here `name` is the name, `paired`/`trusted`/`connected` are booleans, and the two never meet:
//     the panel draws the state as chips beside the name (gfx/widgets.h drawChip).
//
//   * The "favourite" star was a "★ " prefix on the same label, detected with
//     g_str_has_prefix and preserved by hand through every update (gui_bt.c:1241-1243). It is a
//     bool here.
//
//   * safe_utf8() (gui_bt.c:31) called g_utf8_validate and g_utf8_make_valid. Those are gone with
//     GLib, so sanitizeUtf8() below does the job directly -- and it MATTERS more than it looks.
//     A BlueZ device name is chosen by whatever is in radio range, arrives over D-Bus as bytes, and
//     goes straight into FreeType. Cairo's toy text API takes a NUL-terminated UTF-8 string and
//     invalid input is not something to find out about at the glyph cache.

#pragma once

#include <string>
#include <vector>

namespace jackbridge
{

struct BtDevice {
    // The BlueZ object path, "/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF". THE IDENTITY of the row: the
    // list keeps its selection by this, and the MAC to route to is derived from it. Never shown.
    std::string path;

    // Alias or Name, already through sanitizeUtf8(). Falls back to the MAC when BlueZ gives neither,
    // which it does for a device that has been seen but not yet queried.
    std::string name;

    bool paired = false;
    bool trusted = false;
    bool connected = false;

    // The device saved in devices.conf as the Bluetooth output. Drawn as a star, which is what the
    // "★ " label prefix meant.
    bool favourite = false;
};

class BtModel
{
public:
    // Adds the device, or updates it in place if its path is already known. IN PLACE MATTERS: it is
    // what keeps a row's identity stable across a mid-scan refresh, which is what keeps the
    // ListView's selection from moving under somebody reaching for Pair. gui_bt_add_device() looked
    // each device up by object_path for the same reason.
    void upsert(const BtDevice &d);

    // Removes by path. Returns true if anything was removed.
    bool remove(const std::string &path);

    void clear();

    // Merges whatever subset of the properties is known. BlueZ's PropertiesChanged carries only the
    // properties that changed, so a signal saying Connected=true must not blank the name.
    void setName(const std::string &path, const std::string &name);
    void setState(const std::string &path, bool paired, bool trusted, bool connected);
    void setFavourite(const std::string &mac);

    const std::vector<BtDevice> &devices() const
    {
        return mDevices;
    }

    const BtDevice *find(const std::string &path) const;

    // Sorted for display: connected first, then paired, then the rest, each group by name. A
    // discovery scan appends in the order the radio answers, which is arbitrary and changes between
    // scans -- so without an order the row you were about to click moves.
    std::vector<BtDevice> sorted() const;

    // Replaces g_utf8_validate + g_utf8_make_valid. Returns valid UTF-8, with every invalid byte or
    // truncated sequence replaced by U+FFFD. Also strips C0 control characters, which are valid
    // UTF-8 and have no business in a device name: a name containing a newline would otherwise draw
    // over the row below it.
    static std::string sanitizeUtf8(const std::string &s);

private:
    BtDevice *mutableFind(const std::string &path);

    std::vector<BtDevice> mDevices;
};

} // namespace jackbridge

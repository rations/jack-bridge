// See btmodel.h.

#include "btmodel.h"

#include "devices.h"

#include <algorithm>

namespace jackbridge
{

namespace
{

// The replacement character, U+FFFD, as UTF-8.
const char *const kReplacement = "\xEF\xBF\xBD";

// How many continuation bytes follow this lead byte, or -1 if it is not a legal lead byte.
//
// Written out rather than leaning on a library, because the whole point is to accept exactly what is
// valid and nothing else. The bounds are RFC 3629: four bytes maximum, no encoding above U+10FFFF,
// and the surrogate range D800..DFFF excluded -- a UTF-8 encoder that emits a surrogate is producing
// CESU-8, which cairo will not draw.
int trailingBytes(unsigned char c)
{
    if (c < 0x80)
        return 0;
    if (c >= 0xC2 && c <= 0xDF)
        return 1;
    if (c >= 0xE0 && c <= 0xEF)
        return 2;
    if (c >= 0xF0 && c <= 0xF4)
        return 3;
    // 0x80..0xBF is a continuation byte with no lead; 0xC0/0xC1 would be an overlong two-byte
    // encoding of an ASCII character; 0xF5..0xFF is above U+10FFFF.
    return -1;
}

} // namespace

//------------------------------------------------------------------------
std::string BtModel::sanitizeUtf8(const std::string &s)
{
    std::string out;
    out.reserve(s.size());

    size_t i = 0;
    while (i < s.size()) {
        const unsigned char lead = static_cast<unsigned char>(s[i]);
        const int extra = trailingBytes(lead);

        if (extra < 0) {
            out += kReplacement;
            ++i;
            continue;
        }

        if (extra == 0) {
            // ASCII. C0 controls and DEL are dropped: they are valid UTF-8 and a device name
            // containing a newline or a backspace would draw over its neighbours.
            if (lead >= 0x20 && lead != 0x7F)
                out += static_cast<char>(lead);
            ++i;
            continue;
        }

        if (i + static_cast<size_t>(extra) >= s.size()) {
            out += kReplacement; // truncated sequence at the end of the string
            ++i;
            continue;
        }

        // Every continuation byte must be 10xxxxxx, and the second byte is further restricted for
        // the three-and four-byte forms -- without that, an overlong encoding of an ASCII character,
        // or a surrogate, passes as valid.
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            const unsigned char c = static_cast<unsigned char>(s[i + static_cast<size_t>(k)]);
            if (c < 0x80 || c > 0xBF) {
                ok = false;
                break;
            }
        }
        if (ok) {
            const unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
            if (lead == 0xE0 && b1 < 0xA0)
                ok = false; // overlong three-byte
            else if (lead == 0xED && b1 > 0x9F)
                ok = false; // surrogate D800..DFFF
            else if (lead == 0xF0 && b1 < 0x90)
                ok = false; // overlong four-byte
            else if (lead == 0xF4 && b1 > 0x8F)
                ok = false; // above U+10FFFF
        }

        if (!ok) {
            out += kReplacement;
            ++i;
            continue;
        }

        out.append(s, i, static_cast<size_t>(extra) + 1);
        i += static_cast<size_t>(extra) + 1;
    }

    return out;
}

//------------------------------------------------------------------------
BtDevice *BtModel::mutableFind(const std::string &path)
{
    for (BtDevice &d : mDevices) {
        if (d.path == path)
            return &d;
    }
    return nullptr;
}

const BtDevice *BtModel::find(const std::string &path) const
{
    for (const BtDevice &d : mDevices) {
        if (d.path == path)
            return &d;
    }
    return nullptr;
}

//------------------------------------------------------------------------
void BtModel::upsert(const BtDevice &d)
{
    if (d.path.empty())
        return;

    BtDevice clean = d;
    clean.name = sanitizeUtf8(clean.name);
    if (clean.name.empty())
        clean.name = Devices::macFromBluezObject(clean.path);

    if (BtDevice *existing = mutableFind(d.path)) {
        // In place, so the row keeps its position and the list keeps its selection.
        *existing = clean;
        return;
    }
    mDevices.push_back(std::move(clean));
}

bool BtModel::remove(const std::string &path)
{
    for (size_t i = 0; i < mDevices.size(); ++i) {
        if (mDevices[i].path == path) {
            mDevices.erase(mDevices.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

void BtModel::clear()
{
    mDevices.clear();
}

//------------------------------------------------------------------------
void BtModel::setName(const std::string &path, const std::string &name)
{
    BtDevice *d = mutableFind(path);
    if (!d)
        return;
    const std::string clean = sanitizeUtf8(name);
    // An empty name does not blank an existing one: PropertiesChanged carries only what changed, and
    // a device that drops its Alias still has whatever Name it had.
    if (!clean.empty())
        d->name = clean;
}

void BtModel::setState(const std::string &path, bool paired, bool trusted, bool connected)
{
    BtDevice *d = mutableFind(path);
    if (!d)
        return;
    d->paired = paired;
    d->trusted = trusted;
    d->connected = connected;
}

void BtModel::setFavourite(const std::string &mac)
{
    // Matched on the MAC derived from each path, because devices.conf stores a MAC and BlueZ
    // identifies a device by an object path. An empty MAC clears every star.
    for (BtDevice &d : mDevices)
        d.favourite = !mac.empty() && Devices::macFromBluezObject(d.path) == mac;
}

//------------------------------------------------------------------------
std::vector<BtDevice> BtModel::sorted() const
{
    std::vector<BtDevice> out = mDevices;
    std::stable_sort(out.begin(), out.end(), [](const BtDevice &a, const BtDevice &b) {
        // Connected, then paired, then the rest; within a group, by name. A scan appends in whatever
        // order the radio answers, which differs between scans -- so without an order the row
        // somebody is reaching for moves while they reach for it.
        const int ra = a.connected ? 0 : (a.paired ? 1 : 2);
        const int rb = b.connected ? 0 : (b.paired ? 1 : 2);
        if (ra != rb)
            return ra < rb;
        if (a.name != b.name)
            return a.name < b.name;
        return a.path < b.path;
    });
    return out;
}

} // namespace jackbridge

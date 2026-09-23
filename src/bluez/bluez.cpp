// A PLACEHOLDER. The real libdbus-1 implementation of this interface is Phase 3 of the GTK removal
// and lands next; this file is what makes the other four mxeq pages buildable and testable before
// any D-Bus code exists.
//
// THAT SEQUENCING IS DELIBERATE, and it is the plan's reasoning carried over. Porting the BlueZ
// client from GDBus to libdbus-1 is the largest and highest-risk single piece of this work, and if
// it landed in the same commit as the toolkit swap then a failed pairing could be either one. With
// this placeholder, the tab bar, the mixer's four checkbox roles, the device switching, the recorder
// and the Steam bridge are all verified working on the new toolkit FIRST, so when the D-Bus code
// arrives a Bluetooth failure can only be the D-Bus code.
//
// IT IS ALSO A REAL CODE PATH, not dead weight. Everything here is exactly what mxeq must do when
// bluetoothd is not running -- which is the second half of the plan's on-machine check 9: "with
// bluez stopped, mxeq starts and stays up with the Bluetooth page inert". open() returning false
// has to be survivable, the page has to explain itself rather than look empty, and no action may
// crash. Running against this placeholder tests that.

#include "bluez.h"

#include <cstdio>

namespace jackbridge
{

struct Bluez::Impl {
    bool open = false;
};

Bluez::Bluez() : mImpl(new Impl)
{
}

Bluez::~Bluez()
{
    delete mImpl;
}

bool Bluez::open()
{
    fprintf(stderr, "jack-bridge: the BlueZ client is not built yet; "
                    "the Bluetooth page will be inert\n");
    return false;
}

void Bluez::close()
{
}

bool Bluez::isOpen() const
{
    return false;
}

bool Bluez::adapterReady() const
{
    return false;
}

std::vector<int> Bluez::watchDescriptors() const
{
    return {};
}

void Bluez::handleWatches()
{
}

void Bluez::handleTimeouts()
{
}

bool Bluez::powered() const
{
    return false;
}

bool Bluez::discoverable() const
{
    return false;
}

bool Bluez::discovering() const
{
    return false;
}

void Bluez::setPowered(bool)
{
}

void Bluez::setDiscoverable(bool)
{
}

void Bluez::startDiscovery()
{
}

void Bluez::stopDiscovery()
{
}

std::vector<BluezDeviceProps> Bluez::knownDevices()
{
    return {};
}

void Bluez::pair(const std::string &)
{
}

void Bluez::setTrusted(const std::string &, bool)
{
}

void Bluez::connect(const std::string &)
{
}

void Bluez::removeDevice(const std::string &)
{
}

bool Bluez::deviceState(const std::string &, bool *paired, bool *trusted, bool *connected)
{
    if (paired)
        *paired = false;
    if (trusted)
        *trusted = false;
    if (connected)
        *connected = false;
    return false;
}

//------------------------------------------------------------------------
// The one piece of real behaviour here, because it is not D-Bus: the hint block
// compose_hint_message() appended to every failure (gui_bt.c:603). Kept because these three lines
// were written from support questions rather than invented, and they are what a user who cannot pair
// actually needs to check.
std::string Bluez::hintMessage(const std::string &prefix, const std::string &detail)
{
    std::string m = prefix;
    if (!prefix.empty() && !detail.empty())
        m += ": ";
    m += detail;
    m += "  Check: your user is in the 'audio' group (and 'bluetooth' if it exists); "
         "/etc/polkit-1/rules.d/90-jack-bridge-bluetooth.rules exists; "
         "the adapter is powered and the device is in range.";
    return m;
}

} // namespace jackbridge

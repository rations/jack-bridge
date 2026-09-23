// Everything mxeq is, minus the window.
//
// NO X11 HEADER HERE, and none in app.cpp. It owns the mixer, the output device, the recorder, the
// Steam bridge, the Bluetooth client and the panel, and it turns the panel's semantic events into
// model calls and the models' changes back into panel content.
//
// IT OWNS NO TIMER AND NO DESCRIPTOR. main.cpp registers the intervals and the file descriptors on
// the window and calls the tick methods, which is the same division Audio-Gui's app.h has and for
// the same reason: it is what lets the whole of this file be driven by tools/uirender with no event
// loop at all.

#pragma once

#include "bluez/bluez.h"
#include "mxeq/alsamixer.h"
#include "mxeq/btmodel.h"
#include "mxeq/devices.h"
#include "mxeq/panel.h"
#include "mxeq/recorder.h"
#include "mxeq/steam.h"

#include <functional>
#include <string>
#include <vector>

namespace jackbridge
{

class App
{
public:
    App();

    // Wire everything up and read the initial state. Returns false only if something the program
    // cannot run without failed; a missing sound card or a stopped bluetoothd are NOT that.
    bool start();

    Panel &panel()
    {
        return mPanel;
    }
    AlsaMixer &mixer()
    {
        return mMixer;
    }
    Devices &devices()
    {
        return mDevices;
    }
    Bluez &bluez()
    {
        return mBluez;
    }

    //--- what main.cpp drives -------------------------------------------
    // The ALSA mixer's poll descriptors fired: somebody moved a control outside this window.
    void onMixerReadable();
    // Called when the descriptor set changed, because snd_mixer_close() invalidated the old ones.
    // main.cpp re-registers from mixer().pollDescriptors().
    std::function<void()> onMixerDescriptorsChanged;

    // Every two seconds: notice an output change made by another process.
    void tickDevices();
    // Every second, while recording: the duration label.
    void tickRecorder();

    // The window should resize itself to panel().height(), or repaint. main.cpp owns both.
    std::function<void(float height)> onHeightChanged;
    std::function<void()> onNeedsRepaint;

private:
    void rebuildMixerPage();
    void rebuildDevicesPage();
    void rebuildBluetoothPage();
    void refreshSteamPage();
    void refreshRecorderPage();
    void message(const std::string &text, bool isError);
    void requestHeight();

    AlsaMixer mMixer;
    Devices mDevices;
    Recorder mRecorder;
    Steam mSteam;
    BtModel mBtModel;
    Bluez mBluez;
    Panel mPanel;

    // The panel's Strip/SwitchCell/EnumCell carry an index into this, so a click can name the ALSA
    // element it came from without the panel ever seeing a snd_mixer_elem_t.
    //
    // REBUILT WHENEVER THE MIXER IS. snd_mixer_close() invalidates every element pointer, so a
    // stale index here is a use-after-free -- which is why the panel is rebuilt in the same call
    // that reopens the card and never separately.
    std::vector<AlsaMixer::Element> mElements;

    // What the Bluetooth page has selected, kept here rather than read back out of the list, because
    // the routing flow needs it after the list may have been rebuilt underneath it.
    std::string mBtSelected;
};

} // namespace jackbridge

#include "JackServerControl.hpp"
#include <jack/jack.h>
#include <alsa/asoundlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

JackServerControl::JackServerControl() {
}

JackServerControl::~JackServerControl() {
}

bool JackServerControl::is_running() const {
    jack_client_t *test = jack_client_open("status_check", JackNoStartServer, NULL);
    if (test) {
        jack_client_close(test);
        return true;
    }
    return false;
}

std::string JackServerControl::get_status() const {
    if (is_running()) {
        return "Running";
    }
    return "Stopped";
}

bool JackServerControl::start(const JackSettings& settings) {
    if (is_running()) {
        fprintf(stderr, "jack-graph: JACK is already running.\n");
        return true;
    }

    fprintf(stderr, "jack-graph: Starting jackd-rt service with new settings via pkexec...\n");
    
    // Use pkexec to run the helper script with root privileges
    // The helper updates /etc/default/jackd-rt and starts the service
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "pkexec /usr/local/lib/jack-bridge/jack-bridge-service-helper start "
        "\"%s\" %d %d %d \"%s\" 2>&1",
        settings.interface.c_str(),
        settings.sample_rate,
        settings.frames_per_period,
        settings.periods_per_buffer,
        settings.midi_driver.c_str());
    
    fprintf(stderr, "jack-graph: Running: %s\n", cmd);
    
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "jack-graph: Failed to start jackd-rt service (exit code %d)\n", ret);
        return false;
    }
    
    usleep(2000000);
    return is_running();
}

bool JackServerControl::stop() {
    fprintf(stderr, "jack-graph: Stopping jackd-rt service via pkexec...\n");
    
    // Use pkexec to run the helper script with root privileges
    int ret = system("pkexec /usr/local/lib/jack-bridge/jack-bridge-service-helper stop 2>&1");
    if (ret != 0) {
        fprintf(stderr, "jack-graph: Failed to stop jackd-rt service (exit code %d)\n", ret);
        return false;
    }
    
    return true;
}

/* Enumerate playback-capable PCM devices through the ALSA control API.
 * Returns lines of the form "hw:CARD=id,DEV=n|Card Name - Device Name".
 *
 * The id half is deliberately CARD=/DEV= and never hw:0,0. It is persisted to
 * /etc/default/jackd-rt as JACKD_DEVICE and read back on the next boot, but
 * card *numbers* move: plugging in a USB interface can make it card 0 and push
 * the internal card to 1. A numeric id would then silently point jackd at the
 * wrong card. Card ids are stable, which is why detect-alsa-device.sh also
 * emits the hw:CARD= form.
 *
 * This replaces a popen("aplay -l | sed ...") that produced bare hw:X,Y with no
 * device names, so the dropdown showed the user four indistinguishable numbers.
 */
std::string JackServerControl::list_audio_devices() const {
    std::string result;

    int card = -1;
    while (snd_card_next(&card) == 0 && card >= 0) {
        char ctl_name[16];
        snprintf(ctl_name, sizeof(ctl_name), "hw:%d", card);

        snd_ctl_t* ctl = nullptr;
        if (snd_ctl_open(&ctl, ctl_name, 0) < 0)
            continue;

        snd_ctl_card_info_t* card_info;
        snd_ctl_card_info_alloca(&card_info);

        std::string card_id, card_name;
        if (snd_ctl_card_info(ctl, card_info) == 0) {
            const char* i = snd_ctl_card_info_get_id(card_info);
            const char* n = snd_ctl_card_info_get_name(card_info);
            if (i) card_id = i;
            if (n) card_name = n;
        }
        /* No id means no stable name to persist; skip rather than fall back to
         * a card number that will not survive a replug. */
        if (card_id.empty()) {
            snd_ctl_close(ctl);
            continue;
        }

        int dev = -1;
        while (snd_ctl_pcm_next_device(ctl, &dev) == 0 && dev >= 0) {
            snd_pcm_info_t* pcm_info;
            snd_pcm_info_alloca(&pcm_info);
            snd_pcm_info_set_device(pcm_info, (unsigned int)dev);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

            if (snd_ctl_pcm_info(ctl, pcm_info) < 0)
                continue;  // no playback on this device

            /* The id, not the name. On an HDA/SOF card snd_pcm_info_get_name()
             * is empty for every PCM, so all six devices would render as the
             * bare card name and the dropdown would be no more use than the
             * hw:0,0 list it replaces. The id carries "HDA Analog", "HDMI1",
             * "HDMI2"... -- it is what `aplay -l` prints for the same reason. */
            const char* dev_id_raw = snd_pcm_info_get_id(pcm_info);
            const char* dev_name_raw = snd_pcm_info_get_name(pcm_info);
            std::string dev_name = dev_id_raw && *dev_id_raw ? dev_id_raw
                                 : (dev_name_raw ? dev_name_raw : "");

            /* DEV=0 is left off on purpose. Everything else in this project
             * writes a card as the bare "hw:CARD=<id>" -- detect-alsa-device.sh
             * emits it, jack-route-select's ensure_server_device passes it to
             * set-device, and it is therefore what lands in JACKD_DEVICE. An id
             * of "hw:CARD=x,DEV=0" here would never compare equal to that, and
             * the Interface field would sit blank on a correctly configured
             * machine. Devices past 0 (the HDMI PCMs) still need the suffix. */
            std::string id = "hw:CARD=" + card_id;
            if (dev != 0) id += ",DEV=" + std::to_string(dev);

            std::string display = card_name.empty() ? card_id : card_name;
            if (!dev_name.empty() && dev_name != card_name) {
                display += " - ";
                display += dev_name;
            }

            result += id + "|" + display + "\n";
        }

        snd_ctl_close(ctl);
    }

    return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <jack/jack.h>

jack_client_t *client = NULL;
jack_port_t *input_left = NULL;
jack_port_t *input_right = NULL;
jack_port_t *output_left = NULL;
jack_port_t *output_right = NULL;

int process(jack_nframes_t nframes, void *arg) {
    float *in_l = jack_port_get_buffer(input_left, nframes);
    float *in_r = jack_port_get_buffer(input_right, nframes);
    float *out_l = jack_port_get_buffer(output_left, nframes);
    float *out_r = jack_port_get_buffer(output_right, nframes);

    if (in_l && in_r && out_l && out_r) {
        memcpy(out_l, in_l, sizeof(float) * nframes);
        memcpy(out_r, in_r, sizeof(float) * nframes);
    }

    return 0;
}

void signal_handler(int sig) {
    if (client) {
        jack_client_close(client);
    }
    exit(0);
}

int main() {
    jack_status_t status;

    client = jack_client_open("alsa_to_jack", JackNullOption, &status);
    if (!client) {
        fprintf(stderr, "Failed to open JACK client\n");
        return 1;
    }

    jack_set_process_callback(client, process, NULL);

    input_left = jack_port_register(client, "input_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    input_right = jack_port_register(client, "input_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    output_left = jack_port_register(client, "output_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    output_right = jack_port_register(client, "output_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "Failed to activate JACK client\n");
        jack_client_close(client);
        return 1;
    }

    signal(SIGINT, signal_handler);

    printf("JACK bridge running\n");

    while (1) {
        sleep(1);
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <jack/jack.h>
#include <pthread.h>

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024  // Number of frames per process call

// Global variables
jack_client_t *jack_client = NULL;
jack_port_t *jack_port_left = NULL, *jack_port_right = NULL;
snd_pcm_t *alsa_handle = NULL;
float *alsa_buffer_left = NULL, *alsa_buffer_right = NULL;
float *interleaved_buffer = NULL;  // Add this global buffer


// JACK process callback
int process(jack_nframes_t nframes, void *arg) {
    // Get JACK output ports
    float *out_left = jack_port_get_buffer(jack_port_left, nframes);
    float *out_right = jack_port_get_buffer(jack_port_right, nframes);

    // Try to read audio data from ALSA loopback capture (stereo)
    int frames_read = snd_pcm_readi(alsa_handle, interleaved_buffer, nframes);
    
    // Handle ALSA errors and non-blocking cases
    if (frames_read == -EAGAIN) {
        // No data available yet (non-blocking), output silence
        memset(out_left, 0, nframes * sizeof(float));
        memset(out_right, 0, nframes * sizeof(float));
        return 0;
    } else if (frames_read == -EPIPE) {
        // Underrun, recover and output silence
        snd_pcm_prepare(alsa_handle);
        memset(out_left, 0, nframes * sizeof(float));
        memset(out_right, 0, nframes * sizeof(float));
        return 0;
    } else if (frames_read < 0) {
        // Other error
        memset(out_left, 0, nframes * sizeof(float));
        memset(out_right, 0, nframes * sizeof(float));
        return 0;
    }

    // De-interleave stereo audio from ALSA to JACK
    int i;
    for (i = 0; i < frames_read; i++) {
        out_left[i] = interleaved_buffer[i * 2];
        out_right[i] = interleaved_buffer[i * 2 + 1];
    }
    
    // Zero out remaining frames if not all were read
    for (; i < nframes; i++) {
        out_left[i] = 0.0f;
        out_right[i] = 0.0f;
    }
    
    return 0;
}

// Initialize ALSA loopback device
int init_alsa() {
    int err;
    snd_pcm_hw_params_t *hw_params;
        
    // Allocate buffers
    alsa_buffer_left = (float *) malloc(sizeof(float) * BUFFER_SIZE);
    alsa_buffer_right = (float *) malloc(sizeof(float) * BUFFER_SIZE);
    interleaved_buffer = (float *) malloc(sizeof(float) * BUFFER_SIZE * 2);

    // Open the ALSA loopback device for capture (device 1, subdevice 0) in non-blocking mode
    err = snd_pcm_open(&alsa_handle, "hw:Loopback,1,0", SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (err < 0) {
        fprintf(stderr, "Error opening ALSA device: %s\n", snd_strerror(err));
        return -1;
    }

    // Allocate hardware parameters structure
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(alsa_handle, hw_params);

    // Set parameters
    snd_pcm_hw_params_set_access(alsa_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(alsa_handle, hw_params, SND_PCM_FORMAT_FLOAT);
    snd_pcm_hw_params_set_channels(alsa_handle, hw_params, 2);
    snd_pcm_hw_params_set_rate(alsa_handle, hw_params, SAMPLE_RATE, 0);
    
    // Write the parameters to the device
    err = snd_pcm_hw_params(alsa_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Error setting ALSA hardware parameters: %s\n", snd_strerror(err));
        return -1;
    }

    // Prepare the PCM device
    err = snd_pcm_prepare(alsa_handle);
    if (err < 0) {
        fprintf(stderr, "Error preparing ALSA device: %s\n", snd_strerror(err));
        return -1;
    }

    return 0;
}

// Initialize JACK client
int init_jack() {
    const char *client_name = "alsa_to_jack";
    jack_options_t options = JackNoStartServer;
    jack_status_t status;

    // Open JACK client
    jack_client = jack_client_open(client_name, options, &status);
    if (!jack_client) {
        fprintf(stderr, "Failed to open JACK client\n");
        return -1;
    }

    // Register two output ports (left and right channels)
    jack_port_left = jack_port_register(jack_client, "left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    jack_port_right = jack_port_register(jack_client, "right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!jack_port_left || !jack_port_right) {
        fprintf(stderr, "Failed to register JACK ports\n");
        return -1;
    }

    // Set the process callback
    if (jack_set_process_callback(jack_client, process, NULL)) {
        fprintf(stderr, "Failed to set JACK process callback\n");
        return -1;
    }

    // Activate the JACK client
    if (jack_activate(jack_client)) {
        fprintf(stderr, "Failed to activate JACK client\n");
        return -1;
    }

    return 0;
}

// Cleanup function to close ALSA and JACK
void cleanup() {
    if (alsa_handle) {
        snd_pcm_close(alsa_handle);
    }
    if (jack_client) {
        jack_client_close(jack_client);
    }
    if (alsa_buffer_left) {
        free(alsa_buffer_left);
    }
    if (alsa_buffer_right) {
        free(alsa_buffer_right);
    }
     if (interleaved_buffer) {
        free(interleaved_buffer);
    }
}

int main() {
    // Initialize ALSA
    if (init_alsa() < 0) {
        cleanup();
        return 1;
    }

    // Initialize JACK
    if (init_jack() < 0) {
        cleanup();
        return 1;
    }

    printf("JACK client running...\n");

    // Run the JACK processing loop
    while (1) {
        sleep(1); // Keep the process alive
    }

    // Cleanup before exiting
    cleanup();
    return 0;
}
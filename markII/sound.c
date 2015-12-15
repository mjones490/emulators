#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include "sound.h"
#include "logging.h"
#include "bus.h"
#include "cpu_iface.h"
#include "config.h"

int min_buf_size;
int max_buf_size;
BYTE *sound_buf;
WORD read_ndx = 0;
WORD write_ndx = 0;
bool speaker_state = 0;
BYTE clocks;
BYTE clocks_per_sample;

void sound_hook(void *udata, Uint8 *stream, int len)
{
    int i;
    WORD last_ndx;
    BYTE quiet = sound_buf[read_ndx];

    for (i = 0; i < len; ++i) {
        stream[i] = sound_buf[read_ndx];
        sound_buf[read_ndx] = quiet;
        last_ndx = read_ndx;
        read_ndx = (read_ndx + 1) % max_buf_size;
        if (read_ndx == write_ndx)
            read_ndx = last_ndx;
    }
}

BYTE speaker_toggle(BYTE port, bool read, BYTE value, void *data)
{
    speaker_state = !speaker_state;
    return value;
}

void speaker_clock(BYTE cpu_clocks)
{
    clocks += cpu_clocks;
    while (clocks >= clocks_per_sample) {
    SDL_LockAudio();
        sound_buf[write_ndx] = speaker_state? 255 : 127;
        write_ndx = (write_ndx + 1) % max_buf_size;
    SDL_UnlockAudio();
        clocks -= clocks_per_sample;
    }
}

void init_sound()
{
    int freq;
    int cpu_max_speed;

    LOG_INF("Initializing sound.\n");

    min_buf_size = get_config_int("SOUND", "MIN_BUF_SIZE");
    if (min_buf_size == 0) {
        LOG_ERR("Minimume buffer size must be greater than %d.\n",
            min_buf_size);
        return;
    }

    max_buf_size = get_config_int("SOUND", "MAX_BUF_SIZE");
    if (max_buf_size < min_buf_size) {
        LOG_ERR("Maximum buffer size must be greater than or equal \
            to minimum buffer size.\n");
        return;
    }

    freq = get_config_int("SOUND", "FREQUENCY");
    if (freq < 1024) {
        LOG_ERR("Frequency must be greater than 1023.\n");
        return;
    }

    cpu_max_speed = 1023; //get_config_int("6502", "MAX_CLOCKS");
    clocks_per_sample = (cpu_max_speed * 1024) / freq;
    LOG_INF("%d cpu clocks per sample.\n", clocks_per_sample);

    SDL_Init(SDL_INIT_AUDIO);

    sound_buf = malloc(max_buf_size);
    memset(sound_buf, 127, max_buf_size);

    if (Mix_OpenAudio(freq, AUDIO_S8, 1, min_buf_size) == -1) {
        LOG_ERR("%s\n", Mix_GetError());
        return;
    } else {
        LOG_INF("Sound initialized.\n");
    }


    install_soft_switch(0x30, SS_READ, speaker_toggle, NULL);
    add_device(speaker_clock);

    Mix_HookMusic(sound_hook, NULL);
}


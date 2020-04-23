#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "logging.h"
#include "dynosaur.h"

struct channel_t {
    int amp;
    int cnt;
    BYTE vol;
    WORD freq;
};

struct {
    BYTE vol;
    int cnt;
    int amp;
    BYTE freq;
    bool type;
    WORD shiftReg;
} noise;

int noise_cnts[] = { 218, 109, 54 };

struct channel_t channel[3];

const double sample_freq = 22050;
double freq_div;

static inline int parity(int val) 
{
    val ^= val >> 8;
    val ^= val >> 4;
    val ^= val >> 2;
    val ^= val >> 1;
    return val & 0x0001;
}

static inline int frequency(int value)
{
    return (int)(((double) value) / (111860.8 / sample_freq)); 
}

void sound_hook(void *udata, Uint8 *stream, int len)
{
    int i;
    int ch;
    int vol;

    for (i = 0; i < len; ++i) {
        stream[i] = 0;
        for (ch = 0; ch < 3; ch++) {
            if (!(channel[ch].cnt--)) {
                channel[ch].cnt = frequency(channel[ch].freq) / 2;
                vol = (0x0f - channel[ch].vol) << 2;
                channel[ch].amp = (channel[ch].amp > 0)? -vol : vol;
            }
            stream[i] += channel[ch].amp;
        }

        if (noise.cnt == 0) {
            noise.cnt = (noise.freq == 0x03)?  frequency(channel[2].freq) : frequency(noise_cnts[noise.freq]);
            noise.shiftReg = (noise.shiftReg >> 1) | 
                ((noise.type? parity(noise.shiftReg & 0x0009) : noise.shiftReg & 0x0001) << 15);
            if (noise.shiftReg & 0x0001) {
                vol = (0x0f - noise.vol) << 2;
                noise.amp = (noise.amp > 0)? -vol : vol;
            }
        }
        noise.cnt--;
        stream[i] += noise.amp;          
        
    }

}

BYTE sound_port(BYTE port, bool read, BYTE value)
{
    static BYTE prev_channel = 0;
    BYTE ch;
    bool high_byte;
    static bool ctype;
    BYTE vol;
    WORD freq;

    if (!read) {
        SDL_LockAudio();
        if (value & 0x80) {
            prev_channel = (value >> 5) & 0x03;
            high_byte = false;
            ctype = (value & 0x10);
            value &= 0x0f;
        } else {
            value &= 0x3f;
            high_byte = true;
        }
        
        ch = prev_channel;
        if (ch < 3) {
            freq = channel[ch].freq;
            vol = channel[ch].vol;

            if (ctype) { 
                vol = value;
            } else {
                if (!high_byte) {
                    freq = (freq & 0xfff0) | (value & 0x0f);
                } else {
                    freq = (freq & 0x000f) | ((WORD) (value & 0x3f) << 4);
                }
            }
        
             //LOG_INF("Channel = %d  Freq = %04x (%f Hz), cnt = %f, vol = %02x\n", 
              //  ch, freq, (sample_freq * freq_div) / (double) freq, freq / freq_div, vol);
            channel[ch].freq = freq;
            channel[ch].vol = vol;
        } else {
            if (ctype) { 
                noise.vol = value;
            } else {
                noise.cnt = 0;
                noise.type = value & 0x04;
                noise.freq = value & 0x03;
                noise.shiftReg = 0x8000;
            }
            //LOG_INF("Noise type = %s, freq = %d, vol = %02x\n", noise.type? "White" : "Periodic", noise.freq, noise.vol);  
        }
        SDL_UnlockAudio();
    }

    return value;
}

void init_sound()
{
    LOG_INF("Initializing sound.\n");

    freq_div = 111860 / sample_freq;

    SDL_Init(SDL_INIT_AUDIO);
    if (Mix_OpenAudio(sample_freq, AUDIO_S8, 1, sample_freq / 64) == -1) {
        LOG_ERR("%s\n", Mix_GetError());
        return;
    }

    LOG_INF("Sound initialized.\n");

    attach_port(sound_port, 0x18);

    Mix_HookMusic(sound_hook, NULL);
}

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "logging.h" 
#include "dynosaur.h"

BYTE key_set[3];
BYTE keyboard_port(BYTE port, bool read, BYTE value)
{
    if (read) {
        value = key_set[port - 0x10];
        key_set[port - 0x10] = 0;
    }

    return value;
}

BYTE key_stat = 0;

BYTE key_stat_port(BYTE port, bool read, BYTE value)
{
    return key_stat;
}

void check_keyboard()
{
    SDL_Event e;

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_TEXTINPUT) {
            LOG_DBG("Key = '%s'\n", e.text.text);
            key_set[0] = e.text.text[0];
        } else if (e.type == SDL_KEYDOWN) {
            key_stat |= 0x01;
            LOG_DBG("Key = 0x%02x\n", e.key.keysym.sym);
            if (e.key.keysym.sym > 255)
                key_set[2] = e.key.keysym.sym & 0xff;
            else
                key_set[1] = e.key.keysym.sym & 0xff;
        } else if (e.type == SDL_KEYUP) {
            key_stat &= 0xfe;
        }

    }
}

void init_keyboard()
{
    LOG_INF("Initializing keyboard.\n");
    SDL_StartTextInput();
    attach_port(keyboard_port, 0x10);
    attach_port(keyboard_port, 0x11);
    attach_port(keyboard_port, 0x12);
    attach_port(key_stat_port, 0x13);
}

void finalize_keyboard()
{
    LOG_INF("Finalizing keyboard.\n");
    SDL_StopTextInput();
}

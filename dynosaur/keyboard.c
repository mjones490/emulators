#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "logging.h" 
#include "dynosaur.h"

BYTE last_key = 0;
BYTE keyboard_port(BYTE port, bool read, BYTE value)
{
    if (read) {
        value = last_key;
        last_key = 0;
    }

    return value;
}

void check_keyboard()
{
    SDL_Event e;

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_TEXTINPUT) {
            LOG_INF("Key = '%s'\n", e.text.text);
            last_key = e.text.text[0];
        }
    }
}

void init_keyboard()
{
    LOG_INF("Initializing keyboard.\n");
    SDL_StartTextInput();
    attach_port(keyboard_port, 0x10);
}

void finalize_keyboard()
{
    LOG_INF("Finalizing keyboard.\n");
    SDL_StopTextInput();
}

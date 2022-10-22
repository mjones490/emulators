#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "config.h"
#include "logging.h"
#include "bus.h"
#include "cpu_iface.h"

BYTE last_key;

const BYTE SS_KBD       = 0x00;
const BYTE SS_KBDSTRB   = 0x10;
const BYTE SS_RDBTN0    = 0x61;
const BYTE SS_BUTN1     = 0x62;

typedef void (*key_function_t)(SDL_Keycode, SDL_Keymod, bool down);

struct key_map_t {
    BYTE normal;
    BYTE shift;
    BYTE ctrl;
    key_function_t key_function;
};

struct key_map_t key_map[0x200];

bool open_apple = false;
bool solid_apple = false;

void apple_key(SDL_Keycode keycode, SDL_Keymod mod, bool down)
{
    LOG_DBG("%s apple %s\n", (keycode == SDLK_LALT)? "Open" : "Solid", 
        down? "down" : "up");

    if (keycode == SDLK_LALT)
        open_apple = down;
    else
        solid_apple = down;
}

BYTE ss_rdapples(BYTE switch_no, bool read, BYTE value)
{
    if (read) {
        if (switch_no == SS_RDBTN0)
            value = open_apple? 0x80 : 0x00;
        else
            value = solid_apple? 0x80: 0x00;
    }

    return value;
}

void reset_key(SDL_Keycode keycode, SDL_Keymod mod, bool down)
{
    if (down) {
        LOG_DBG("Reset key pressed.\n");
        cpu_toggle_signal(SIG_RESET);
    }
}

static void set_key(SDL_Keycode keycode, BYTE normal, BYTE shift, BYTE ctrl)
{
    if (keycode >= 0x100) 
        keycode = (keycode & 0x1ff) | 0x100;

    key_map[keycode].normal = normal;
    key_map[keycode].shift = shift;
    key_map[keycode].ctrl = ctrl;
    key_map[keycode].key_function = NULL;
}


static void set_key_function(SDL_Keycode keycode, key_function_t key_function)
{
    if (keycode >= 0x100) 
        keycode = (keycode & 0x1ff) | 0x100;

    key_map[keycode].key_function = key_function;
}


static void init_key_map()
{
    int i;

    LOG_DBG("Initializing keyboard map.\n");

    memset(key_map, 0x00, sizeof(key_map));

    set_key(SDLK_0,     '0',    ')',    0x00);
    set_key(SDLK_1,     '1',    '!',    0x00);
    set_key(SDLK_2,     '2',    '@',    0x00);
    set_key(SDLK_3,     '3',    '#',    0x00);
    set_key(SDLK_4,     '4',    '$',    0x00);
    set_key(SDLK_5,     '5',    '%',    0x00);
    set_key(SDLK_6,     '6',    '^',    0x00);
    set_key(SDLK_7,     '7',    '&',    0x00);
    set_key(SDLK_8,     '8',    '*',    0x00);
    set_key(SDLK_9,     '9',    '(',    0x00);

    for (i = 0; i < 26; ++i)
        set_key(SDLK_a + i, 'a' + i, 'A' + i, 0x01 + i);

    set_key(SDLK_BACKSLASH, '\\',   '|',    0x00);
    set_key(SDLK_BACKQUOTE, '`',    '~',    0x00);
    set_key(SDLK_COMMA,     ',',    '<',    0x00);
    set_key(SDLK_EQUALS,    '=',    '+',    0x00);
    set_key(SDLK_MINUS,     '-',    '_',    0x00);
    set_key(SDLK_PERIOD,    '.',    '>',    0x00);
    set_key(SDLK_QUOTE,     '\'',   '\"',   0x00);
    set_key(SDLK_SEMICOLON, ';',    ':',    0x00);
    set_key(SDLK_SLASH,     '/',    '?',    0x00);
    set_key(SDLK_RIGHTBRACKET, ']', '}',    0x00);
    set_key(SDLK_LEFTBRACKET, '[',  '{',    0x00);


    set_key(SDLK_LEFT,      0x08,   0x08,   0x08);
    set_key(SDLK_RIGHT,     0x15,   0x15,   0x15);
    set_key(SDLK_UP,        0x0a,   0x0a,   0x0a);
    set_key(SDLK_DOWN,      0x0b,   0x0b,   0x0b);

    set_key(SDLK_RETURN,    0x0d,   0x0d,   0x0d);
    set_key(SDLK_SPACE,     0x20,   0x20,   0x20);
    set_key(SDLK_BACKSPACE, 0x08,   0x08,   0x08);
    set_key(SDLK_ESCAPE,    0x1b,   0x1b,   0x1b);
    set_key(SDLK_TAB,       0x09,   0x09,   0x09);

    set_key_function(SDLK_LALT, apple_key);
    set_key_function(SDLK_RALT, apple_key);
    set_key_function(SDLK_F12, reset_key);
/*
    set_key(SDLK_LSHIFT,    0x80,   0x00,   0x00);
    set_key(SDLK_RSHIFT,    0x80,   0x00,   0x00);
    set_key(SDLK_LCTRL,     0x80,   0x00,   0x00);
    set_key(SDLK_RCTRL,     0x80,   0x00,   0x00);
    set_key(SDLK_CAPSLOCK,  0x80,   0x00,   0x00);
*/
}

static BYTE translate_key(SDL_Keycode keycode, SDL_Keymod mod, bool down)
{
    BYTE key;
    struct key_map_t map;
    SDL_Keycode idx = keycode;
    
    if (idx >= 0x100)
       idx = (idx & 0x1ff) | 0x100;

    map = key_map[idx];
    
    if (NULL != map.key_function) {
        map.key_function(keycode, mod, down);
        return 0;    
    } else if (!down || (map.normal + map.shift + map.ctrl) == 0)
        return 0;
    else if (mod & KMOD_SHIFT)
        key = map.shift;
    else if (mod & KMOD_CTRL)
        key = map.ctrl;
    else if (map.normal == 0x80)
        return 0x00;
    else 
        key = map.normal;

    if (key >= 'a' && key <= 'z' && !(mod & KMOD_CAPS))
        key ^= 0x20;

    return key | 0x80;

}

static BYTE ss_keyboard(BYTE switch_no, bool read, BYTE value)
{
    if (read && SS_KBD == switch_no)
        value = last_key;
    else
        last_key &= 0x7f;

    return value;
}

void check_keyboard()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (SDL_KEYDOWN == event.type)
                LOG_DBG("Detected key %02x!\n", event.key.keysym.sym);
            last_key = translate_key(event.key.keysym.sym, 
                event.key.keysym.mod, SDL_KEYDOWN == event.type); 
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                LOG_DBG("Window resized to %dx%d.\n", event.window.data1, 
                    event.window.data2);
            break;
        }
    }
}

static void keyboard_clock(BYTE clocks)
{
    static int keyboard_clocks = 0;

    keyboard_clocks -= clocks;
    if (keyboard_clocks <= 0) {
        check_keyboard();
        keyboard_clocks += 17030;
    }
}

void init_keyboard()
{
    LOG_DBG("Initilizing keyboard.\n");

    // Set keyboard soft switches
    install_soft_switch(SS_KBD, SS_READ, ss_keyboard);
    install_soft_switch(SS_KBDSTRB, SS_RDWR, ss_keyboard);
    install_soft_switch(SS_RDBTN0, SS_READ, ss_rdapples);
    install_soft_switch(SS_BUTN1, SS_READ, ss_rdapples);
    
    init_key_map();

    add_device(keyboard_clock);
}


#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <shell.h>
#include <6502_shell.h>
#include <SDL2/SDL.h>
#include "logging.h"
#include "bus.h"
#include "ROM.h"
#include "cpu_iface.h"

static bool watching = false;
static Uint32 prev_time = 0;
static int total_clocks;
static char buf[256];

static void watch_clock(BYTE clocks)
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;

    
    total_clocks += clocks;

    if (timer_diff >= 1000) {
        prev_time = timer;   
        if (watching) {
            sprintf(buf, "Clocks this second: %d\n", total_clocks);
            shell_print(buf);
        }
        total_clocks = 0;
    }
}

static int watch_clocks(int argc, char **argv)
{
    printf("Watch clocks...\n");
    watching = !watching;
    return 0;
}

void add_shell_commands()
{
    shell_add_command("clocks", "Watch clocks per second.", 
        watch_clocks, false);

    add_device(watch_clock);
}


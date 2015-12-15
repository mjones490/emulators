#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <6502_shell.h>
#include "config.h"
#include "logging.h"
#include "bus.h"
#include "ROM.h"

/**
 * Timing
 */

SDL_sem *clock_sem;

Uint32 prev_time = 0;
int remaining_clocks = 0;

void (*device_clock[8])(BYTE);
int num_devices = 0;

void add_device(void (*device_clock_hook)(BYTE))
{
    device_clock[num_devices++] = device_clock_hook;
}

void add_clocks()
{
    SDL_SemPost(clock_sem);
}

static bool has_clocks()
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;
   
    if (timer_diff >= 5) {
        //SDL_SemWait(clock_sem);
        prev_time = timer;
        remaining_clocks = timer_diff * 1023;
    }

    return remaining_clocks > 0;
}

static void use_clocks(BYTE clocks)
{
    int i;

    remaining_clocks -= clocks;

    for (i = 0; i < num_devices; ++i)
        device_clock[i](clocks);
}

void cpu_cycle()
{
    BYTE clocks;
    
    if (has_clocks()) {
        if (!cpu_get_signal(SIG_HALT))
            clocks = cpu_execute_instruction();
        else
            clocks = 2;

        use_clocks(clocks);
    } else
        use_clocks(0); 

}

void init_cpu()
{
    cpu_shell_load_commands();
    cpu_init(bus_accessor);
    cpu_set_signal(SIG_HALT);

    clock_sem = SDL_CreateSemaphore(0);
}
            

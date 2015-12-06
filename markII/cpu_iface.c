#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <6502_shell.h>
#include "config.h"
#include "logging.h"
#include "bus.h"
#include "ROM.h"
#include "video.h"
#include "keyboard.h"

/**
 * Timing
 */

Uint32 prev_time = 0;
int remaining_clocks = 0;
int clock_counter = 0;
int diff_count;

static bool has_clocks()
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;
    
    if (timer_diff >= 17) {
        if (++diff_count == 60) {
            LOG_DBG("Clocks = %d\n", clock_counter);
            clock_counter = 0;
            diff_count  = 0;
        }

        prev_time = timer;
        remaining_clocks += timer_diff * 1004;
        check_keyboard();
    }

    return remaining_clocks > 0;
}

static void use_clocks(BYTE clocks)
{
    clock_counter += clocks;
    remaining_clocks -= clocks;
}

void cpu_cycle()
{
    BYTE clocks;
    static bool prev_state = true; // true == halted
    
    if (has_clocks()) {
        if (!cpu_get_signal(SIG_HALT))
            clocks = cpu_execute_instruction();
        else
            clocks = 2;

        video_clock(clocks);
        use_clocks(clocks);
    }
}

            

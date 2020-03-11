/**
 * @file init.c
 */

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
#include "mmu.h"
#include "video.h"
#include "sound.h"
#include "keyboard.h"
#include "cpu_iface.h"
#include "slot.h"
#include "disk.h"
#include "markII_commands.h"

BYTE ss_trace(BYTE switch_no, bool read, BYTE value)
{
    return value;
}

static BYTE output_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    static char buf[256];
    static char *buf_ptr = buf;
  
    if (value)
        buf_ptr += sprintf(buf_ptr, "%c", value);

    if ('\n' == value || (buf_ptr - buf) > 254) { 
        shell_print(buf);
        buf_ptr = buf;
    }

    return value;
}

/**
 * Initialize everything.
 */
void init_all()
{
    init_logging(); 
    init_config("markII.cfg");
    char * ll = get_config_string("LOGGING", "LOG_LEVEL");
    LOG_DBG(ll);
    set_log_level(ll);
    LOG_DBG("Log set.\n");
       
    init_bus();
    init_mmu();
    init_slot();
    init_cpu();
    
    init_video();
    init_sound();
    init_keyboard();
    init_disk();

    install_soft_switch(0x2A, SS_WRITE, output_soft_switch);
    install_soft_switch(0x4B, SS_RDWR, ss_trace);

    shell_set_accessor(bus_accessor);
    shell_set_loop_cb(cpu_cycle);
    shell_initialize("MarkII");
    add_shell_commands();
    set_log_output_hook(shell_print);
    
    
}

/**
 * Finalize everyting.
 */
void finalize_all()
{
    shell_finalize();
    finalize_sound();
    finalize_video();
    finalize_disk();
    finalize_cpu();
    finalize_slot();
    free_page_buffers();
    free_page_block_list();
    finalize_config();
}

int main(int argc, char **argv)
{
    while (true) {
        init_all();
        shell_loop();
        finalize_all();
        if (!is_rebooting())
            break;
    }

    LOG_INF("Mark II Done\n");
    return 0;
}

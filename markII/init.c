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
#include "video.h"

Uint32 prev_time = 0;
int remaining_clocks = 0;
int clock_counter = 0;
int diff_count;
int sleep_loops = 0;
int wake_loops = 0;
int draw_clocks = 0;

SDL_Thread *shell_key_thread;
SDL_sem *shell_key_ready;

static int shell_key_thread_function(void *data)
{
    while (true) {
        if (shell_check_key())
        //    SDL_SemPost(shell_key_ready);
            shell_read_key();
    }

    return 0;
}

void init_shell_key_thread()
{
    shell_key_ready = SDL_CreateSemaphore(0);
    shell_key_thread = SDL_CreateThread(shell_key_thread_function, 
        "shell_key_thread_function", (void *) NULL);
}

bool has_clocks()
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;
    static int refresh = 0;
    if (timer_diff >= 17) {
        if (++diff_count == 60) {
            LOG_DBG("Clocks = %d, sleep_loops = %d, wake_loops = %d  \n", 
                clock_counter, sleep_loops);
            clock_counter = 0;
            diff_count  = 0;
            sleep_loops = 0;
            wake_loops = 0;
        }
        prev_time = timer;
        remaining_clocks += timer_diff * 1004;
    }

    return remaining_clocks > 0;
}

static void cycle()
{
    BYTE clocks;
    static bool prev_state = true; // true == halted
    
    if (!cpu_get_signal(SIG_HALT)) {
        if (true == prev_state) {
            prev_state = false;
            prev_time = SDL_GetTicks();
        }
        if (has_clocks()) {
            clocks = cpu_execute_instruction();
            remaining_clocks -= clocks;
            clock_counter += clocks;
            wake_loops++;
            video_clock(clocks);
        } else {
            sleep_loops++;
        }
    } else if (false == prev_state) {
        shell_print("CPU halted.\n");
        prev_state = true;
    }
/* 
    if (0 == SDL_SemTryWait(shell_key_ready)) {
        LOG_DBG("Have key ready...\n");
        LOG_DBG("Done.\n");
    }*/
}

            
BYTE RAM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
   
    if (NULL != pb->buffer) {
        if (read)
            value = pb->buffer[pb_offset(pb, address)];
        else
            pb->buffer[pb_offset(pb, address)] = value;
    } else {
        value = 0;
    }

    return value;
}

static BYTE *alt_zp_buf;
static BYTE *norm_zp_buf;

#define SS_SETSTDZP     0x08
#define SS_SETALTZP     0x09
#define SS_RDALTZP      0x16


BYTE zp_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0);

    if (SS_SETSTDZP == switch_no)
        pb->buffer = norm_zp_buf;
    else if(SS_SETALTZP == switch_no)
        pb->buffer = alt_zp_buf;
    else if (SS_RDALTZP == switch_no)
        value = (pb->buffer == alt_zp_buf)? 0x80 : 0x00;

    return value;
}

BYTE my_accessor(WORD address, bool read, BYTE value)
{
    return hi(address);
}

BYTE output_soft_switch(BYTE switch_no, bool read, BYTE value)
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

int anonymous_command(int argc, char **argv)
{
    int matches;
    int value, value2;
    char c;
    command_func_t peek_function = shell_get_command_function("peek");
    command_func_t poke_function = shell_get_command_function("poke");
   
    matches = sscanf(argv[1], "%04x%c", &value, &c);
    if (matches == 2 && c == ':' && NULL != poke_function) {
        poke_function(argc, argv);
    } else {
        matches = sscanf(argv[1], "%04x%c%04x", &value,
        &c, &value2);
        if (((2 <= matches && '-' == c) || (1 == matches)) &&
            (NULL != peek_function))
            peek_function(argc, argv);
        else
            printf("Unknown command \"%s\".\n", argv[1]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct page_block_t *my_pb;
    struct page_block_t *zpage_pb;
   
    init_logging(); 
    init_config();
    set_log_level();
       
    init_bus();

    my_pb = create_page_block(0, 8);
    my_pb->buffer = create_page_buffer(my_pb->total_pages);
    my_pb->accessor = RAM_accessor;
    install_page_block(my_pb);
    
    zpage_pb = create_page_block(0, 2);
    install_page_block(zpage_pb);
    alt_zp_buf = create_page_buffer(2);
    norm_zp_buf = zpage_pb->buffer;
    install_soft_switch(SS_SETSTDZP, SS_WRITE, zp_soft_switch);
    install_soft_switch(SS_SETALTZP, SS_WRITE, zp_soft_switch);
    install_soft_switch(SS_RDALTZP, SS_READ, zp_soft_switch);

    install_soft_switch(0x2A, SS_WRITE, output_soft_switch);

    // create some video ram
    my_pb = create_page_block(0x20, 0x20);
    my_pb->buffer = create_page_buffer(my_pb->total_pages);
    my_pb->accessor = RAM_accessor;
    install_page_block(my_pb);
    init_video();
     
    shell_set_accessor(bus_accessor);
    shell_set_loop_cb(cycle);
    shell_set_anonymous_command_function(anonymous_command);
    shell_initialize("MarkII");
    cpu_shell_load_commands();
    cpu_init(bus_accessor);
    cpu_set_signal(SIG_HALT);

    init_shell_key_thread();
    shell_loop();
    SDL_DetachThread(shell_key_thread);
    shell_finalize();

    LOG_INF("Mark II Done\n");
    return 0;
}

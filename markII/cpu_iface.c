/**
 * @file cpu_iface.c
 * Interface to the 6502 cpu library.
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <6502_shell.h>
#include <Stuff.h>
#include "config.h"
#include "logging.h"
#include "bus.h"


Uint32 prev_time = 0;       ///< Last time checked
int remaining_clocks = 0;   ///< Remaining CPU clocks

/**
 * List of devices that need cpu clocks
 */
struct device_t {
    void (*clock)(BYTE clocks); ///< Function pointer for device clocks
    struct list_head list;      ///< Next device in list
};

struct list_head device_list; ///< First device in list

/**
 * Allocate one device and add it to the list.
 * @param[in] device_clock_hook Function pointer for device clocks
 */
void add_device(void (*device_clock_hook)(BYTE))
{
    struct device_t *device = malloc(sizeof(struct device_t));
    device->clock = device_clock_hook;
    list_add(&device->list, &device_list);
}

/**
 * Checks to see if there are any remaining CPU clocks.  If none are remaining,
 * and one or more milliseconds have passed since last check, add clocks
 * to remaining clocks.
 * @returns true if greater than 0 clocks remain.
 */
static bool has_clocks()
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;
   
    if (remaining_clocks <= 0 && timer_diff >= 1) {
        prev_time = timer;
        remaining_clocks = timer_diff * 1023;
    }

    return remaining_clocks > 0;
}

/**
 * Subtracts clocks from remaining clocks.  Calls each device in device list
 * with number of clocks subtraced.
 * @param clocks Number of clocks used.
 */
static void use_clocks(BYTE clocks)
{
    struct device_t *device;
    struct list_head *tmp;

    remaining_clocks -= clocks;

    LIST_FOR_EACH(tmp, &device_list) {
        device = GET_ELEMENT(struct device_t, list, tmp);
        device->clock(clocks);
    }
}

/**
 * If enough clocks are remaining, and SIG_HALT is not set, execute
 * one cpu instruction.
 */
void cpu_cycle()
{
    BYTE clocks;
    
    if (has_clocks()) {
        if (!cpu_get_signal(SIG_HALT)) {
            if (cpu_get_signal(SIG_RESET)){
                LOG_INF("RESET signal received.\n");
                bus_accessor(0xc082, true, 0);
            }

            clocks = cpu_execute_instruction();
        } else
            clocks = 0;

        use_clocks(clocks);
    } else
        use_clocks(0); 

}

int (*old_step)(int argc, char **argv);

int step(int argc, char **argv)
{
    old_step(argc, argv);
    use_clocks(cpu_get_clocks());
    return 0;
}

/**
 * Initialize cpu.
 */
void init_cpu()
{
    cpu_shell_load_commands();
    old_step = shell_add_command("step", "Overridden step command", 
        step, true);
    cpu_init(bus_accessor);
    cpu_set_signal(SIG_HALT);

    if (get_config_bool("MARKII", "AUTO_START")) {
        cpu_set_signal(SIG_RESET);
        cpu_clear_signal(SIG_HALT);
    }   

    INIT_LIST_HEAD(&device_list);
}

/**
 * Finalize cpu.  Remove all devices from device list.
 */
void finalize_cpu()
{
    struct device_t *device;
    struct list_head *tmp;
    struct list_head * next;

    LOG_INF("Clearing device list...\n");
    LIST_FOR_EACH_SAFE(tmp, next, &device_list) {
        device = GET_ELEMENT(struct device_t, list, tmp);
        list_remove(&device->list);
        free(device);
    }
}

            

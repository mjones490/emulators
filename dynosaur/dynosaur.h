#ifndef __DYNOSAUR_H
#define __DYNOSAUR_H
#include <SDL2/SDL.h>
#include "types.h"
#include "plugin.h"
#include "bus.h"
#include "jLib.h"

struct config_t {
    char cpu_config[32];
    char *cpu_name;
    char *cpu_plugin;
    int ram_size;
    char *bin_dir;
    Uint32 clock_speed;
    jVal *root;
};

extern struct cpu_interface *cpu;
extern struct config_t dyn_config;
extern WORD breakpoint;
extern WORD nextpoint;

static inline BYTE put_byte(WORD address, BYTE value)
{
    return bus_accessor(address, false, value);
}

static inline BYTE get_byte(WORD address)
{
    return bus_accessor(address, true, 0);
}
#endif

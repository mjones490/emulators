#ifndef __CPU_IFACE_H
#define __CPU_IFACE_H
#include <types.h>

void cpu_cycle();
void add_device(void (*device_clock_hook)(BYTE));
void add_clocks();
void init_cpu();

#endif

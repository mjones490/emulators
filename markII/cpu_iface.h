#ifndef __CPU_IFACE_H
#define __CPU_IFACE_H
#include <types.h>

void cpu_cycle();
void add_device(void (*device_clock_hook)(BYTE));
void init_cpu();
void finalize_cpu();

#endif

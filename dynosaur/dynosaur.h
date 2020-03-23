#ifndef __DYNOSAUR_H
#define __DYNOSAUR_H
#include "types.h"
#include "plugin.h"

extern struct cpu_interface *cpu;
extern BYTE *ram;
extern WORD breakpoint;

typedef BYTE (*port_accessor_t)(BYTE port, bool read, BYTE value);

void attach_bus(accessor_t accessor, BYTE start_page, BYTE num_pages);
void attach_port(port_accessor_t accessor, BYTE port);

BYTE bus_accessor(WORD address, bool read, BYTE value);

static inline BYTE put_byte(WORD address, BYTE value)
{
    return bus_accessor(address, false, value);
}

#endif

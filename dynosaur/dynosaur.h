#ifndef __DYNOSAUR_H
#define __DYNOSAUR_H
#include "types.h"
#include "plugin.h"

extern struct cpu_interface *cpu;
extern BYTE *ram;
extern WORD breakpoint;

BYTE ram_accessor(WORD address, bool read, BYTE value);

static inline BYTE put_byte(WORD address, BYTE value)
{
    return ram_accessor(address, false, value);
}

#endif

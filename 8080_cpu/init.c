#include "8080_cpu.h"
#include "cpu_types.h"

void cpu_init(accessor_t bus)
{
    cpu_state.regs = &regs;
    cpu_state.bus = bus;
    
    map_instructions();
}

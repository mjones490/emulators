#include "8080_cpu.h"
#include "cpu_types.h"

void cpu_init(accessor_t bus, port_accessor_t ports)
{
    cpu_state.regs = &regs;
    cpu_state.bus = bus;
    cpu_state.port = ports;
    cpu_state.halted = true;

    map_instructions();
}

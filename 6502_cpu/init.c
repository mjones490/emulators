#include "instructions.h"

void init_address_modes()
{
    set_address_mode(IMM, immediate);
    set_address_mode(ACC, accumulator);
    set_address_mode(ABS, absolute);
    set_address_mode(AIX, absolute_X);
    set_address_mode(AIY, absolute_Y);
    set_address_mode(ZP, zero_page);
    set_address_mode(ZPX, zero_page_X);
    set_address_mode(ZPY, zero_page_Y);
    set_address_mode(ZPIX, zero_page_indirect_X);
    set_address_mode(ZPIY, zero_page_indirect_Y);
}

void cpu_init(accessor_t bus) {
    cpu_state.regs = &regs;
    cpu_state.bus = bus;
    cpu_state.signal = SIG_NONE;
    init_address_modes();
    init_instructions();
}


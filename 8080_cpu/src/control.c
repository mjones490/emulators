#include "8080_cpu.h"
#include "cpu_types.h"
#include "instructions.h"

union regs_t regs;
struct cpu_state_t cpu_state;

static BYTE exec_instruction(BYTE code)
{
    cpu_state.code = code;
    cpu_state.total_clocks = instruction_map[code]->clocks;
    instruction_map[code]->handler();    
    return cpu_state.total_clocks;
}

BYTE cpu_execute_instruction()
{
    return exec_instruction(get_next_byte());
}

void cpu_set_halted(bool halted)
{
    cpu_state.halted = halted;
}

bool cpu_get_halted()
{
    return cpu_state.halted;
}


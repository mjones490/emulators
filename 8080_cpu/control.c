#include "8080_cpu.h"
#include "cpu_types.h"
#include "instructions.h"

union regs_t regs;
struct cpu_state_t cpu_state;

static BYTE exec_instruction(BYTE code)
{
    if (instruction_map[code] > 0) {
        cpu_state.code = code;
        instruction_map[code]->handler();
    }
    
    return 0;
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


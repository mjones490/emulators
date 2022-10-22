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
    WORD pc = cpu_get_reg_PC();
    set_signal(SIG_INST_FETCH);
    BYTE code = get_byte(pc);
    clear_signal(SIG_INST_FETCH);
   
    if (!get_signal(SIG_INT_ENABLED) || !get_signal(SIG_INTERRUPT))
        cpu_set_reg_PC(pc + 1);
    else if (get_signal(SIG_INTERRUPT))
        clear_signal(SIG_INTERRUPT);

    return exec_instruction(code);    
}

void cpu_set_halted(bool halted)
{
    cpu_state.halted = halted;
}

bool cpu_get_halted()
{
    return cpu_state.halted;
}


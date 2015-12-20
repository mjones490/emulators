#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu_types.h"
#include "instructions.h"
#include "modes.h"

// Figure out were to put these two structs.
struct cpu_state_t cpu_state;
struct regs_t regs;

struct running_t running;

BYTE cpu_get_clocks()
{
    return running.clocks;
}

BYTE exec_instruction(BYTE code)
{    
    running.instruction = &instruction[code];
    running.descripter = 
        &instruction_desc[running.instruction->nmonic];

    if (NULL != running.descripter->handler) {
        running.op_mode = running.instruction->mode;
        running.clocks = running.instruction->clocks;
        running.descripter->handler();
    } else 
        return 0;

    return running.clocks;
}

BYTE cpu_execute_instruction()
{
    static bool interrupts_enabled;
    
    interrupts_enabled = are_clear(I);

    if (get_signal(SIG_CLI)) {
        clear_flags(I);
        clear_signal(SIG_CLI);
    }
        
    if (get_signal(SIG_SEI)) {
        set_flags(I);
        clear_signal(SIG_SEI);
    }        

    if (get_signal(SIG_RESET)) {
        regs.SP -= 3;
        regs.PC = get_word(0xFFFC);
        set_flags(I);
        clear_signal(SIG_RESET);
    } else if (get_signal(SIG_NMI)) {
        interrupt(0xFFFA);
        clear_signal(SIG_NMI);
    } else if (interrupts_enabled && get_signal(SIG_IRQ)) {
        interrupt(0xFFFE);
    }
 
    cpu_state.prev_PC = regs.PC;   
    return exec_instruction(get_next_byte());
}


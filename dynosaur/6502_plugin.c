#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_INTERFACE
#include "plugin.h"
#include "shell.h"
#include "6502_shell.h"
#include "6502_cpu.h"

static void set_halted(bool halted)
{
    if (halted)
        cpu_set_signal(SIG_HALT);
    else
        cpu_clear_signal(SIG_HALT);
}

static bool get_halted()
{
    return cpu_get_signal(SIG_HALT);
}

static WORD disassemble(WORD address)
{
    disasm_instr(&address);
    printf("\n");
    return address;
}

struct cpu_interface interface;

/*
struct shell_commands_t shell_commands[] = {
    { "registers", "View/Change 6502 registers.", registers, false },
    { "disassemble", "Dissassemble 6502 instructions", disassemble, true }
};
*/

void initialize(accessor_t accessor)
{
    cpu_init(accessor);
    set_halted(true);
}

void finalize()
{
    printf("\n");
}

struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.finalize = finalize;
    interface.execute_instruction = cpu_execute_instruction;
    interface.show_registers = show_registers;
    interface.set_register = set_register;
    interface.disassemble = disassemble;
    interface.set_halted = set_halted;
    interface.get_halted = get_halted;
    interface.set_PC = cpu_set_reg_PC;
    interface.get_PC = cpu_get_reg_PC;
    interface.shell_commands = NULL; //shell_commands;
    interface.num_shell_commands = 0; //SHELL_COMMANDS_CNT(shell_commands);

    return &interface;
}

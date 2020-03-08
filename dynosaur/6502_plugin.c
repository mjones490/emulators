#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_INTERFACE
#include "plugin.h"
#include "shell.h"
#include "6502_shell.h"
#include "6502_cpu.h"

void initialize(accessor_t accessor)
{
    cpu_init(accessor);
}

void finalize()
{
    printf("\n");
}

void set_halted(bool halted)
{
    if (halted)
        cpu_set_signal(SIG_HALT);
    else
        cpu_clear_signal(SIG_HALT);
}

bool get_halted()
{
    return cpu_get_signal(SIG_HALT);
}

struct cpu_interface interface;

struct shell_commands_t shell_commands[] = {
    { "registers", "View/Change 6502 registers.", registers, false },
    { "disassemble", "Dissassemble 6502 instructions", disassemble, true }
};

struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.finalize = finalize;
    interface.execute_instruction = cpu_execute_instruction;
    interface.show_registers = show_registers;
    interface.set_halted = set_halted;
    interface.get_halted = get_halted;
    interface.set_PC = cpu_set_reg_PC;
    interface.shell_commands = shell_commands;
    interface.num_shell_commands = SHELL_COMMANDS_CNT(shell_commands);

    return &interface;
}

#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_INTERFACE
#include "plugin.h"
#include "8080_shell.h"
#include "8080_cpu.h"

BYTE ports(BYTE port, bool read, BYTE value)
{
    return 0;
}

void initialize(accessor_t accessor)
{
    cpu_init(accessor, ports);
}

void finalize()
{
    printf("\n");
}

struct cpu_interface interface;

struct shell_commands_t shell_commands[] = {
    { "registers", "View/Change 8080 registers.", registers, false },
    { "dissassemble", "Disassemble 8080 instructions.", disassemble, true }
};

struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.finalize = finalize;
    interface.execute_instruction = cpu_execute_instruction;
    interface.show_registers = show_registers;
    interface.set_halted = cpu_set_halted;
    interface.get_halted = cpu_get_halted;
    interface.set_PC = cpu_set_reg_PC;
    interface.shell_commands = shell_commands;
    interface.num_shell_commands = SHELL_COMMANDS_CNT(shell_commands);
    return &interface;
}

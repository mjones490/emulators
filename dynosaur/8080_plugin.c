#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_INTERFACE
#include "plugin.h"
#include "8080_shell.h"
#include "8080_cpu.h"
#include "cpu_types.h"

static accessor_t real_accessor;

BYTE plugin_accessor(WORD address, bool read, BYTE value)
{
    if (read && get_signal(SIG_INT_ENABLED) && get_signal(SIG_INTERRUPT) && 
        get_signal(SIG_INST_FETCH)) {
        value = reset_code(cpu_state.interrupt_vector);
    } else {
        value = real_accessor(address, read, value);
    }

    return value;
}

void initialize(accessor_t accessor, port_accessor_t ports)
{
    real_accessor = accessor;
    cpu_init(plugin_accessor, ports);
}

void finalize()
{
    printf("\n");
}

void interrupt(BYTE vector)
{
    cpu_state.interrupt_vector = vector;
    set_signal(SIG_INTERRUPT);
}

struct cpu_interface interface;
/*
struct shell_commands_t shell_commands[] = {
    { "registers", "View/Change 8080 registers.", registers, false },
    { "dissassemble", "Disassemble 8080 instructions.", disassemble, true }
};
*/
struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.finalize = finalize;
    interface.execute_instruction = cpu_execute_instruction;
    interface.show_registers = show_registers;
    interface.set_register = set_register;
    interface.disassemble = disassemble_inst;
    interface.set_halted = cpu_set_halted;
    interface.get_halted = cpu_get_halted;
    interface.set_PC = cpu_set_reg_PC;
    interface.get_PC = cpu_get_reg_PC;
    interface.interrupt = interrupt;
    interface.shell_commands = NULL; // shell_commands;
    interface.num_shell_commands = 0; // SHELL_COMMANDS_CNT(shell_commands);
    return &interface;
}

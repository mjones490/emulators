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

accessor_t dynamic_accessor;
port_accessor_t dynamic_port_accessor;

#define TIMER_LOW       0x6004
#define TIMER_HIGH      0x6005
#define STD_KEY         0x6011
#define KEY             0x6012
#define KEY_STAT        0x6013

static BYTE timer_low = 0;
static BYTE timer_high = 0;
static BYTE real_accessor(WORD address, bool read, BYTE value) 
{
    switch (address) {
        case 0x6001:
            value = dynamic_port_accessor(0x18, read, value);
            break;

        case TIMER_LOW:
            if (read) {
                cpu_clear_signal(SIG_IRQ);
            } else {
                timer_low = value;
            }
            break;

        case TIMER_HIGH:
            timer_high = value;
            dynamic_port_accessor(0x08, read, timer_low);
            dynamic_port_accessor(0x09, read, timer_high);
            break;

        case KEY:
        case STD_KEY:
        case KEY_STAT:
            value = dynamic_port_accessor(address & 0xff, read, value);
            break;

        default:
            value = dynamic_accessor(address, read, value);
    }
    
    return value;
}

void initialize(accessor_t accessor, port_accessor_t port_accessor)
{
    dynamic_accessor = accessor;
    dynamic_port_accessor = port_accessor;

    cpu_init(real_accessor);
    set_halted(true);
}

void finalize()
{
    printf("\n");
}

void cpu_interrupt(BYTE vector)
{
    cpu_set_signal(SIG_IRQ);
}

struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.finalize = finalize;
    interface.execute_instruction = cpu_execute_instruction;
    interface.show_registers = show_registers;
    interface.set_register = set_register;
    interface.disassemble = disassemble;
    interface.get_instruction_size = cpu_get_instruction_size;
    interface.set_halted = set_halted;
    interface.get_halted = get_halted;
    interface.set_PC = cpu_set_reg_PC;
    interface.get_PC = cpu_get_reg_PC;
    interface.shell_commands = NULL; //shell_commands;
    interface.num_shell_commands = 0; //SHELL_COMMANDS_CNT(shell_commands);
    interface.interrupt = cpu_interrupt;
    return &interface;
}

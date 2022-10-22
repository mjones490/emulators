#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "shell.h"
#include "instructions.h"
#include "6502_cpu.h"
#include "cpu_types.h"

static int disassemble(int argc, char **argv)
{
    int matched;
    int start;
    int end;
    static WORD address;
    static WORD extend = 1;

    if (1 < argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start;
            if (matched == 1) { 
                end = address;
                extend = 1;
            } else {
                extend = end - address;
            }
        }
    } else {
        end = address + extend;
    }

    do {
        disasm_instr(&address);
        printf("\n");
    } while (address < end);

    return 0;
}

/**********************************************************
   Registers command
 **********************************************************/
static void show_registers()
{
    printf("a = %02x ", cpu_get_reg_A());
    printf("x = %02x ", cpu_get_reg_X());
    printf("y = %02x ", cpu_get_reg_Y());
    printf("sp = %02x ", cpu_get_reg_SP());
    printf("pc = %04x ", cpu_get_reg_PC());
    printf("ps = %02x ", cpu_get_reg_PS());
    printf("\n");
}

static int registers(int argc, char **argv)
{
    int tmp;
    WORD value = 0;
    char *reg_name;

    if (argc == 3) {
        reg_name = argv[1];
        if (1 == sscanf(argv[2], "%4x", &tmp))
            value = (WORD) tmp;    
        if (0 == strncmp(reg_name, "a", 1))
            cpu_set_reg_A(lo(value));
        else if (0 == strncmp(reg_name, "x", 1))
            cpu_set_reg_X(lo(value));
        else if (0 == strncmp(reg_name, "y", 1))
            cpu_set_reg_Y(lo(value));
        else if (0 == strncmp(reg_name, "sp", 1))
            cpu_set_reg_SP( lo(value));
        else if (0 == strncmp(reg_name, "pc", 2))
            cpu_set_reg_PC(value);
    }

    show_registers();
    return 0;
}

/**********************************************************
   Step command
 **********************************************************/
static int step(int argc, char **argv)
{
    WORD address;
    int clocks;
    clocks = cpu_execute_instruction();
    address = cpu_get_prev_PC();
    disasm_instr(&address);
    printf("\t; ");
    if (0 == clocks)
        printf("Bad instruction\n");
    else
        printf("%d clocks\n", clocks);
    show_registers();
    return 0;
}

static int go(int argc, char **argv)
{
    int tmp;

    if (1 < argc) {
        if (1 == sscanf(argv[1], "%4x", &tmp))
            cpu_set_reg_PC((WORD) tmp);
    }

    cpu_clear_signal(SIG_HALT);
    return 0;        
}

static int assert_interrupt(int argc, char **argv)
{
    if (0 == strncmp(argv[0], "nmi", 4))
        cpu_toggle_signal(SIG_NMI);
    else if (0 == strncmp(argv[0], "irq", 4))
        cpu_toggle_signal(SIG_IRQ);
    else if (0 == strncmp(argv[0], "reset", 6))
        cpu_toggle_signal(SIG_RESET);
    else if (0 == strncmp(argv[0], "halt", 5))
        cpu_toggle_signal(SIG_HALT);
    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 6502 register.", 
        registers, false);
    shell_add_command("disassemble", "Disassemble code.", disassemble, true);
    shell_add_command("step", "Step through code.", step, true);
    shell_add_command("go", "Start program.", go, false);
    shell_add_command("nmi", "Assert NMI.", assert_interrupt, false);
    shell_add_command("irq", "Assert IRQ.", assert_interrupt, false);
    shell_add_command("reset", "Reset CPU.", assert_interrupt, false);
    shell_add_command("halt", "Halt CPU", assert_interrupt, false);
}


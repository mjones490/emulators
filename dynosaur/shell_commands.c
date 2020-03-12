#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "types.h"
#include "dynosaur.h"

static int step(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->execute_instruction();
    cpu->show_registers();
    cpu->disassemble(cpu->get_PC());
    return 0;
}

static int go(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->set_halted(false);
    return 0;
}

static int halt(int argc, char **argv)
{
    cpu->set_halted(true);
    return 0;
}

static int set_breakpoint(int argc, char **argv)
{
    WORD temp;

    if (argc == 2) {
        if (1 == sscanf(argv[1], "%04hx", &temp))
            breakpoint = temp;
    }

    printf("Breakpoint set at %04x.\n", breakpoint);

    return 0;
}

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
        address = cpu->disassemble(address);
    } while (address < end);

    return 0;
}

static int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) 
            cpu->set_register(reg_name, (WORD) tmp);
    }
    
    cpu->show_registers();
    return 0;
}

void shell_load_dynosaur_commands()
{
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("go", "Start CPU.", go, false);
    shell_add_command("halt", "Halt CPU", halt, false);
    shell_add_command("registers", "View/Set CPU registers", registers, false);
    shell_add_command("disassemble", "Disassemble instructions", disassemble, true);
    shell_add_command("breakpoint", "View/Set breakpoint", set_breakpoint, false);
}
    

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include "shell.h"
#include "8080_cpu.h"
#include "cpu_types.h"

const int ram_size = 8192;

BYTE *ram;

BYTE my_accessor(WORD address, bool read_byte, BYTE value)
{
    if (address >= ram_size)
        return 0x00;
    if (read_byte)
        value = ram[address];
    else
        ram[address] = value;

    return value;
}

/**********************************************
   Registers command 
 **********************************************/
static void show_registers() 
{
    printf("a = %02x ", cpu_get_reg_A());
    printf("b = %02x ", cpu_get_reg_B());
    printf("c = %02x ", cpu_get_reg_C());
    printf("d = %02x ", cpu_get_reg_D());
    printf("e = %02x ", cpu_get_reg_E());
    printf("hl = %04x ", cpu_get_reg_HL());
    printf("sp = %04x ", cpu_get_reg_SP());
    printf("pc = %04X ", cpu_get_reg_PC());
    
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
        if (0 == strncmp(reg_name, "a", 2))
            cpu_set_reg_A(lo(value));
        else if (0 == strncmp(reg_name, "b", 2))
            cpu_set_reg_B(lo(value));
        else if (0 == strncmp(reg_name, "c", 2))
            cpu_set_reg_C(lo(value));
        else if (0 == strncmp(reg_name, "bc", 3))
            cpu_set_reg_BC(value);
        else if (0 == strncmp(reg_name, "d", 2))
            cpu_set_reg_D(lo(value));
        else if (0 == strncmp(reg_name, "e", 2))
            cpu_set_reg_E(lo(value));
        else if (0 == strncmp(reg_name, "de", 3))
            cpu_set_reg_DE(value);
        else if (0 == strncmp(reg_name, "h", 2))
            cpu_set_reg_H(lo(value));
        else if (0 == strncmp(reg_name, "l", 2))
            cpu_set_reg_L(lo(value));
        else if (0 == strncmp(reg_name, "hl", 3))
            cpu_set_reg_HL(value);
        else if (0 == strncmp(reg_name, "sp", 3))
            cpu_set_reg_SP(value);
        else if (0 == strncmp(reg_name, "pc", 3))
            cpu_set_reg_PC(value);
    }
    
    show_registers();
    return 0;
}

static int step()
{
    cpu_execute_instruction();
    show_registers();
    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 8080 registers.", registers, false);
    shell_add_command("step", "Execute single instruction.", step, true);
}

int main(int argc, char **argv)
{
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("8080 shell");
    cpu_init(my_accessor);
    cpu_shell_load_commands();
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}

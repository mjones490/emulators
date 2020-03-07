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
#include <ctype.h>
#include "shell.h"
#include "8080_cpu.h"
#include "cpu_types.h"
#include "8080_shell.h"

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

BYTE my_ports(BYTE port, bool read_byte, BYTE value)
{
    if (port == 1) {
        printf("%c", value);
    //    fflush(stdout);
    }

    return value;
}

static void execute_instruction()
{
    BYTE already_halted = cpu_get_halted();
    cpu_execute_instruction();
    if (!already_halted && cpu_get_halted()) {
        printf("System halted.\n");
    }
}

static void cycle()
{
    usleep(10);

    if (!cpu_get_halted()) {
        if (cpu_state.breakpoint == cpu_get_reg_PC()) {
            printf("Breakpoint at %04xh reached.\n", cpu_get_reg_PC());
            cpu_set_halted(true);
        } else {
            execute_instruction();
        }
    }
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 8080 registers.", registers, false);
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("disassemble", "Disassemble code.", disassemble, true);
    shell_add_command("go", "Start program.", go, false);
    shell_add_command("halt", "Halt CPU.", halt, false);
    shell_add_command("load", "Load a binary file the given address.", load, false);
    shell_add_command("breakpoint", "Set or view the PC breakpoint.", breakpoint, false);
}


int main(int argc, char **argv)
{
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("8080 shell");
    cpu_init(my_accessor, my_ports);
    cpu_shell_load_commands();
    shell_set_loop_cb(cycle);
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}

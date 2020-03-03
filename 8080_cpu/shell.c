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
WORD PC_breakpoint = 0;

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
        if (PC_breakpoint == cpu_get_reg_PC()) {
            printf("Breakpoint at %04xh reached.\n", cpu_get_reg_PC());
            cpu_set_halted(true);
        } else {
            execute_instruction();
        }
    }
}

static int breakpoint(int argc, char **argv)
{
    int tmp;

    if (argc == 2 && 1 == sscanf(argv[1], "%4x", &tmp))
        PC_breakpoint = (WORD)tmp;

    printf("Breakpoint set at %04xh.\n", PC_breakpoint);

    return 0;
}

int main(int argc, char **argv)
{
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("8080 shell");
    cpu_init(my_accessor, my_ports);
    cpu_shell_load_commands();
    shell_add_command("breakpoint", "Set or view the PC breakpoint.", breakpoint, false);
    shell_set_loop_cb(cycle);
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}

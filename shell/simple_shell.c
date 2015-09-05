#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shell.h"
#include "types.h"

BYTE RAM[0x4000];
static BYTE RAM_accessor(WORD address, bool read, BYTE value)
{
    if (read)
        value = RAM[address];
    else 
        RAM[address] = value;

    return value;
}


command_func_t old_help;
int new_help(int argc, char **argv)
{
    printf("This help function shows how to override and reuse"
        " commands already added to help.\n\n");

    return old_help(argc, argv);
}

int main(int argc, char **argv)
{
    shell_set_accessor(RAM_accessor);
    shell_initialize("simple_shell");
    old_help = shell_add_command("help", "Shell help.", new_help, false);
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}

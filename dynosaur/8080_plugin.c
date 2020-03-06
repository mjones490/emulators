#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_INTERFACE
#include "plugin.h"
#include "shell.h"
#include "8080_shell.h"
#include "8080_cpu.h"

BYTE ports(BYTE port, bool read, BYTE value)
{
    return 0;
}

void initialize(char* prompt, accessor_t accessor)
{
    shell_set_accessor(accessor);
    shell_initialize(prompt);
    cpu_init(accessor, ports);
    cpu_shell_load_commands();
}

void finalize()
{
    shell_finalize();
    printf("\n");
}

struct cpu_interface interface;

struct cpu_interface *get_cpu_interface()
{
    interface.initialize = initialize;
    interface.loop = shell_loop;
    interface.finalize = finalize;

    return &interface;
}

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "plugin.h"
#include "shell_commands.h"

struct cpu_interface *cpu;

BYTE *ram;
const int ram_size = 8192;

BYTE ram_accessor(WORD address, bool read, BYTE value)
{
    if (address > ram_size)
        value = 0;
    else if (read)
        value = ram[address];
    else
        ram[address] = value;

    return value;
}

void *load_plugin(char *plugin_name)
{
    void *handle;
    char *error;
    int i;

    handle = dlopen(plugin_name, RTLD_LAZY);
    if (handle == NULL) {
        fprintf(stderr, "dynosaur: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    get_cpu_interface = dlsym(handle, "get_cpu_interface");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "dynosaur: %s\n", error);
        exit(EXIT_FAILURE);
    }

    cpu = get_cpu_interface();

    for (i = 0; i < cpu->num_shell_commands; i++)
        shell_add_command(cpu->shell_commands[i].name, cpu->shell_commands[i].description,
            cpu->shell_commands[i].function, cpu->shell_commands[i].is_repeatable);

    return handle;
}

void unload_plugin(void *handle)
{
    dlclose(handle);
}

WORD breakpoint = 0;
static void execute_instruction()
{
    if (!cpu->get_halted()) {
        cpu->execute_instruction();
        if (cpu->get_PC() == breakpoint) {
            printf("Breakpoint reached.\n");
            cpu->set_halted(true);
        }
    }
}

static void cycle()
{
    usleep(10);

    execute_instruction();
}

int main(int argv, char **argc)
{
    void *plugin;
    char soname[256];
    char prompt[256];

    printf("Dynosaur\n");

    if (argv == 2) {
        sprintf(soname, "./lib%s.so", argc[1]);
        sprintf(prompt, "dynamic %s", argc[1]);
    } else {
        strcpy(soname, "./lib8080.so");
        strcpy(prompt, "dynamic 8080");
    }
    
    shell_set_accessor(ram_accessor);
    shell_initialize(prompt);
    shell_load_dynosaur_commands();

    plugin = load_plugin(soname);
    ram = malloc(ram_size);
    cpu->initialize(ram_accessor);
    shell_set_loop_cb(cycle);
    shell_loop();
    cpu->finalize();
    shell_finalize();
    free(ram);
    unload_plugin(plugin);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "plugin.h"

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

    handle = dlopen(plugin_name, RTLD_LAZY);
    if (handle == NULL) {
        fprintf(stderr, "dynosaur: %s\n", dlerror());
        exit(1);
    }

    get_cpu_interface = dlsym(handle, "get_cpu_interface");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "dynosaur: %s\n", error);
        exit(1);
    }

    cpu = get_cpu_interface();

    return handle;
}

void unload_plugin(void *handle)
{
    dlclose(handle);
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
    plugin = load_plugin(soname);
    ram = malloc(ram_size);
    cpu->initialize(prompt, ram_accessor);
    cpu->loop();
    cpu->finalize();
    free(ram);
    unload_plugin(plugin);

    return 0;
}

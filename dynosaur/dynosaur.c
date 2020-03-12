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
#include "logging.h"
#include "config.h"

static struct {
    char *cpu_config;
    char *cpu_name;
    char *cpu_plugin;
} config;

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

void *load_plugin()
{
    void *handle;
    char *error;
    int i;

    handle = dlopen(config.cpu_plugin, RTLD_LAZY);
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

void load_config(char *param)
{
    init_config("dynosaur.cfg");
    if (param == NULL) {
        config.cpu_config = get_config_string("DYNOSAUR", "DEFAULT_CPU");
        if (config.cpu_config == NULL)
            LOG_FTL("Cannot determine default configuration.\n");
    } else {
        config.cpu_config = malloc(32);
        sprintf(config.cpu_config, "%s_CPU", param);
    }

    config.cpu_name = get_config_string(config.cpu_config, "NAME");
    
    if (config.cpu_name == NULL)
        LOG_ERR("No CPU name.\n");
    else
        LOG_INF("CPU Name is %s.\n", config.cpu_name);

    config.cpu_plugin = get_config_string(config.cpu_config, "PLUGIN");
    if (config.cpu_plugin == NULL)
        LOG_FTL("Cannot determine CPU plugin file name.\n");
    LOG_INF("CPU plugin file name = %s.\n", config.cpu_plugin);
}

int main(int argv, char **argc)
{
    void *plugin;

    printf("Dynosaur\n");
   
    init_logging();
    
    load_config(argc[1]);
    shell_set_accessor(ram_accessor);
    shell_initialize("dynosaur");
    shell_load_dynosaur_commands();

    plugin = load_plugin();
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

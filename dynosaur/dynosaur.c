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
#include "video.h"

static struct {
    char cpu_config[32];
    char *cpu_name;
    char *cpu_plugin;
    int ram_size;
    char *bin_dir;
} config;

struct cpu_interface *cpu;

BYTE *ram;

BYTE ram_accessor(WORD address, bool read, BYTE value)
{
    if (address >= config.ram_size)
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

void load_config(char *config_name)
{
    char *temp;

    init_config("dynosaur.cfg");
    temp = get_config_string("LOGGING", "LOG_LEVEL");
    set_log_level(temp);

    if (config_name == NULL) {
        temp = get_config_string("DYNOSAUR", "DEFAULT_CPU");
        if (config.cpu_config == NULL)
            LOG_FTL("Cannot determine default configuration.\n");
        strncpy(config.cpu_config, temp, 32);
    } else {
        sprintf(config.cpu_config, "%s_CPU", config_name);
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

    config.ram_size = get_config_hex("DYNOSAUR", "RAM_SIZE");
    if (config.ram_size == 0) {
        LOG_WRN("RAM size not specified.  Assuming 0x8000.\n");
        config.ram_size = 0x8000;
    }

    config.bin_dir = get_config_string(config.cpu_config, "DIRECTORY");
    if (config.bin_dir == NULL)
        LOG_WRN("Binary directory not specified.  Will use current directory.\n");
}

void *plugin;

static void init(char *config_name)
{
    init_logging();
    
    load_config(config_name);
    shell_set_accessor(ram_accessor);

    plugin = load_plugin();
    ram = malloc(config.ram_size);
    cpu->initialize(ram_accessor);

    if (config.bin_dir != NULL) {
        if(chdir(config.bin_dir) == -1)
            LOG_ERR("Error changing to %s.\n", config.bin_dir);
        else
            LOG_INF("Changed to %s.\n", config.bin_dir);
    }

    init_video();

    shell_initialize("dynosaur");
    shell_load_dynosaur_commands();
    shell_set_loop_cb(cycle);
}

static void finalize()
{
    cpu->finalize();
    shell_finalize();
    
    finalize_video();

    free(ram);
    unload_plugin(plugin);
}

int main(int argv, char **argc)
{
    printf("Dynosaur\n");
   
    init(argc[1]);
    shell_loop();
    finalize();

    return 0;
}

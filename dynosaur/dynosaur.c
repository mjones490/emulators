#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "shell.h"
#include "plugin.h"
#include "shell_commands.h"
#include "logging.h"
#include "config.h"
#include "video.h"
#include "bus.h"
#include "dynosaur.h"

struct config_t dyn_config;
struct cpu_interface *cpu;
WORD breakpoint = 0;

static void *plugin;

void *load_plugin()
{
    void *handle;
    char *error;
    int i;

    handle = dlopen(dyn_config.cpu_plugin, RTLD_LAZY);
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

static BYTE execute_instruction()
{
    BYTE clocks;
    if (!cpu->get_halted()) {
        clocks = cpu->execute_instruction();
        if (cpu->get_PC() == breakpoint) {
            printf("Breakpoint reached.\n");
            cpu->set_halted(true);
        }
    }

    return clocks;
}

static void cycle()
{
    static int bucket = 0;
    Uint32 current_timer= SDL_GetTicks();
    static Uint32 timer;
    if (timer == 0)
        timer = current_timer;

    if (bucket > 0)
        bucket -= execute_instruction();
    
    if (current_timer > timer) {
        bucket += (current_timer - timer) * dyn_config.clock_speed;
        timer = current_timer;
    }

    refresh_video();
}

void load_config(char *config_name)
{
    char *temp;

    init_config("dynosaur.cfg");
    temp = get_config_string("LOGGING", "LOG_LEVEL");
    set_log_level(temp);

    if (config_name == NULL) {
        temp = get_config_string("DYNOSAUR", "DEFAULT_CPU");
        if (temp == NULL)
            LOG_FTL("Cannot determine default configuration.\n");
        strncpy(dyn_config.cpu_config, temp, 32);
    } else {
        sprintf(dyn_config.cpu_config, "%s_CPU", config_name);
    }
    LOG_INF("CPU configuration name is %s.\n", dyn_config.cpu_config);

    dyn_config.cpu_name = get_config_string(dyn_config.cpu_config, "NAME");
    
    if (dyn_config.cpu_name == NULL)
        LOG_ERR("No CPU name.\n");
    else
        LOG_INF("CPU Name is %s.\n", dyn_config.cpu_name);

    dyn_config.cpu_plugin = get_config_string(dyn_config.cpu_config, "PLUGIN");
    if (dyn_config.cpu_plugin == NULL)
        LOG_FTL("Cannot determine CPU plugin file name.\n");
    LOG_INF("CPU plugin file name = %s.\n", dyn_config.cpu_plugin);

    dyn_config.ram_size = get_config_hex("DYNOSAUR", "RAM_SIZE");
    if (dyn_config.ram_size == 0) {
        LOG_WRN("RAM size not specified.  Assuming 0x80.\n");
        dyn_config.ram_size = 0x80;
    }

    dyn_config.clock_speed = get_config_int(dyn_config.cpu_config, "CLOCK_SPEED");
    if (dyn_config.clock_speed == 0) {
        LOG_WRN("CPU Clock speed not configured.\n");
        dyn_config.clock_speed = 500;
    }
    LOG_INF("CPU clock speed set to %dMhZ.\n", dyn_config.clock_speed);

    dyn_config.bin_dir = get_config_string(dyn_config.cpu_config, "DIRECTORY");
    if (dyn_config.bin_dir == NULL)
        LOG_WRN("Binary directory not specified.  Will use current directory.\n");
}

static void init(char *config_name)
{
    init_logging();
        
    load_config(config_name);
    init_bus();

    shell_set_accessor(bus_accessor);

    plugin = load_plugin();
    
    cpu->initialize(bus_accessor, port_accessor);

    if (dyn_config.bin_dir != NULL) {
        if(chdir(dyn_config.bin_dir) == -1)
            LOG_ERR("Error changing to %s.\n", dyn_config.bin_dir);
        else
            LOG_INF("Changed to %s.\n", dyn_config.bin_dir);
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

    unload_plugin(plugin);
    finalize_bus();
}

int main(int argv, char **argc)
{
    printf("Dynosaur\n");
   
    init(argc[1]);
    shell_loop();
    finalize();

    return 0;
}

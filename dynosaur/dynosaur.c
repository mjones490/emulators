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

static struct {
    char cpu_config[32];
    char *cpu_name;
    char *cpu_plugin;
    int ram_size;
    char *bin_dir;
    Uint32 clock_speed;
} config;

struct cpu_interface *cpu;

BYTE *ram;

/********************************************************/
// Bus
accessor_t bus[256];

BYTE ram_accessor(WORD address, bool read, BYTE value)
{
    if (read)
        value = ram[address];
    else
        ram[address] = value;

    return value;
}

BYTE bus_accessor(WORD address, bool read, BYTE value)
{
    if (bus[hi(address)])
        return bus[hi(address)](address, read, value);

    return 0;
}

void attach_bus(accessor_t accessor, BYTE start_page, BYTE num_pages)
{
    WORD i;
    for (i = 0; i < num_pages; i++) {
        if ((i + start_page) > 255)
            break;
        bus[i + start_page] = accessor;
    }
}
/************************/
// Ports
port_accessor_t port[256];

BYTE port_accessor(BYTE port_no, bool read, BYTE value)
{
    if (port[port_no])
        return port[port_no](port_no, read, value);

    LOG_DBG("Port %02x not attached.\n", port_no);
    return 0;
}

void attach_port(port_accessor_t accessor, BYTE port_no)
{
    port[port_no] = accessor;
}

/************************/

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
        bucket += (current_timer - timer) * config.clock_speed;
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
        if (config.cpu_config == NULL)
            LOG_FTL("Cannot determine default configuration.\n");
        strncpy(config.cpu_config, temp, 32);
    } else {
        sprintf(config.cpu_config, "%s_CPU", config_name);
    }
    LOG_INF("CPU configuration name is %s.\n", config.cpu_config);

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
        LOG_WRN("RAM size not specified.  Assuming 0x80.\n");
        config.ram_size = 0x80;
    }

    config.clock_speed = get_config_int(config.cpu_config, "CLOCK_SPEED");
    if (config.clock_speed == 0) {
        LOG_WRN("CPU Clock speed not configured.\n");
        config.clock_speed = 500;
    }
    LOG_INF("CPU clock speed set to %dMhZ.\n", config.clock_speed);

    config.bin_dir = get_config_string(config.cpu_config, "DIRECTORY");
    if (config.bin_dir == NULL)
        LOG_WRN("Binary directory not specified.  Will use current directory.\n");
}

void *plugin;

static void init(char *config_name)
{
    init_logging();
    
    load_config(config_name);
    shell_set_accessor(bus_accessor);

    plugin = load_plugin();
    ram = malloc(config.ram_size * 256);
    attach_bus(ram_accessor, 0, config.ram_size);
    cpu->initialize(bus_accessor, port_accessor);

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

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
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

static void execute_instruction()
{
    if (!cpu->get_halted())
        cpu->execute_instruction();
}

static void cycle()
{
    usleep(10);

    execute_instruction();
}

int step(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->execute_instruction();
    cpu->show_registers();
    cpu->disassemble(cpu->get_PC());
    return 0;
}

int go(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->set_halted(false);
    return 0;
}

int halt(int argc, char **argv)
{
    cpu->set_halted(true);
    return 0;
}

static int disassemble(int argc, char **argv)
{
    int matched;
    int start;
    int end;
    static WORD address;
    static WORD extend = 1;

    if (1 < argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start;
            if (matched == 1) { 
                end = address;
                extend = 1;
            } else {
                extend = end - address;
            }
        }
    } else {
        end = address + extend;
    }

    do {
        address = cpu->disassemble(address);
    } while (address < end);

    return 0;
}

static int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) 
            cpu->set_register(reg_name, (WORD) tmp);
    }
    
    cpu->show_registers();
    return 0;
}

void shell_load_commands()
{
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("go", "Start CPU.", go, false);
    shell_add_command("halt", "Halt CPU", halt, false);
    shell_add_command("registers", "View/Set CPU registers", registers, false);
    shell_add_command("disassemble", "Disassemble instructions", disassemble, true);
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
    shell_load_commands();

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

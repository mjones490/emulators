#ifndef __PLUGIN_H
#define __PLUGIN_H
#include <ctype.h>
#include "types.h"
#include "shell.h"

struct shell_commands_t {
    char *name;
    char *description;
    command_func_t function;
    bool is_repeatable;
};
#define SHELL_COMMANDS_CNT(SHELL_COMMANDS) ((sizeof(SHELL_COMMANDS) / sizeof(struct shell_commands_t)))
struct cpu_interface {
    void (*initialize)(accessor_t, port_accessor_t);
    void (*finalize)();
    BYTE (*execute_instruction)();
    void (*show_registers)();
    void (*set_register)(char *, WORD);
    WORD (*disassemble)(WORD);
    int  (*get_instruction_size)(BYTE);
    void (*set_halted)(bool);
    bool (*get_halted)();
    void (*set_PC)(WORD);
    WORD (*get_PC)();
    void (*interrupt)(BYTE);
    int num_shell_commands;
    struct shell_commands_t *shell_commands;
};

#ifndef PLUGIN_INTERFACE
#define PLUGIN_INTERFACE
static struct cpu_interface *(*get_cpu_interface)();
#endif

#endif

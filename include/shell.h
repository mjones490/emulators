#ifndef __SHELL_H
#define __SHELL_H
#include <stdbool.h>
#include "types.h"

typedef int (*command_func_t)(int, char **);
void shell_loop();
void shell_initialize(char *prompt);
void shell_finalize();
command_func_t shell_add_command(char *name, char *description, 
    command_func_t function, bool is_repeatable);
void shell_set_accessor(accessor_t accessor);
void shell_set_loop_cb(void (*callback)());
BYTE shell_peek_byte(WORD address);
BYTE shell_poke_byte(WORD address, BYTE value);
void shell_print(char *string);
#endif

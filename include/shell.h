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
command_func_t shell_get_command_function(char *name);
void shell_set_accessor(accessor_t accessor);
void shell_set_loop_cb(void (*callback)());
BYTE shell_peek_byte(WORD address);
BYTE shell_poke_byte(WORD address, BYTE value);
void shell_print(char *string);
void shell_set_anonymous_command_function(command_func_t command_function);
void shell_read_key();
bool shell_check_key();
int shell_load_history(char *filename);
int shell_save_history(char *filename);

#include "shell_extended_commands.h"
#endif

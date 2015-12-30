#ifndef __COMMAND_H
#define __COMMAND_H
#include <stdbool.h>

struct command_t {
    char *name;
    char *description;
    command_func_t function; 
    bool is_repeatable;
};

struct command_t *get_command(int i);
struct command_t *find_command(char *name);
void command_initialize();
char **command_completer(const char *text, int start, int end);
void shell_free_commands();
#endif

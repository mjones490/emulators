#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include "shell.h"
#include "command.h"

struct command_t command[64];
static int num_commands = 0;

command_func_t shell_add_command(char *name, char *description, 
    command_func_t function, bool is_repeatable)
{
    command_func_t old_func = NULL;
    struct command_t *cmd = find_command(name);

    if (NULL != cmd) {
        old_func = cmd->function;
        free(cmd->name);
        free(cmd->description);
    } else {
        cmd = &command[num_commands++];
    }

    cmd->name = malloc(strlen(name) + 1);
    strcpy(cmd->name, name);

    cmd->description = malloc(strlen(description));
    strcpy(cmd->description, description);

    cmd->function = function;
    cmd->is_repeatable = is_repeatable;

    return old_func;
}

struct command_t *get_command(int i)
{
    struct command_t *cmd = NULL;
    if (i < num_commands)
        cmd = &command[i];

    return cmd;
}

struct command_t *find_command(char *name)
{
    int i = 0;
    struct command_t *cmd = NULL;
    while (NULL != (cmd = get_command(i++))) {
        if (0 == strcmp(cmd->name, name))
            break;
    }

    return cmd;
}

command_func_t shell_get_command_function(char *name)
{
    struct command_t *cmd = find_command(name);
    command_func_t func = NULL;

    if (cmd)
        func = cmd->function;

   return func;
}

struct command_t *command_generator(const char *text, int state)
{
    static int index, len;
    struct command_t *cmd;

    if (!state) {
        index = 0;
        len = strlen(text);
    }

    while ((cmd = get_command(index++))) 
        if (0 == strncmp(cmd->name, text, len))  
            break;

    return cmd;
}

char *command_name_generator(const char *text, int state)
{
    char *match = NULL;
    struct command_t *cmd = command_generator(text, state);
    if (cmd) {
        match = malloc(strlen(cmd->name) + 1);
        strcpy(match, cmd->name);
    }

    return match;
}

char **command_completer(const char *text, int start, int end)
{
    char **matches = NULL;
    if (text && start == 0)
        matches = rl_completion_matches(text, command_name_generator);
    return matches;
}

int help(int, char **);
int quit(int, char **);
int poke(int, char **);
int peek(int, char **);
int move(int, char **);
int anonymous_command(int, char **);

void command_initialize()
{
    shell_add_command("help", "Show this help.", help, false);
    shell_add_command("quit", "Quit shell.", quit, false);
    shell_add_command("peek", "Peek command.", peek, true);
    shell_add_command("poke", "Poke command.", poke, false);
    shell_add_command("move", "Move bytes.", move, false);
    shell_set_anonymous_command_function(anonymous_command);
}


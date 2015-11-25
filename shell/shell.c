#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <malloc.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "command.h"

static inline char *append_char(char *buffer, char ch)
{
    if (buffer)
        *(buffer++) = ch;
    return buffer;
}

static char *next_arg(char *string, char *buffer)
{
    bool is_quoted = false;

    while (*string && isspace(*string))
        ++string;

    if ('\"' == *string) {
        is_quoted = true;
        buffer = append_char(buffer, *(string++));
    }

    while (*string) {
        if (is_quoted) {
            if ('\"' == *string) {
                buffer = append_char(buffer, *(string++));
                break;
            }
        } else if (isspace(*string)) {
            break;
        }

        if (buffer)
            buffer = append_char(buffer, *string);
        ++string;
    }

    append_char(buffer, '\0');
    while (*string && isspace(*string))
        ++string;

    return string;
}

static int parse(char *string, char ***argvp)
{
    char *tmp;
    char *arg;
    int argc = 0;
    int i;

    tmp = string;
    while (*tmp) {
        ++argc;
        tmp = next_arg(tmp, NULL);
    }

    arg = malloc(strlen(string) + 1);
    *argvp = malloc(sizeof(char *) * argc);

    for (tmp = string, i = 0; i < argc; ++i) {
        tmp = next_arg(tmp, arg);
        (*argvp)[i] =  arg;
        arg += strlen(arg) + 1;
    }

    return argc;
}

static bool all_stop = false;
static void do_command(int argc, char **argv)
{
    static struct command_t *prev_cmd = NULL;
    struct command_t *cmd = NULL;
    
    if (0 != argc) {
        cmd = find_command(argv[0]);
        if (NULL == cmd) {
            printf("Command \"%s\" not found.\n", argv[0]);
            prev_cmd = NULL;
        }
    } else {
        cmd = prev_cmd;
    }
        
    if (NULL != cmd) {
        if (NULL != cmd->function && 
            0 != cmd->function(argc, argv))
            all_stop = true;

        if (cmd->is_repeatable)
            prev_cmd = cmd;
        else
            prev_cmd = NULL;
    } 
}

static void line_handler(char *line)
{
    char **argv = NULL;
    int argc = 0;

    if (line) {
        if (*line) {
            argc = parse(line, &argv);
            add_history(line);
        }

        do_command(argc, argv);

        if (0 != argv) {
            free(argv[0]);
            free(argv);
        }
        free(line);
    }
}

static void (*loop_callback)() = NULL;

void shell_set_loop_cb(void (*callback)())
{
    loop_callback = callback;
}

void shell_print(char* str)
{
    printf("\r%s", str);
    rl_on_new_line_with_prompt();
    rl_forced_update_display();
}

void shell_loop()
{
    fd_set fds;
    int r;
    struct timeval notime;

    while (!all_stop) {
        if (loop_callback)
            loop_callback();
        FD_ZERO(&fds);
        FD_SET(fileno(rl_instream), &fds);
        notime.tv_sec = 0;
        notime.tv_usec = 0;
        r = select(FD_SETSIZE, &fds, NULL, NULL, &notime);
        if (r < 0)
            return;
        if (FD_ISSET(fileno(rl_instream), &fds))
            rl_callback_read_char();
    }
}

void shell_initialize(char *prompt)
{
    char full_prompt[32];
    sprintf(full_prompt, "%s>", prompt);
    command_initialize();
    rl_callback_handler_install(full_prompt, line_handler);
    rl_attempted_completion_function = command_completer;
}

void shell_finalize()
{
    rl_callback_handler_remove();
}

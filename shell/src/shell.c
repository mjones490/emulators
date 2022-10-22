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

static inline char *rtrim(char *str)
{
    char *back = str + strlen(str);
    while (isspace(*--back));
    *(back + 1) = '\0';
    return str;
}

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
        if ('\\' == *string) {
            ++string;
            buffer = append_char(buffer, *(string++));
            continue;
        } else if (is_quoted) {
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

static command_func_t anonymous_command_function = NULL;
 
static void do_anonymous_command(int argc, char **argv)
{
    char **new_argv;
    int i;

    if (NULL == anonymous_command_function) { 
        printf("Command \"%s\" not found.\n", argv[0]);
    } else {    
        new_argv = malloc(sizeof(char *) * (argc + 1));
        for (i = 0; i < argc; ++i)
            new_argv[i + 1] = argv[i];

        new_argv[0] = "";
        anonymous_command_function(argc + 1, new_argv);
        free(new_argv);    
    }
}

static void do_command(int argc, char **argv)
{
    static struct command_t *prev_cmd = NULL;
    struct command_t *cmd = NULL;
    
    if (0 != argc) {
        cmd = find_command(argv[0]);
        if (NULL == cmd) {
            do_anonymous_command(argc, argv);
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
    HIST_ENTRY *he;
    HISTORY_STATE *hs;

    if (line) {
        if (*line) {
            rtrim(line);
            argc = parse(line, &argv);
            hs = history_get_history_state();
            he = history_get(hs->length);
            if (he == NULL || strcmp(line, he->line)) {
                add_history(line);
            }
        }

        rl_replace_line("", true);
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

void shell_set_anonymous_command_function(command_func_t command_function)
{
    anonymous_command_function = command_function;
}

bool shell_check_key()
{
    fd_set fds;
    int r;
    struct timeval notime;
    int i = 0;

    FD_ZERO(&fds);
    FD_SET(fileno(rl_instream), &fds);
    notime.tv_sec = 0;
    notime.tv_usec = 0;
    r = select(FD_SETSIZE, &fds, NULL, NULL, &notime);
    if (r < 0)
        return false;
    if (FD_ISSET(fileno(rl_instream), &fds))
        return true;
    return false;
}

void shell_read_key()
{
    rl_callback_read_char();
}

void shell_loop()
{
    while (!all_stop) {
        if (loop_callback)
            loop_callback();
        if (shell_check_key())
            shell_read_key();
    }
}

int shell_save_history(char *filename)
{
    return write_history(filename);
}

int shell_load_history(char *filename)
{
    int ret = read_history(filename);
    HISTORY_STATE *hs = history_get_history_state();
    history_set_pos(hs->length);
    return ret;
}

void shell_initialize(char *prompt)
{
    char full_prompt[32];
    sprintf(full_prompt, "%s>", prompt);
    command_initialize();
    rl_callback_handler_install(full_prompt, line_handler);
    rl_attempted_completion_function = command_completer;
    all_stop = false;
}

void shell_finalize()
{
    rl_callback_handler_remove();
    clear_history();
    shell_free_commands();
}

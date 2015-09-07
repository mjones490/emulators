#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shell.h"
#include "command.h"
#include "types.h"

static accessor_t shell_accessor;

void shell_set_accessor(accessor_t accessor)
{
    shell_accessor = accessor;
}

BYTE shell_peek_byte(WORD address)
{
    BYTE value = 0;
    if (NULL != shell_accessor)
        value = shell_accessor(address, true, 0x00);
    return value;

}

BYTE shell_poke_byte(WORD address, BYTE value)
{
    if (NULL != shell_accessor)
        shell_accessor(address, false, value);
    return value;
}

/**********************************************************
    Peek command
 **********************************************************/
void dump_line(WORD *address)
{
    int i;
    BYTE text[17];
    printf("%04x: ", *address);
    for (i = 0; i < 16; i++) {
        text[i] = shell_peek_byte((*address)++);
        printf("%02x ", text[i]);
        if (text[i] < 32 || text[i] > 127)
            text[i] = '.';
    }

    text[16] = 0;
    printf(" %s\n", text);
}

int peek(int argc, char **argv)
{
    int i;
    int start;
    int end;
    static int lines = 1;
    static WORD address = 0;
    int matched = 0;

    if (2 == argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start & 0xfff0;
            if (matched == 2) 
                lines = 1 + ((end - start) >> 4);
        }
    }

    if (lines < 1)
        lines = 1;

    for (i = 0; i < lines; ++i)
        dump_line(&address);

    return 0;
}

/**********************************************************
    Poke command
 **********************************************************/
int poke(int argc, char **argv)
{
    static WORD address = 0;
    unsigned int value;
    int arg_num;
    char colon;
    char *ptr;
    int matches;

    for (arg_num = 1; arg_num < argc; ++arg_num) { 
       if ('\"' == argv[arg_num][0]) {
            ptr = argv[arg_num];
            ptr++;
            while (*ptr && '\"' != *ptr)
                shell_poke_byte(address++, (BYTE) *(ptr++));
            shell_poke_byte(address++, 0);
            continue;               
        }
        
        matches = sscanf(argv[arg_num], "%04x%c", &value, &colon);
        if (2 == matches) {
            address = (WORD) value;
        } else if (1 == matches) {
            shell_poke_byte(address++, (BYTE) value);
        } else {
            printf("Bad value: %s\n", argv[arg_num]);
            break;
        }
    }

    return 0;
}


/**********************************************************
    Move command
 **********************************************************/
int move(int argc, char **argv)
{
    int start;
    int end;
    int matched;
    WORD from;
    WORD to;
    WORD src;
    WORD dst;
    WORD len;
    int i;
    
    if (3 > argc) {
        printf("Too few arguments.\n");
        goto done;
    }

    if (3 == argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (2 == matched) {
            from = (WORD) start;
            len = (WORD) end - from + 1; 
        } else {
            printf("Bad 'from' range.\n");
            goto done;
        }

        matched = sscanf(argv[2], "%4x", &start);
        if (1 == matched) {
            to = (WORD) start;
        } else {
            printf("Bad 'to' address.\n");
            goto done;
        }

        if (from < to) {
            src = from + len - 1;
            dst = to + len - 1;
        } else {
            src = from;
            dst = to;
        }

        for (i = 0; i < len; ++i) {
            shell_poke_byte(dst, shell_peek_byte(src));
            if (from < to) {
                --src;
                --dst;
            } else {
                ++src;
                ++dst;
            }
        }
    }

    printf("Moved %d bytes from $%04x to $%04x.\n", len, from, to);

done:
    return 0;
}


/**********************************************************
    Help command
 **********************************************************/
// *NOTE:
// The following shit is an attempt at trying to use readline 
// command completion functions to generate a sorted list of commands
// for help.  Will have to figure this out later.
#include <readline/readline.h>
char *command_name_generator(const char *text, int state);

int help(int argc, char **argv)
{
    struct command_t *cmd;
    char **cmd_names = rl_completion_matches("", command_name_generator);
    int i;

    for (i = 0; NULL != cmd_names[i]; ++i) {
        cmd = find_command(cmd_names[i]);
        if (NULL != cmd)
            printf("%s - %s\n", cmd->name, cmd->description);
    }
    return 0;
}

int quit(int argc, char **argv)
{
    return 1;
}


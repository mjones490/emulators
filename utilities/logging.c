#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "logging.h"

static char level_name[5][4];
static enum log_type log_level = INF;

static char out_buf[256];
static void (*output_hook)(char *str);

void print_log(enum log_type ltype, char *c_filename, const char *function, 
    int line, const char *format, ...)
{
    va_list args;
    char *buf = out_buf;   

    if (ltype <= log_level) {
        va_start(args, format);

        buf += sprintf(buf, "\r%s %s:%s:%d  ", level_name[ltype], c_filename, 
            function, line);
        buf += vsprintf(buf, format, args);

        if (NULL != output_hook)
            output_hook(out_buf);
        else
            printf("%s", out_buf);
    }

    va_end(args);
}

void set_log_level(char *log_level_name)
{
    int i;
    for (i = FTL; i <= DBG; ++i) {
        if (strncmp(log_level_name, level_name[i], 3) == 0)
            log_level = i;
    }
}

void set_log_output_hook(void (*func)(char *))
{
    output_hook = func;
}

void init_logging()
{
    output_hook = NULL;
    strncpy(level_name[FTL], "FTL", 4);
    strncpy(level_name[ERR], "ERR", 4);
    strncpy(level_name[WRN], "WRN", 4);
    strncpy(level_name[INF], "INF", 4);
    strncpy(level_name[DBG], "DBG", 4);

    log_level = INF;

}

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "logging.h"
#include "config.h"

static char level_name[5][4];
static enum log_type log_level = INF;

void print_log(enum log_type ltype, char *c_filename, const char *function, 
    int line, const char *format, ...)
{
    va_list args;

    if (ltype <= log_level) {
        va_start(args, format);

        printf("\r%s %s:%s:%d  ", level_name[ltype], c_filename, function, line);
        vprintf(format, args);
    }

    va_end(args);
}

void set_log_level()
{
    int i;
    char *cfg_level_name = get_config_string("LOGGING", "LOG_LEVEL");
    for (i = FTL; i <= DBG; ++i) {
        if (strncmp(cfg_level_name, level_name[i], 3) == 0)
            log_level = i;
    }
}

void init_logging()
{
    strncpy(level_name[FTL], "FTL", 4);
    strncpy(level_name[ERR], "ERR", 4);
    strncpy(level_name[WRN], "WRN", 4);
    strncpy(level_name[INF], "INF", 4);
    strncpy(level_name[DBG], "DBG", 4);

    log_level = INF;

}

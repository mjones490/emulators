#ifndef __LOGGING_H__
#define __LOGGING_H__

/*#define LOG_OUT(TYPE, ...) do { printf("%s %s:%s:%d  ", TYPE, __BASE_FILE__, \
    __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__); \
} while (0)
*/
enum log_type {
    FTL,
    ERR,
    WRN,
    INF,
    DBG
};

#define LOG_OUT(TYPE, ...) print_log(TYPE, __BASE_FILE__, \
    __FUNCTION__, __LINE__, __VA_ARGS__)

#define LOG_DBG(...) LOG_OUT(DBG, __VA_ARGS__)
#define LOG_INF(...) LOG_OUT(INF, __VA_ARGS__)
#define LOG_WRN(...) LOG_OUT(WRN, __VA_ARGS__)
#define LOG_ERR(...) LOG_OUT(ERR, __VA_ARGS__)
#define LOG_FTL(...) do { LOG_OUT(FTL, __VA_ARGS__); \
    exit(1); \
} while(0)

void print_log(enum log_type ltype, char *c_filename, const char *function, 
    int line, const char *format, ...);
void set_log_level();
void init_logging();
#endif

#ifndef LOG_API_H
#define LOG_API_H

#include <stdio.h>
#include <stdbool.h>

typedef enum
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_BOOT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level;

int log_init(char* log_file_path, bool log_erase, log_level log_level);

void log_close(void);

void log_msg(log_level level, const char *fmt, ...);

#endif /* LOG_API_H */

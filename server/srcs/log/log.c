#include "log_api.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_QUEUE_SIZE 1024
#define LOG_MSG_MAX_LEN 1024

typedef struct
{
    log_level level;
    char message[LOG_MSG_MAX_LEN];
} t_logEntry;

static t_logEntry *log_queue;
static int log_queue_head = 0, log_queue_tail = 0;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;
static bool log_running = true;
static pthread_t log_thread;

static FILE *m_log_fp        = NULL;
static log_level  m_log_threshold = LOG_LEVEL_INFO;

static void* log_worker_thread(void* arg)
{
    t_logEntry entry;
    FILE* out;

    (void)arg;
    while (1)
    {
        pthread_mutex_lock(&log_mutex);

        while (log_queue_head == log_queue_tail && log_running)
            pthread_cond_wait(&log_cond, &log_mutex);

        if (!log_running && log_queue_head == log_queue_tail)
        {
            pthread_mutex_unlock(&log_mutex);
            break;
        }

        entry = log_queue[log_queue_tail];
        log_queue_tail = (log_queue_tail + 1) % LOG_QUEUE_SIZE;

        pthread_mutex_unlock(&log_mutex);

        fprintf(m_log_fp, "%s", entry.message);
        fflush(m_log_fp);

        if ((entry.level <= m_log_threshold) || (entry.level == LOG_LEVEL_BOOT))
        {
            out = (entry.level == LOG_LEVEL_ERROR) ? stderr :
                        (entry.level == LOG_LEVEL_WARN) ? stderr : stdout;
            const char* color = (entry.level == LOG_LEVEL_ERROR) ? "\033[1;31m" :
                                (entry.level == LOG_LEVEL_WARN) ? "\033[1;33m" : "";
            const char* reset = "\033[0m";

            fprintf(out, "%s%s%s", color, entry.message, color[0] ? reset : "");
        }
    }

    return NULL;
}

static void get_timestamp(char *buffer, size_t buffer_size)
{
    static char cached_time_prefix[64] = {0};
    static time_t last_sec = 0;
    struct timeval tv;
    struct tm tm_info;

    gettimeofday(&tv, NULL);

    if (tv.tv_sec != last_sec)
    {
        localtime_r(&tv.tv_sec, &tm_info);

        snprintf(cached_time_prefix, sizeof(cached_time_prefix),
                 "%02d.%02d.%04d %02d:%02d:%02d",
                 tm_info.tm_mday,
                 tm_info.tm_mon + 1,
                 tm_info.tm_year + 1900,
                 tm_info.tm_hour,
                 tm_info.tm_min,
                 tm_info.tm_sec);
        last_sec = tv.tv_sec;
    }

    if (buffer_size < (strlen(cached_time_prefix) + 5))
    {
        fprintf(stderr, "Buffer size too small for timestamp\n");
        buffer[0] = '\0';
        return;
    }

    snprintf(buffer, buffer_size, "%s.%03ld", cached_time_prefix, (long)(tv.tv_usec / 1000));
}

int log_init(char* log_file_path, bool log_erase, log_level log_level)
{
    char* options;

    options = log_erase == true ? "w+" : "a";
    m_log_threshold = log_level;

    m_log_fp = fopen(log_file_path, options);
    if (!m_log_fp)
    {
        perror("fopen");
        return -1;
    }

    log_queue = malloc(LOG_QUEUE_SIZE * sizeof(t_logEntry));
    log_running = true;
    pthread_create(&log_thread, NULL, log_worker_thread, NULL);

    return 0;
}

void log_close(void)
{
    pthread_mutex_lock(&log_mutex);
    log_running = false;
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);

    pthread_join(log_thread, NULL);

    if (m_log_fp)
    {
        fclose(m_log_fp);
        m_log_fp = NULL;
    }
}

void log_msg(log_level level, const char *fmt, ...)
{
    char timestamp[90];
    char log_line[LOG_MSG_MAX_LEN];
    va_list args;
    int offset;
    int next_head;

    if (!m_log_fp) return;

    if (level > m_log_threshold && level != LOG_LEVEL_BOOT)
        return;

    get_timestamp(timestamp, sizeof(timestamp));

    va_start(args, fmt);
    offset = snprintf(log_line, sizeof(log_line), "[%s] ", timestamp);
    vsnprintf(log_line + offset, sizeof(log_line) - offset, fmt, args);
    va_end(args);

    pthread_mutex_lock(&log_mutex);
    next_head = (log_queue_head + 1) % LOG_QUEUE_SIZE;
    if (next_head != log_queue_tail)
    {
        log_queue[log_queue_head].level = level;
        strncpy(log_queue[log_queue_head].message, log_line, LOG_MSG_MAX_LEN - 1);
        log_queue[log_queue_head].message[LOG_MSG_MAX_LEN - 1] = '\0';
        log_queue_head = next_head;
        pthread_cond_signal(&log_cond);
    }
    else
    {
        /* foo */
    }

    pthread_mutex_unlock(&log_mutex);
}

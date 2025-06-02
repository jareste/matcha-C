#include <sys/signal.h>
#include <stdbool.h>
#include "../inc/error_codes.h"
#include "log/log_api.h"
#include "parse/config_file.h"
#include "server/server_api.h"
#include "router/router_api.h"
#include "mail/mail_api.h"

static bool m_die = false;

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        m_die = true;
    }
}

static int m_http_request_handler(int fd, const char *request, size_t request_len)
{    
    return router_handle_http_request(fd, request, request_len);
}

int main_loop()
{
    int ret;
    while (!m_die)
    {
        ret = server_select();
        if (ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "Error in server_select\n");
            return ERROR;
        }
    }

    server_cleanup();
    return 0;
}

int main()
{
    ssl_config ssl_config;
    log_config log_config;

    if (parse_config("config") == ERROR)
            goto error;

    parse_set_log_config(&log_config);
    log_init(log_config.LOG_FILE_PATH, log_config.LOG_ERASE, log_config.LOG_LEVEL);

    parse_set_ssl_config(&ssl_config);
    /* On failure will simply exit soooo :) */
    if (server_init(ssl_config.PORT) == ERROR)
        goto error;

    server_set_http_request_handler(m_http_request_handler);

    parse_free_config(); /* init config */

    /* if server closes us something weird could happen */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    main_loop();
    log_msg(LOG_LEVEL_INFO, "Exiting...\n");
    log_close();

    return 0;

error:
    parse_free_config();
    server_cleanup();
    return ERROR;
}

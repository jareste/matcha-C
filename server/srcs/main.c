#include <sys/signal.h>
#include <stdbool.h>
#include "../inc/error_codes.h"
#include "log/log_api.h"
#include "parse/config_file.h"
#include "server/server_api.h"
#include "router/router_api.h"
#include "mail/mail_api.h"
#include "db/db_api.h"
#include "db/tables/db_table_user.h"
#include "db/tables/db_table_tag.h"
#include "db/tables/db_table_pic.h"
#include "db/tables/db_table_visit.h"
#include "db/tables/db_table_like.h"
#include "db/tables/db_table_message.h"
#include "db/tables/db_table_notification.h"
#include "db/tables/db_table_session.h"
#include "db/db_gen.h"
#include "../third_party/cJSON/cJSON.h"

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

static int m_init_dbs(DB_ID *DB)
{
    db_config db_config;
    parse_set_db_config(&db_config);
    if (db_init(DB, db_config.DB_HOST, db_config.DB_PORT, db_config.DB_USER, db_config.DB_PASSWORD, db_config.DB_NAME) == ERROR)
        return ERROR;

    if (db_tuser_init(*DB) == ERROR)
        return ERROR;
    if (db_ttag_init(*DB) == ERROR)
        return ERROR;
    if (db_tpicture_init(*DB) == ERROR)
        return ERROR;
    if (db_tvisit_init(*DB) == ERROR)
        return ERROR;
    if (db_tlike_init(*DB) == ERROR)
        return ERROR;
    if (db_tmessage_init(*DB) == ERROR)
        return ERROR;
    if (db_tnotification_init(*DB) == ERROR)
        return ERROR;
    if (db_tsession_init(*DB) == ERROR)
        return ERROR;

    return SUCCESS;
}

int main()
{
    ssl_config ssl_config;
    log_config log_config;
    DB_ID DB;

    if (parse_config("config") == ERROR)
            goto error;

    parse_set_log_config(&log_config);
    log_init(log_config.LOG_FILE_PATH, log_config.LOG_ERASE, log_config.LOG_LEVEL);

    parse_set_ssl_config(&ssl_config);
    if (server_init(ssl_config.PORT) == ERROR)
        goto error;

    server_set_http_request_handler(m_http_request_handler);

    if (m_init_dbs(&DB) == ERROR)
        goto error;

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

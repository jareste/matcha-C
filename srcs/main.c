#include <sys/signal.h>
#include <stdbool.h>
#include "../inc/error_codes.h"
#include "log/log_api.h"
#include "parse/config_file.h"
#include "server/server_api.h"
#include "router/router_api.h"
#include "mail/mail_api.h"
#include "db/db_api.h"
#include "db/db_users.h"

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

typedef struct
{
    const char *username;
    const char *email;
    const char *password_hash;
    const char *first_name;
    const char *last_name;
    const char *gender;
    const char *orientation;
    const char *bio;
    int fame_rating;
    double gps_lat;
    double gps_lon;
    bool location_optout;
    const char *last_online;
} user_t;

static int m_foo_instert_db_users(DB_ID DB)
{
    int res;
    user_t u[6] = 
    {
        {"jareste", "jar@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"},
        {"alice", "jar1@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"},
        {"silly", "jar2@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"},
        {"jareste2", "jar3@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"},
        {"jareste3", "jar4@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"},
        {"jareste4", "jar5@jar.com", "fooo", "Joan", "Areste", "F", "BI", "gay", 2000, 0, 0, false, "today"}
    };

    for (int i = 0; i < 6; i++)
    {
        res = DB_user_insert(DB, u[i].username, u[i].email, u[i].password_hash, u[i].first_name,\
        u[i].last_name, u[i].gender, u[i].orientation, u[i].bio, u[i].fame_rating, u[i].gps_lat,\
        u[i].gps_lon, u[i].location_optout, u[i].last_online);
        if (res == ERROR)
        {
            fprintf(stderr, "FAILED SO MUUUUUUUUUUUUUCH\n");
            return ERROR;
        }
        printf("Inserted user '%s'\n", u[i].username);
    }

    res = DB_user_print_all(DB);
    if (res == ERROR)
    {
        fprintf(stderr, "FAILED SO MUUUUUUUUUUUUUCH2\n");
        return ERROR;
    }
    printf("Donete\n");

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
    /* On failure will simply exit soooo :) */
    if (server_init(ssl_config.PORT) == ERROR)
        goto error;

    server_set_http_request_handler(m_http_request_handler);

    if (db_init(&DB, "localhost", "5432", "admin", "1234qwer", "matcha_db") == ERROR)
        goto error;

    if (m_foo_instert_db_users(DB) == ERROR)
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

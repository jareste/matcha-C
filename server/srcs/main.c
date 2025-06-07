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
#include "db/db_gen.h"

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

/* TEST */
static int m_foo_instert_db_users(DB_ID db)
{
    user_t users[] = {
        {
            .username = "alice99",
            .email = "alice@alice.com",
            .password_hash = "hashed_pw_123",
            .first_name = "Alice",
            .last_name = "Anderson",
            .gender = "Female",
            .orientation = "Bisexual",
            .bio = "Hello, I'm Alice!",
            .fame_rating = 42,
            .gps_lat = 37.7749,
            .gps_lon = -122.4194,
            .location_optout = false,
            .last_online = "2025-06-03 14:22:00+00",
            .created_at = 0
        },
        {
            .username = "bob",
            .email = "alicer@ralice.com",
            .password_hash = "hashed_pw_123",
            .first_name = "Aliece",
            .last_name = "Andereson",
            .gender = "Femaledas",
            .orientation = "Bisedasxual",
            .bio = "Hello, I'm Alicdasdae!",
            .fame_rating = 422312,
            .gps_lat = 3721.7741119,
            .gps_lon = -1221.4194,
            .location_optout = false,
            .last_online = "2025-06-03 14:22:00+00",
            .created_at = 0
        },
    };
 
    for (int i = 0; i < (int)(sizeof(users) / sizeof(users[0])); i++)
    {
        if (db_tuser_insert_user(db, &users[i]) != SUCCESS)
        {
            fprintf(stderr, "ERROR: Failed to insert user '%s'\n", users[i].username);
            return ERROR;
        }
        printf("✅ Inserted user '%s'\n", users[i].username);
    }

    user_t_array *all_users = db_tuser_select_all_users(db);
    printf("\n👥 All users in the database:\n");
    for (size_t i = 0; i < all_users->count; i++)
    {
        user_t *user = all_users->users[i];
        printf("User %d: %s, Email: %s, Fame Rating: %d\n", user->id, user->username, user->email, user->fame_rating);
    }

    db_tuser_free_array(all_users);

    db_tuser_delete_user_from_pk(db, "bob");

    all_users = db_tuser_select_all_users(db);
    printf("\n👥 All users in the database:\n");
    for (size_t i = 0; i < all_users->count; i++)
    {
        user_t *user = all_users->users[i];
        printf("User %d: %s, Email: %s, Fame Rating: %d\n", user->id, user->username, user->email, user->fame_rating);
    }

    db_tuser_free_array(all_users);

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

    if (db_tuser_init(DB) == ERROR)
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

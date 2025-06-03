#include "db_users.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../inc/error_codes.h"
#include "../../inc/ft_malloc.h"

int DB_user_create_table(DB_ID db)
{
    if (db == INVALID_DB_ID)
    {
        fprintf(stderr, "[create_users_table] db is NULL\n");
        return -1;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id              SERIAL PRIMARY KEY,"
        "  username        VARCHAR(32) UNIQUE NOT NULL,"
        "  email           VARCHAR(255) UNIQUE NOT NULL,"
        "  password_hash   VARCHAR(128) NOT NULL,"
        "  first_name      VARCHAR(64),"
        "  last_name       VARCHAR(64),"
        "  gender          VARCHAR(16),"
        "  orientation     VARCHAR(16) DEFAULT 'bisexual',"
        "  bio             TEXT,"
        "  fame_rating     INTEGER DEFAULT 0,"
        "  gps_lat         DOUBLE PRECISION,"
        "  gps_lon         DOUBLE PRECISION,"
        "  location_optout BOOLEAN DEFAULT FALSE,"
        "  last_online     TIMESTAMP,"
        "  created_at      TIMESTAMP NOT NULL DEFAULT NOW()"
        ");";

    return db_execute(db, create_sql, 0, NULL);
}

PGresult *DB_user_get_user_by_uname(DB_ID db, const char *username)
{
    if ((db == INVALID_DB_ID) || !username)
    {
        fprintf(stderr, "[get_user_by_username] Invalid arguments\n");
        return NULL;
    }

    const char *sql = "SELECT * FROM users WHERE username = $1;";
    return db_query(db, sql, 1, &username);
}

PGresult *DB_user_get_all(DB_ID db)
{
    if (db == INVALID_DB_ID)
    {
        fprintf(stderr, "[get_all_users] db is NULL\n");
        return NULL;
    }

    const char *sql = "SELECT * FROM users ORDER BY id;";
    return db_query(db, sql, 0, NULL);
}

int DB_user_insert(DB_ID db,
                const char *username,
                const char *email,
                const char *password_hash,
                const char *first_name,
                const char *last_name,
                const char *gender,
                const char *orientation,
                const char *bio,
                int fame_rating,
                double gps_lat,
                double gps_lon,
                bool location_optout,
                const char *last_online)
{
    char fame_buf[16];
    char lat_buf[32];
    char lon_buf[32];
    char bool_buf[8];
    
    if ((db == INVALID_DB_ID) || !username || !email || !password_hash)
    {
        fprintf(stderr, "[insert_user] username/email/password_hash must be non-NULL\n");
        return -1;
    }

    const char *sql =
        "INSERT INTO users ("
        "  username, email, password_hash, first_name, last_name, "
        "  gender, orientation, bio, fame_rating, gps_lat, gps_lon, location_optout, last_online"
        ") VALUES ("
        "  $1, $2, $3, $4, $5, "
        "  $6, $7, $8, $9, $10, $11, $12, $13"
        ") ON CONFLICT (username) DO NOTHING;";
    
    snprintf(fame_buf, sizeof(fame_buf), "%d", fame_rating);
    snprintf(lat_buf,  sizeof(lat_buf),  "%f", gps_lat);
    snprintf(lon_buf,  sizeof(lon_buf),  "%f", gps_lon);
    snprintf(bool_buf, sizeof(bool_buf), "%s", (location_optout ? "TRUE" : "FALSE"));

    const char *paramValues[13] =
    {
        /* $1  */ username,
        /* $2  */ email,
        /* $3  */ password_hash,
        /* $4  */ first_name,       /* may be NULL */
        /* $5  */ last_name,        /* may be NULL */
        /* $6  */ gender,           /* may be NULL */
        /* $7  */ orientation,      /* may be NULL */
        /* $8  */ bio,              /* may be NULL */
        /* $9  */ fame_buf,
        /* $10 */ lat_buf,
        /* $11 */ lon_buf,
        /* $12 */ bool_buf,
        /* $13 */ last_online       /* may be NULL */
    };

    return db_execute(db, sql, 13, paramValues);
}

int DB_user_delete_by_username(DB_ID db, const char *username)
{
    if ((db == INVALID_DB_ID) || !username)
    {
        fprintf(stderr, "[delete_user_by_username] Invalid arguments\n");
        return -1;
    }

    const char *sql = "DELETE FROM users WHERE username = $1;";
    return db_execute(db, sql, 1, &username);
}

int DB_user_print_all(DB_ID DB)
{
    PGresult* res;
    int nrows;
    int row;

    res = DB_user_get_all(DB);
    if (!res)
    {
        fprintf(stderr, "db_query() failed for SELECT\n");
        return ERROR;
    }

    nrows = PQntuples(res);
    printf("=== All users (total: %d) ===\n\n", nrows);
    printf("%3s | %-10s | %-20s | %-10s %-10s | %s\n",
           "ID", "Username", "Email", "First", "Last", "Created At");
    printf("----+------------+----------------------+------------+------------+----------------------\n");

    for (row = 0; row < nrows; row++)
    {
        printf(
            "%3s | %-10s | %-20s | %-10s %-10s | %s\n",
            PQgetvalue(res, row, 0),
            PQgetvalue(res, row, 1),
            PQgetvalue(res, row, 2),
            PQgetvalue(res, row, 3),
            PQgetvalue(res, row, 4),
            PQgetvalue(res, row, 5)
        );
    }

    db_clear_result(res);
    return SUCCESS;
}

#include "db_api.h"

int main(void)
{
    // 1) Read connection params from environment
    const char *host     = "localhost"; // Default to localhost if not set
    const char *port     = "5432";
    const char *user     = "matcha_user";
    const char *password = "matcha_password";
    const char *dbname   = "matcha_db";
    if (!host || !port || !user || !password || !dbname)
    {
        fprintf(stderr, "One of PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE is not set\n");
        return 2;
    }

    char conninfo[512];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s user=%s password=%s dbname=%s",
             host, port, user, password, dbname);

    DB_ID db = db_connect(conninfo);
    if (!db)
    {
        fprintf(stderr, "db_connect() failed\n");
        return 2;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id SERIAL PRIMARY KEY,"
        "  username TEXT NOT NULL UNIQUE,"
        "  email TEXT NOT NULL UNIQUE,"
        "  first_name TEXT NOT NULL,"
        "  last_name TEXT NOT NULL,"
        "  created_at TIMESTAMP WITH TIME ZONE DEFAULT now()"
        ");";
    if (db_execute(db, create_sql, 0, NULL) != 0)
    {
        fprintf(stderr, "Failed to create table\n");
        db_close(db);
        return 2;
    }

    // 4) Data to insert
    const char *users_to_insert[][5] =
    {
        { "alice99",  "alice@example.com",  "Alice",   "Anderson", "1234" },
        { "bob42",    "bob@example.net",    "Bob",     "Bobby", "1234" },
        { "charlie7", "charlie@example.org","Charlie", "Chaplin", "1234" }
    };
    size_t n_users = sizeof(users_to_insert) / sizeof(users_to_insert[0]);

    const char *insert_sql =
        "INSERT INTO users(username, email, first_name, last_name, password_hash) "
        "VALUES($1, $2, $3, $4, $5) "
        "ON CONFLICT (username) DO NOTHING;";

    for (size_t i = 0; i < n_users; i++)
    {
        const char *params[5] =
        {
            users_to_insert[i][0],
            users_to_insert[i][1],
            users_to_insert[i][2],
            users_to_insert[i][3],
            users_to_insert[i][4]
        };
        if (db_execute(db, insert_sql, 5, params) != 0)
        {
            fprintf(stderr, "Warning: failed to insert user \"%s\"\n", params[0]);
        }
    }

    // 5) Select everything
    const char *select_sql =
        "SELECT id, username, email, first_name, last_name, created_at "
        "FROM users ORDER BY id;";
    PGresult *res = db_query(db, select_sql, 0, NULL);
    if (!res)
    {
        fprintf(stderr, "db_query() failed for SELECT\n");
        db_close(db);
        return 2;
    }

    int nrows = PQntuples(res);
    printf("=== All users (total: %d) ===\n\n", nrows);
    printf("%3s | %-10s | %-20s | %-10s %-10s | %s\n",
           "ID", "Username", "Email", "First", "Last", "Created At");
    printf("----+------------+----------------------+------------+------------+----------------------\n");

    for (int row = 0; row < nrows; row++)
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
    db_close(db);
    return 1;
}

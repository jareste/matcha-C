#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../db_gen.h"
#include "../db_api.h"
#include "db_table_user.h"
#include <string.h>

const columnDef_t m_users_cols[] =
{
    { .name = "id",            .type = "SERIAL",           .is_primary = true,  .is_unique = false, .not_null = true,  .default_val = NULL },
    { .name = "username",      .type = "VARCHAR(32)",      .is_primary = false, .is_unique = true,  .not_null = true,  .default_val = NULL },
    { .name = "email",         .type = "VARCHAR(255)",     .is_primary = false, .is_unique = true,  .not_null = true,  .default_val = NULL },
    { .name = "password_hash", .type = "VARCHAR(128)",     .is_primary = false, .is_unique = false, .not_null = true,  .default_val = NULL },
    { .name = "first_name",    .type = "VARCHAR(64)",      .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "last_name",     .type = "VARCHAR(64)",      .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "gender",        .type = "VARCHAR(16)",      .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "orientation",   .type = "VARCHAR(16)",      .is_primary = false, .is_unique = false, .not_null = false, .default_val = "'bisexual'" },
    { .name = "bio",           .type = "TEXT",             .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "fame_rating",   .type = "INTEGER",          .is_primary = false, .is_unique = false, .not_null = false, .default_val = "0" },
    { .name = "gps_lat",       .type = "DOUBLE PRECISION", .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "gps_lon",       .type = "DOUBLE PRECISION", .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "location_optout", .type = "BOOLEAN",        .is_primary = false, .is_unique = false, .not_null = false, .default_val = "FALSE" },
    { .name = "last_online",   .type = "TIMESTAMP",        .is_primary = false, .is_unique = false, .not_null = false, .default_val = NULL },
    { .name = "created_at",    .type = "TIMESTAMP NOT NULL", .is_primary = false, .is_unique = false, .not_null = true, .default_val = "NOW()" },
    { .name = "email_verified", .type = "BOOLEAN",         .is_primary = false, .is_unique = false, .not_null = false, .default_val = "FALSE" }
};

const int m_n_users_cols = sizeof(m_users_cols) / sizeof(m_users_cols[0]);

const tableSchema_t m_users_schema =
{
    .name    = "users",
    .n_cols  = m_n_users_cols,
    .columns = m_users_cols
};

int db_tuser_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_users_schema) != 0)
    {
        /* ERROR */
        return ERROR;
    }

    return SUCCESS;
}

int db_tuser_insert_user(DB_ID DB, user_t* u)
{
    char fame_buf[16];
    char lat_buf[32];
    char lon_buf[32];
    char bool_buf[8];
    
    snprintf(fame_buf, sizeof(fame_buf),  "%d",  u->fame_rating);
    snprintf(lat_buf,  sizeof(lat_buf),   "%f",  u->gps_lat);
    snprintf(lon_buf,  sizeof(lon_buf),   "%f",  u->gps_lon);
    snprintf(bool_buf, sizeof(bool_buf),  "%s",  u->location_optout ? "TRUE" : "FALSE");

   if (db_gen_insert(DB, &m_users_schema,
        /* id            */ NULL, /* Allways wanting default */
        /* username      */ u->username,
        /* email         */ u->email,
        /* password_hash */ u->password_hash,
        /* first_name    */ u->first_name,
        /* last_name     */ u->last_name,
        /* gender        */ u->gender,
        /* orientation   */ u->orientation,
        /* bio           */ u->bio,
        /* fame_rating   */ fame_buf,
        /* gps_lat       */ lat_buf,
        /* gps_lon       */ lon_buf,
        /* location_optout */ bool_buf,
        /* last_online   */ u->last_online,
        /* created_at    */ NULL, /* Allways wanting default */
        /* email_verified */ NULL /* Allways wanting default */
    ) != 0)
    {
        /* ERROR. but could be that it already exists */
    }

    return SUCCESS;
}

user_t_array* db_tuser_select_all_users(DB_ID DB)
{
    PGresult* all;
    user_t_array* result;
    user_t** users = NULL;
    int i;

    all = db_gen_select_all_from(DB, &m_users_schema);
    if (!all)
    {
        /* ERROR */
        return NULL;
    }
    int n_rows = PQntuples(all);
    if (n_rows <= 0)
    {
        /* ERROR NO USERS */
        db_clear_result(all);
        return NULL;
    }

    result = malloc(sizeof(user_t_array));
    users = malloc(n_rows * sizeof(user_t*));

    result->users = users;
    result->count = n_rows;
    result->pg_result = all;

    for (i = 0; i < n_rows; i++)
    {
        users[i] = calloc(sizeof(user_t), 1);

        users[i]->id = atoi(PQgetvalue(all, i, 0));
        users[i]->username = PQgetvalue(all, i, 1);
        users[i]->email = PQgetvalue(all, i, 2);
        users[i]->password_hash = PQgetvalue(all, i, 3);
        users[i]->first_name = PQgetvalue(all, i, 4);
        users[i]->last_name = PQgetvalue(all, i, 5);
        users[i]->gender = PQgetvalue(all, i, 6);
        users[i]->orientation = PQgetvalue(all, i, 7);
        users[i]->bio = PQgetvalue(all, i, 8);
        users[i]->fame_rating = atoi(PQgetvalue(all, i, 9));
        users[i]->gps_lat = atof(PQgetvalue(all, i, 10));
        users[i]->gps_lon = atof(PQgetvalue(all, i, 11));
        users[i]->location_optout = (strcmp(PQgetvalue(all, i, 12), "t") == 0);
        users[i]->last_online = PQgetvalue(all, i, 13);
        users[i]->created_at = db_gen_parse_timestamp(PQgetvalue(all, i, 14));
        users[i]->email_verified = (strcmp(PQgetvalue(all, i, 15), "t") == 0);
    }
    
    return result;
}

int db_tuser_free_array(user_t_array* users)
{
    size_t i;

    for (i = 0; i < users->count; i++)
        free(users->users[i]);

    free(users->users);
    db_clear_result(users->pg_result);
    free(users);

    return SUCCESS;
}

int db_tuser_delete_user_from_pk(DB_ID DB, const char* name)
{
    char* id;
    PGresult* r2;
    const char *sql = "SELECT id FROM users WHERE username = $1;";

    r2 = db_query(DB, sql, 1, &name);
    if (r2 && PQntuples(r2) == 1)
    {
        id = PQgetvalue(r2, 0, 0);

        if (db_gen_delete_by_pk(DB, &m_users_schema, id) != 0)
        {
            /* ERROR */
        }
        else
        {
            /* SUCCESS */
        }
    }
    else
    {
        /* ERROR NO USER FOUND */
    }

    if (r2) db_clear_result(r2);

    return SUCCESS;
}

int db_tuser_update_user(DB_ID DB, const user_t *u)
{
    char id_buf[16];
    char fame_buf[16];
    char lat_buf[32];
    char lon_buf[32];
    char bool_buf[8];
    int rc;

    snprintf(id_buf,   sizeof(id_buf),   "%d",   u->id);
    snprintf(fame_buf, sizeof(fame_buf), "%d",   u->fame_rating);
    snprintf(lat_buf,  sizeof(lat_buf),  "%f",   u->gps_lat);
    snprintf(lon_buf,  sizeof(lon_buf),  "%f",   u->gps_lon);
    snprintf(bool_buf, sizeof(bool_buf), "%s",   u->location_optout ? "TRUE" : "FALSE");

    rc = db_gen_update_by_pk(DB, &m_users_schema,
        /* pk_value */   id_buf,

        /* username      */ u->username,
        /* email         */ u->email,
        /* password_hash */ u->password_hash,
        /* first_name    */ u->first_name,
        /* last_name     */ u->last_name,
        /* gender        */ u->gender,
        /* orientation   */ u->orientation,
        /* bio           */ u->bio,
        /* fame_rating   */ fame_buf,
        /* gps_lat       */ lat_buf,
        /* gps_lon       */ lon_buf,
        /* location_optout */ bool_buf,
        /* last_online   */ u->last_online,
        /* created_at    */ NULL,
        /* email_verified */ u->email_verified ? "TRUE" : "FALSE"
    );

    return rc;
}

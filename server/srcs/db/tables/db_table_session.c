#include "db_table_session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Column schema for sessions */
static const columnDef_t m_sessions_cols[] =
{
    { .name="session_id", .type="VARCHAR(128)", .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="user_id",    .type="INTEGER",      .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="csrf_token", .type="VARCHAR(64)",  .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="expires_at", .type="TIMESTAMP",    .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL }
};
static const int m_n_sessions_cols = sizeof(m_sessions_cols)/sizeof(*m_sessions_cols);
static const tableSchema_t m_sessions_schema =
{
    .name    = "sessions",
    .n_cols  = m_n_sessions_cols,
    .columns = m_sessions_cols
};

/* Helper: build session_t from PGresult row */
static session_t *make_session_from_row(PGresult *res, int row)
{
    session_t *s;
    
    s = calloc(1, sizeof(*s));
    s->session_id = strdup(PQgetvalue(res, row, 0));
    s->user_id    = atoi(PQgetvalue(res, row, 1));
    s->csrf_token = strdup(PQgetvalue(res, row, 2));
    s->expires_at = db_gen_parse_timestamp(PQgetvalue(res, row, 3));
    return s;
}

int db_tsession_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_sessions_schema) != 0)
        return ERROR;

    return SUCCESS;
}

int db_tsession_insert(DB_ID DB, const char *session_id, int user_id, const char *csrf_token, time_t expires_at)
{
    int rc;
    struct tm* tm;
    char ubuf[16];
    char expbuf[32];

    if (!session_id || !csrf_token) return ERROR;
    snprintf(ubuf,   sizeof(ubuf),   "%d", user_id);
    tm = localtime(&expires_at);
    strftime(expbuf, sizeof(expbuf), "%Y-%m-%d %H:%M:%S", tm);

    rc = db_gen_insert(DB, &m_sessions_schema,
        /* session_id */ session_id,
        /* user_id    */ ubuf,
        /* csrf_token */ csrf_token,
        /* expires_at */ expbuf
    );
    return rc;
}

session_t_array* db_tsession_select_all(DB_ID DB)
{
    PGresult *res;
    session_t_array *arr;
    session_t **ss;
    int n;
    int i;
    
    res = db_gen_select_all_from(DB, &m_sessions_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ss = malloc(n * sizeof(session_t*));
    arr->sessions = ss;
    arr->count    = n;
    arr->pg_result= res;
    for (i = 0; i < n; i++)
    {
        ss[i] = make_session_from_row(res, i);
    }
    return arr;
}

session_t_array* db_tsession_select_for_user(DB_ID DB, int user_id)
{
    int n;
    int i;
    session_t_array* arr;
    session_t** ss;
    PGresult* res;
    char ubuf[16];

    snprintf(ubuf, sizeof(ubuf), "%d", user_id);
    const char *params[1] = { ubuf };

    const char *sql =
      "SELECT session_id,user_id,csrf_token,expires_at "
      "FROM sessions WHERE user_id = $1 ORDER BY expires_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ss = malloc(n * sizeof(session_t*));
    arr->sessions  = ss;
    arr->count     = n;
    arr->pg_result = res;
    for (int i = 0; i < n; i++)
    {
        ss[i] = make_session_from_row(res, i);
    }
    return arr;
}

session_t *db_tsession_select_by_id(DB_ID DB, const char *session_id)
{
    PGresult* res;
    session_t* s;

    if (!session_id) return NULL;
    const char *params[1] = { session_id };

    const char *sql =
      "SELECT session_id,user_id,csrf_token,expires_at "
      "FROM sessions WHERE session_id = $1;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    if (PQntuples(res) != 1)
    {
        PQclear(res);
        return NULL;
    }
    s = make_session_from_row(res, 0);
    PQclear(res);
    return s;
}

int db_tsession_update(DB_ID DB, const session_t *s)
{
    int rc;
    struct tm *tm;
    char ubuf[16];
    char expbuf[32];

    if (!s || !s->session_id) return ERROR;
    snprintf(ubuf, sizeof(ubuf), "%d", s->user_id);
    tm = localtime(&s->expires_at);
    strftime(expbuf, sizeof(expbuf), "%Y-%m-%d %H:%M:%S", tm);

    rc = db_gen_update_by_pk(DB, &m_sessions_schema,
        /* pk_value    */ s->session_id,
        /* user_id     */ ubuf,
        /* csrf_token  */ s->csrf_token,
        /* expires_at  */ expbuf
    );
    return rc;
}

int db_tsession_delete_by_id(DB_ID DB, const char *session_id)
{
    int rc;

    if (!session_id) return ERROR;
    rc = db_gen_delete_by_pk(DB, &m_sessions_schema, session_id);
    return rc;
}

int db_tsession_free_array(session_t_array *arr)
{
    size_t i;

    if (!arr) return ERROR;
    for (i = 0; i < arr->count; i++)
    {
        free(arr->sessions[i]->session_id);
        free(arr->sessions[i]->csrf_token);
        free(arr->sessions[i]);
    }
    free(arr->sessions);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

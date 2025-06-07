#include "db_table_message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const columnDef_t m_messages_cols[] =
{
    { .name="id",           .type="SERIAL",            .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="sender_id",    .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="recipient_id", .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="content",      .type="TEXT",              .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="sent_at",      .type="TIMESTAMP NOT NULL",.is_primary=false, .is_unique=false, .not_null=true,  .default_val="NOW()" },
    { .name="is_read",      .type="BOOLEAN",           .is_primary=false, .is_unique=false, .not_null=false, .default_val="FALSE" }
};
static const int m_n_messages_cols = sizeof(m_messages_cols)/sizeof(*m_messages_cols);
static const tableSchema_t m_messages_schema =
{
    .name    = "messages",
    .n_cols  = m_n_messages_cols,
    .columns = m_messages_cols
};

/* Helper: build a message_t from a PGresult row */
static message_t *make_message_from_row(PGresult *res, int row)
{
    message_t *m;
    
    m = calloc(1, sizeof(*m));
    m->id           = atoi(PQgetvalue(res, row, 0));
    m->sender_id    = atoi(PQgetvalue(res, row, 1));
    m->recipient_id = atoi(PQgetvalue(res, row, 2));
    m->content      = strdup(PQgetvalue(res, row, 3));
    m->sent_at      = db_gen_parse_timestamp(PQgetvalue(res, row, 4));
    m->is_read      = (strcmp(PQgetvalue(res, row, 5), "t") == 0);
    return m;
}

int db_tmessage_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_messages_schema) != 0)
        return ERROR;

    return SUCCESS;
}

int db_tmessage_insert(DB_ID DB,
                       int        sender_id,
                       int        recipient_id,
                       const char *content,
                       bool        is_read)
{
    int rc;
    char sbuf[16];
    char rbuf[16];
    char readbuf[8];

    if (!content) return ERROR;
    snprintf(sbuf,   sizeof(sbuf),   "%d", sender_id);
    snprintf(rbuf,   sizeof(rbuf),   "%d", recipient_id);
    snprintf(readbuf,sizeof(readbuf),"%s", is_read ? "TRUE":"FALSE");

    rc = db_gen_insert(DB, &m_messages_schema,
        /* id           */ NULL,
        /* sender_id    */ sbuf,
        /* recipient_id */ rbuf,
        /* content      */ content,
        /* sent_at      */ NULL,       /* DEFAULT NOW() */
        /* is_read      */ readbuf
    );
    return rc;
}

message_t_array *db_tmessage_select_all(DB_ID DB) 
{
    PGresult *res;
    message_t_array *arr;
    message_t **ms;
    int n;
    int i;
    
    res = db_gen_select_all_from(DB, &m_messages_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ms = malloc(n * sizeof(message_t*));
    arr->messages  = ms;
    arr->count     = n;
    arr->pg_result = res;

    for (i = 0; i < n; i++)
    {
        ms[i] = make_message_from_row(res, i);
    }
    return arr;
}

message_t_array *db_tmessage_select_by_sender(DB_ID DB, int sender_id)
{
    PGresult* res;
    message_t_array* arr;
    message_t** ms;
    int n;
    int i;
    char sbuf[16];
    snprintf(sbuf, sizeof(sbuf), "%d", sender_id);
    const char *params[1] = { sbuf };

    const char *sql =
      "SELECT id,sender_id,recipient_id,content,sent_at,is_read "
      "FROM messages WHERE sender_id = $1 ORDER BY sent_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ms = malloc(n * sizeof(message_t*));
    arr->messages  = ms;
    arr->count     = n;
    arr->pg_result = res;
    for (i = 0; i < n; i++)
    {
        ms[i] = make_message_from_row(res, i);
    }
    return arr;
}

message_t_array *db_tmessage_select_by_recipient(DB_ID DB, int recipient_id)
{
    PGresult* res;
    message_t_array* arr;
    message_t** ms;
    int n;
    int i;
    char rbuf[16];

    snprintf(rbuf, sizeof(rbuf), "%d", recipient_id);
    const char *params[1] = { rbuf };

    const char *sql =
      "SELECT id,sender_id,recipient_id,content,sent_at,is_read "
      "FROM messages WHERE recipient_id = $1 ORDER BY sent_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ms = malloc(n * sizeof(message_t*));
    arr->messages  = ms;
    arr->count     = n;
    arr->pg_result = res;
    for (i = 0; i < n; i++)
    {
        ms[i] = make_message_from_row(res, i);
    }
    return arr;
}

int db_tmessage_update_read_status(DB_ID DB, int message_id, bool is_read)
{
    char idbuf[16], readbuf[8];
    snprintf(idbuf,   sizeof(idbuf),   "%d", message_id);
    snprintf(readbuf, sizeof(readbuf), "%s", is_read ? "TRUE" : "FALSE");

    int rc = db_gen_update_by_pk(DB, &m_messages_schema,
        /* pk_value */ idbuf,
        /* sender_id    */ NULL,
        /* recipient_id */ NULL,
        /* content      */ NULL,
        /* sent_at      */ NULL,
        /* is_read      */ readbuf
    );
    return rc;
}

/* 7) Delete by PK */
int db_tmessage_delete_by_pk(DB_ID DB, int id)
{
    char ibuf[16];
    int rc;

    snprintf(ibuf, sizeof(ibuf), "%d", id);
    rc = db_gen_delete_by_pk(DB, &m_messages_schema, ibuf);
    return rc;
}

int db_tmessage_free_array(message_t_array *arr)
{
    if (!arr) return ERROR;
    for (size_t i = 0; i < arr->count; i++)
    {
        free(arr->messages[i]->content);
        free(arr->messages[i]);
    }
    free(arr->messages);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

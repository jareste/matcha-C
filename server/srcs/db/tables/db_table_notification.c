#include "db_table_notification.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Column schema for notifications */
static const columnDef_t m_notifications_cols[] =
{
    { .name="id",          .type="SERIAL",            .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="user_id",     .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="type",        .type="VARCHAR(32)",       .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="related_id",  .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=false, .default_val=NULL },
    { .name="created_at",  .type="TIMESTAMP NOT NULL",.is_primary=false, .is_unique=false, .not_null=true,  .default_val="NOW()" },
    { .name="is_read",     .type="BOOLEAN",           .is_primary=false, .is_unique=false, .not_null=false, .default_val="FALSE" }
};
static const int m_n_notifications_cols = sizeof(m_notifications_cols)/sizeof(*m_notifications_cols);
static const tableSchema_t m_notifications_schema =
{
    .name    = "notifications",
    .n_cols  = m_n_notifications_cols,
    .columns = m_notifications_cols
};

/* Helper: build notification_t from PGresult row */
static notification_t *make_notification_from_row(PGresult *res, int row)
{
    notification_t *n;
    
    n = calloc(1, sizeof(*n));
    n->id         = atoi(PQgetvalue(res, row, 0));
    n->user_id    = atoi(PQgetvalue(res, row, 1));
    n->type       = strdup(PQgetvalue(res, row, 2));
    {
        const char *rid = PQgetvalue(res, row, 3);
        n->related_id = (rid && *rid) ? atoi(rid) : 0;
    }
    n->created_at = db_gen_parse_timestamp(PQgetvalue(res, row, 4));
    n->is_read    = (strcmp(PQgetvalue(res, row, 5), "t") == 0);
    return n;
}

int db_tnotification_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_notifications_schema) != 0)
        return ERROR;
    
    return SUCCESS;
}

int db_tnotification_insert(DB_ID DB, int user_id, const char* type, int related_id)
{
    char ubuf[16];
    char rbuf[16];
    int rc;

    if (!type) return ERROR;
    snprintf(ubuf, sizeof(ubuf), "%d", user_id);
    snprintf(rbuf, sizeof(rbuf), "%d", related_id);

    rc = db_gen_insert(DB, &m_notifications_schema,
        /* id         */ NULL,
        /* user_id    */ ubuf,
        /* type       */ type,
        /* related_id */ rbuf,
        /* created_at */ NULL,
        /* is_read    */ NULL
    );
    return rc == 0 ? SUCCESS : ERROR;
}

notification_t_array *db_tnotification_select_all(DB_ID DB)
{
    PGresult *res;
    int n;
    int i;
    notification_t_array *arr;
    notification_t **ns;
    
    res = db_gen_select_all_from(DB, &m_notifications_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ns = malloc(n * sizeof(notification_t*));
    arr->notifications = ns;
    arr->count         = n;
    arr->pg_result     = res;

    for (i = 0; i < n; i++)
    {
        ns[i] = make_notification_from_row(res, i);
    }
    return arr;
}


notification_t_array *db_tnotification_select_for_user(DB_ID DB, int user_id)
{
    PGresult *res;
    notification_t_array *arr;
    notification_t **ns;
    int n;
    int i;
    char ubuf[16];


    snprintf(ubuf, sizeof(ubuf), "%d", user_id);
    const char *params[1] = { ubuf };

    const char *sql =
      "SELECT id,user_id,type,related_id,created_at,is_read "
      "FROM notifications WHERE user_id = $1 "
      "ORDER BY created_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ns = malloc(n * sizeof(notification_t*));
    arr->notifications = ns;
    arr->count         = n;
    arr->pg_result     = res;
    for (i = 0; i < n; i++)
    {
        ns[i] = make_notification_from_row(res, i);
    }
    return arr;
}

int db_tnotification_update_read_status(DB_ID DB, int notification_id, bool is_read)
{
    char ibuf[16];
    char readbuf[8];
    int rc;

    snprintf(ibuf,   sizeof(ibuf),   "%d", notification_id);
    snprintf(readbuf,sizeof(readbuf), "%s", is_read ? "TRUE":"FALSE");

    rc = db_gen_update_by_pk(DB, &m_notifications_schema,
        /* pk_value    */ ibuf,
        /* user_id     */ NULL,
        /* type        */ NULL,
        /* related_id  */ NULL,
        /* created_at  */ NULL,
        /* is_read     */ readbuf
    );
    return rc == 0 ? SUCCESS : ERROR;
}

/* 6) Delete by PK */
int db_tnotification_delete_by_pk(DB_ID DB, int id)
{
    char ibuf[16];
    int rc;

    snprintf(ibuf, sizeof(ibuf), "%d", id);
    rc = db_gen_delete_by_pk(DB, &m_notifications_schema, ibuf);
    return rc == 0 ? SUCCESS : ERROR;
}

int db_tnotification_free_array(notification_t_array *arr)
{
    size_t i;

    if (!arr) return ERROR;
    for (i = 0; i < arr->count; i++)
    {
        free(arr->notifications[i]->type);
        free(arr->notifications[i]);
    }
    free(arr->notifications);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

#include "db_table_pic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Column schema for pictures */
static const columnDef_t m_pictures_cols[] = {
    { .name="id",          .type="SERIAL",            .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="user_id",     .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="file_path",   .type="VARCHAR(255)",      .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="is_profile",  .type="BOOLEAN",           .is_primary=false, .is_unique=false, .not_null=false, .default_val="FALSE" },
    { .name="uploaded_at", .type="TIMESTAMP NOT NULL",.is_primary=false, .is_unique=false, .not_null=true,  .default_val="NOW()" }
};
static const int m_n_pictures_cols = sizeof(m_pictures_cols)/sizeof(*m_pictures_cols);
static const tableSchema_t m_pictures_schema = {
    .name    = "pictures",
    .n_cols  = m_n_pictures_cols,
    .columns = m_pictures_cols
};

int db_tpicture_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_pictures_schema) != 0)
        return ERROR;

    return SUCCESS;
}

int db_tpicture_insert_picture(DB_ID DB,
                               int         user_id,
                               const char *file_path,
                               bool        is_profile)
{
    if (!file_path) return ERROR;

    char uid_buf[16];
    char prof_buf[8];
    snprintf(uid_buf,   sizeof(uid_buf),   "%d",  user_id);
    snprintf(prof_buf,  sizeof(prof_buf),  "%s",  is_profile ? "TRUE" : "FALSE");

    int rc = db_gen_insert(DB, &m_pictures_schema,
        /* id          */ NULL,
        /* user_id     */ uid_buf,
        /* file_path   */ file_path,
        /* is_profile  */ prof_buf,
        /* uploaded_at */ NULL  /* DEFAULT NOW() */
    );
    return rc == 0 ? SUCCESS : ERROR;
}

/* Helper to build picture_t from PGresult row */
static picture_t *make_picture_from_row(PGresult *res, int row)
{
    picture_t* p;
    p = calloc(1, sizeof(*p));
    p->id          = atoi(PQgetvalue(res, row, 0));
    p->user_id     = atoi(PQgetvalue(res, row, 1));
    p->file_path   = strdup(PQgetvalue(res, row, 2));
    p->is_profile  = (strcmp(PQgetvalue(res, row, 3), "t") == 0);
    p->uploaded_at = db_gen_parse_timestamp(PQgetvalue(res, row, 4));
    return p;
}

picture_t_array *db_tpicture_select_all(DB_ID DB)
{
    int n;
    int i;
    picture_t_array* arr;
    picture_t** ps;
    PGresult *res;
    
    res = db_gen_select_all_from(DB, &m_pictures_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ps = malloc(n * sizeof(picture_t*));
    arr->pictures      = ps;
    arr->count         = n;
    arr->pg_result     = res;

    for (i = 0; i < n; i++)
    {
        ps[i] = make_picture_from_row(res, i);
    }
    return arr;
}

picture_t_array *db_tpicture_select_for_user(DB_ID DB, int user_id)
{
    char uid_buf[16];
    int n;
    int i;
    picture_t_array* arr;
    picture_t** ps;

    snprintf(uid_buf, sizeof(uid_buf), "%d", user_id);
    const char *params[1] = { uid_buf };
    
    const char *sql =
      "SELECT id,user_id,file_path,is_profile,uploaded_at "
      "FROM pictures WHERE user_id = $1 ORDER BY id;";

    PGresult *res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ps = malloc(n * sizeof(picture_t*));
    arr->pictures  = ps;
    arr->count     = n;
    arr->pg_result = res;

    for (i = 0; i < n; i++)
    {
        ps[i] = make_picture_from_row(res, i);
    }
    return arr;
}

int db_tpicture_select_picture_by_id(DB_ID DB, int id, picture_t **p)
{
    char id_buf[16];
    int n;

    if (!p) return ERROR;

    snprintf(id_buf, sizeof(id_buf), "%d", id);
    const char *params[1] = { id_buf };

    const char *sql =
      "SELECT id,user_id,file_path,is_profile,uploaded_at "
      "FROM pictures WHERE id = $1;";

    PGresult *res = db_query(DB, sql, 1, params);
    if (!res) return ERROR;

    n = PQntuples(res);
    if (n == 0)
    {
        PQclear(res);
        return ERROR; // No picture found
    }

    *p = make_picture_from_row(res, 0);
    PQclear(res);
    return SUCCESS;
}

int db_tpicture_select_picture_by_path(DB_ID DB, const char *file_path, picture_t **p)
{
    int n;
    PGresult* res;

    if (!p || !file_path) return ERROR;

    const char *params[1] = { file_path };
    const char *sql =
      "SELECT id,user_id,file_path,is_profile,uploaded_at "
      "FROM pictures WHERE file_path = $1;";

    res = db_query(DB, sql, 1, params);
    if (!res) return ERROR;

    n = PQntuples(res);
    if (n == 0)
    {
        PQclear(res);
        return ERROR;
    }

    *p = make_picture_from_row(res, 0);
    PQclear(res);
    return SUCCESS;
}

int db_tpicture_update_picture(DB_ID DB, const picture_t *p)
{
    char id_buf[16];
    char uid_buf[16];
    char prof_buf[8];
    int rc;

    if (!p) return ERROR;

    snprintf(id_buf,  sizeof(id_buf),  "%d", p->id);
    snprintf(uid_buf, sizeof(uid_buf), "%d", p->user_id);
    snprintf(prof_buf,sizeof(prof_buf), "%s", p->is_profile ? "TRUE" : "FALSE");

    rc = db_gen_update_by_pk(DB, &m_pictures_schema,
        /* pk_value    */ id_buf,
        /* user_id     */ uid_buf,
        /* file_path   */ p->file_path,
        /* is_profile  */ prof_buf,
        /* uploaded_at */ NULL   /* leave unchanged */
    );
    return rc;
}

int db_tpicture_delete_picture_from_pk(DB_ID DB, int id)
{
    char id_buf[16];
    int rc;
    snprintf(id_buf, sizeof(id_buf), "%d", id);
    rc = db_gen_delete_by_pk(DB, &m_pictures_schema, id_buf);
    return rc;
}

int db_tpicture_free_array(picture_t_array *arr)
{
    if (!arr) return ERROR;
    for (size_t i = 0; i < arr->count; i++)
    {
        free(arr->pictures[i]->file_path);
        free(arr->pictures[i]);
    }
    free(arr->pictures);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

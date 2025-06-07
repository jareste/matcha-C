#include "db_table_tag.h"
#include "../../../inc/ft_malloc.h"
#include <stdio.h>
#include <string.h>

static const columnDef_t m_tags_cols[] =
{
    { .name="id",   .type="SERIAL",        .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="name", .type="VARCHAR(32)",   .is_primary=false, .is_unique=true,  .not_null=true,  .default_val=NULL }
};
static const int m_n_tags_cols = sizeof(m_tags_cols)/sizeof(*m_tags_cols);
static const tableSchema_t m_tags_schema =
{
    .name   = "tags",
    .n_cols = m_n_tags_cols,
    .columns= (columnDef_t*)m_tags_cols
};

static const columnDef_t m_user_tags_cols[] =
{
    { .name="user_id", .type="INTEGER", .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="tag_id",  .type="INTEGER", .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL }
};
static const int m_n_user_tags_cols = sizeof(m_user_tags_cols)/sizeof(*m_user_tags_cols);
static const tableSchema_t m_user_tags_schema =
{
    .name   = "user_tags",
    .n_cols = m_n_user_tags_cols,
    .columns= (columnDef_t*)m_user_tags_cols
};

int db_ttag_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_tags_schema) != 0)
        return ERROR;

    if (db_gen_create_table(DB, &m_user_tags_schema) != 0)
        return ERROR;

    return SUCCESS;
}

int db_ttag_insert_tag(DB_ID DB, const char *name)
{
    int rc;

    if (!name) return ERROR;
    rc = db_gen_insert(DB, &m_tags_schema,
                           /* id   */ NULL,
                           /* name */ name);
    return rc;
}

tag_t_array *db_ttag_select_all_tags(DB_ID DB)
{
    PGresult *res;
    tag_t_array* arr;
    tag_t** ts;
    
    res = db_gen_select_all_from(DB, &m_tags_schema);
    if (!res) return NULL;
    int n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ts = malloc(n * sizeof(tag_t*));
    arr->tags      = ts;
    arr->count     = n;
    arr->pg_result = res;

    for (int i = 0; i < n; i++)
    {
        ts[i] = calloc(1, sizeof(tag_t));
        ts[i]->id   = atoi(PQgetvalue(res,i,0));
        ts[i]->name = strdup(PQgetvalue(res,i,1));
    }
    return arr;
}

int db_ttag_update_tag(DB_ID DB, int id, const char *new_name)
{
    char id_buf[16];
    int rc;

    if (!new_name) return ERROR;
    snprintf(id_buf, sizeof(id_buf), "%d", id);
    rc = db_gen_update_by_pk(DB, &m_tags_schema,
                                 /* pk = id */ id_buf,
                                 /* name */ new_name);
    return rc;
}

int db_ttag_select_tag_by_name(DB_ID DB, const char* name, tag_t** tag)
{
    const char *params[1] = { name };
    PGresult* res;
    int n;

    if (!name || !tag) return ERROR;

    const char *sql =
      "SELECT id, name "
      "FROM tags "
      "WHERE name = $1;";
    res = db_query(DB, sql, 1, params);
    if (!res) return ERROR;

    n = PQntuples(res);
    if (n == 0)
    {
        PQclear(res);
        return ERROR;
    }

    *tag = calloc(1, sizeof(tag_t));
    (*tag)->id   = atoi(PQgetvalue(res, 0, 0));
    (*tag)->name = strdup(PQgetvalue(res, 0, 1));

    PQclear(res);
    return SUCCESS;
}

int db_ttag_select_tag_by_id(DB_ID DB, int id, tag_t** tag)
{
    char id_buf[16];
    int n;
    if (!tag) return ERROR;

    snprintf(id_buf, sizeof(id_buf), "%d", id);

    const char *params[1] = { id_buf };

    const char *sql =
      "SELECT id, name "
      "FROM tags "
      "WHERE id = $1;";
    PGresult *res = db_query(DB, sql, 1, params);
    if (!res) return ERROR;

    n = PQntuples(res);
    if (n == 0)
    {
        PQclear(res);
        return ERROR;
    }

    *tag = calloc(1, sizeof(tag_t));
    (*tag)->id   = atoi(PQgetvalue(res, 0, 0));
    (*tag)->name = strdup(PQgetvalue(res, 0, 1));

    PQclear(res);
    return SUCCESS;
}

int db_ttag_delete_tag_from_pk(DB_ID DB, int id)
{
    char id_buf[16];
    snprintf(id_buf, sizeof(id_buf), "%d", id);
    int rc = db_gen_delete_by_pk(DB, &m_tags_schema, id_buf);
    return rc == 0 ? SUCCESS : ERROR;
}

int db_ttag_free_array(tag_t_array *arr)
{
    if (!arr) return ERROR;
    for (size_t i = 0; i < arr->count; i++)
    {
        free(arr->tags[i]->name);
        free(arr->tags[i]);
    }
    free(arr->tags);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

/* UTAGS */
int db_ttag_insert_user_tag(DB_ID DB, int user_id, int tag_id)
{
    char ubuf[16];
    char tbuf[16];
    int rc;

    snprintf(ubuf, sizeof(ubuf), "%d", user_id);
    snprintf(tbuf, sizeof(tbuf), "%d", tag_id);
    rc = db_gen_insert(DB, &m_user_tags_schema,
                           /* user_id */ ubuf,
                           /* tag_id  */ tbuf);
    return rc;
}

user_tag_array *db_ttag_select_tags_for_user(DB_ID DB, int user_id)
{
    char ubuf[16];
    PGresult *res;
    int n;
    int i;

    snprintf(ubuf, sizeof(ubuf), "%d", user_id);

    const char *params[1] = { ubuf };


    printf("db_ttag_select_tags_for_user: user_id = %d\n", user_id);
    const char *sql =
      "SELECT user_id, tag_id "
      "FROM user_tags "
      "WHERE user_id = $1;";
    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;

    n = PQntuples(res);
    user_tag_array *arr = malloc(sizeof(*arr));
    user_tag_t **mt = malloc(n * sizeof(user_tag_t*));
    arr->mappings  = mt;
    arr->count     = n;
    arr->pg_result = res;

    for (i = 0; i < n; i++)
    {
        mt[i] = calloc(1, sizeof(user_tag_t));
        mt[i]->user_id = atoi(PQgetvalue(res,i,0));
        mt[i]->tag_id  = atoi(PQgetvalue(res,i,1));
    }
    return arr;
}

int db_ttag_delete_user_tag(DB_ID DB, int user_id, int tag_id)
{
    const char *sql =
      "DELETE FROM user_tags WHERE user_id = $1 AND tag_id = $2;";
    char ubuf[16];
    char tbuf[16];
    int rc;

    snprintf(ubuf, sizeof(ubuf), "%d", user_id);
    snprintf(tbuf, sizeof(tbuf), "%d", tag_id);
    rc = db_execute(DB, sql, 2, (const char*[]){ubuf, tbuf});
    return rc;
}

int db_ttag_free_map_array(user_tag_array *arr)
{
    size_t i;

    if (!arr) return ERROR;
    for (i = 0; i < arr->count; i++)
        free(arr->mappings[i]);

    free(arr->mappings);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

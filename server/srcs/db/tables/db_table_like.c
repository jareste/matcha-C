#include "db_table_like.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Column schema for likes */
static const columnDef_t m_likes_cols[] =
{
    { .name="id",        .type="SERIAL",            .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="liker_id",  .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="liked_id",  .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="liked_at",  .type="TIMESTAMP NOT NULL",.is_primary=false, .is_unique=false, .not_null=true,  .default_val="NOW()" }
};
static const int m_n_likes_cols = sizeof(m_likes_cols)/sizeof(*m_likes_cols);
static const tableSchema_t m_likes_schema =
{
    .name    = "likes",
    .n_cols  = m_n_likes_cols,
    .columns = m_likes_cols
};

static like_t *make_like_from_row(PGresult *res, int row)
{
    like_t *l;
    
    l = calloc(1, sizeof(*l));
    l->id        = atoi(PQgetvalue(res, row, 0));
    l->liker_id  = atoi(PQgetvalue(res, row, 1));
    l->liked_id  = atoi(PQgetvalue(res, row, 2));
    l->liked_at  = db_gen_parse_timestamp(PQgetvalue(res, row, 3));
    return l;
}

int db_tlike_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_likes_schema) != 0)
    {
        return ERROR;
    }
    
    const char *do_block =
      "DO $$ BEGIN\n"
      "  ALTER TABLE likes "
      "ADD CONSTRAINT likes_unique_liker_liked UNIQUE(liker_id, liked_id);\n"
      "EXCEPTION WHEN duplicate_object THEN NULL; END $$;";
    db_execute(DB, do_block, 0, NULL);

    return SUCCESS;
}

int db_tlike_insert(DB_ID DB, int liker_id, int liked_id)
{
    char lbuf[16];
    char dbuf[16];
    int rc;

    snprintf(lbuf, sizeof(lbuf), "%d", liker_id);
    snprintf(dbuf, sizeof(dbuf), "%d", liked_id);

    rc = db_gen_insert(DB, &m_likes_schema,
        /* id       */ NULL,
        /* liker_id */ lbuf,
        /* liked_id */ dbuf,
        /* liked_at */ NULL
    );
    return rc;
}

like_t_array *db_tlike_select_all(DB_ID DB)
{
    PGresult *res;
    int n;
    int i;
    like_t_array *arr;
    like_t **ls;
    
    res = db_gen_select_all_from(DB, &m_likes_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    ls = malloc(n * sizeof(like_t*));
    arr->likes      = ls;
    arr->count      = n;
    arr->pg_result  = res;

    for (i = 0; i < n; i++)
    {
        ls[i] = make_like_from_row(res, i);
    }
    return arr;
}

like_t_array *db_tlike_select_by_liker(DB_ID DB, int liker_id)
{
    PGresult* res;
    like_t_array* arr;
    like_t** ls;
    int n;
    int i;
    char lbuf[16];

    snprintf(lbuf, sizeof(lbuf), "%d", liker_id);
    const char *params[1] = { lbuf };

    const char *sql =
      "SELECT id,liker_id,liked_id,liked_at "
      "FROM likes WHERE liker_id = $1 ORDER BY liked_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ls = malloc(n * sizeof(like_t*));
    arr->likes     = ls;
    arr->count     = n;
    arr->pg_result = res;
    for (i = 0; i < n; i++)
    {
        ls[i] = make_like_from_row(res, i);
    }
    return arr;
}

like_t_array *db_tlike_select_by_liked(DB_ID DB, int liked_id)
{
    PGresult* res;
    like_t_array* arr;
    like_t** ls;
    int n;
    int i;
    char dbuf[16];

    snprintf(dbuf, sizeof(dbuf), "%d", liked_id);
    const char *params[1] = { dbuf };

    const char *sql =
      "SELECT id,liker_id,liked_id,liked_at "
      "FROM likes WHERE liked_id = $1 ORDER BY liked_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    ls = malloc(n * sizeof(like_t*));
    arr->likes     = ls;
    arr->count     = n;
    arr->pg_result = res;
    for (i = 0; i < n; i++)
    {
        ls[i] = make_like_from_row(res, i);
    }
    return arr;
}

int db_tlike_delete_by_pk(DB_ID DB, int id)
{
    int rc;
    char ibuf[16];
    
    snprintf(ibuf, sizeof(ibuf), "%d", id);
    rc = db_gen_delete_by_pk(DB, &m_likes_schema, ibuf);
    return (rc == 0 ? SUCCESS : ERROR);
}

int db_tlike_free_array(like_t_array *arr)
{
    size_t i;

    if (!arr) return ERROR;
    for (i = 0; i < arr->count; i++)
    {
        free(arr->likes[i]);
    }
    free(arr->likes);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

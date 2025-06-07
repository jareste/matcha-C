#include "db_table_visit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Column schema for visits */
static const columnDef_t m_visits_cols[] = {
    { .name="id",        .type="SERIAL",            .is_primary=true,  .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="viewer_id", .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="viewed_id", .type="INTEGER",           .is_primary=false, .is_unique=false, .not_null=true,  .default_val=NULL },
    { .name="viewed_at", .type="TIMESTAMP NOT NULL",.is_primary=false, .is_unique=false, .not_null=true,  .default_val="NOW()" }
};
static const int m_n_visits_cols = sizeof(m_visits_cols)/sizeof(*m_visits_cols);
static const tableSchema_t m_visits_schema = {
    .name    = "visits",
    .n_cols  = m_n_visits_cols,
    .columns = m_visits_cols
};

/* Helper: build a visit_t from PGresult row */
static visit_t *make_visit_from_row(PGresult *res, int row)
{
    visit_t *v;

    v = calloc(1, sizeof(*v));
    v->id        = atoi(PQgetvalue(res, row, 0));
    v->viewer_id = atoi(PQgetvalue(res, row, 1));
    v->viewed_id = atoi(PQgetvalue(res, row, 2));
    v->viewed_at = db_gen_parse_timestamp(PQgetvalue(res, row, 3));
    return v;
}

int db_tvisit_init(DB_ID DB)
{
    if (db_gen_create_table(DB, &m_visits_schema) != 0)
        return ERROR;
    
    return SUCCESS;
}

int db_tvisit_insert(DB_ID DB, int viewer_id, int viewed_id)
{
    char vbuf[16];
    char dbuf[16];
    int rc;

    snprintf(vbuf, sizeof(vbuf), "%d", viewer_id);
    snprintf(dbuf, sizeof(dbuf), "%d", viewed_id);

    rc = db_gen_insert(DB, &m_visits_schema,
        /* id        */ NULL,
        /* viewer_id */ vbuf,
        /* viewed_id */ dbuf,
        /* viewed_at */ NULL  /* DEFAULT NOW() */
    );
    return rc;
}

visit_t_array *db_tvisit_select_all(DB_ID DB)
{
    PGresult *res;
    int n;
    int i;
    visit_t_array* arr;
    visit_t** vs;

    res = db_gen_select_all_from(DB, &m_visits_schema);
    if (!res) return NULL;
    n = PQntuples(res);
    if (n < 0) { PQclear(res); return NULL; }

    arr = malloc(sizeof(*arr));
    vs = malloc(n * sizeof(visit_t*));
    arr->visits      = vs;
    arr->count       = n;
    arr->pg_result   = res;

    for (i = 0; i < n; i++)
    {
        vs[i] = make_visit_from_row(res, i);
    }
    return arr;
}

visit_t_array *db_tvisit_select_by_viewer(DB_ID DB, int viewer_id)
{
    char vbuf[16];
    PGresult *res;
    int n;
    int i;
    visit_t_array* arr;
    visit_t** vs;
    snprintf(vbuf, sizeof(vbuf), "%d", viewer_id);
    const char *params[1] = { vbuf };

    const char *sql =
      "SELECT id,viewer_id,viewed_id,viewed_at "
      "FROM visits WHERE viewer_id = $1 ORDER BY viewed_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    vs = malloc(n * sizeof(visit_t*));
    arr->visits    = vs;
    arr->count     = n;
    arr->pg_result = res;

    for (i = 0; i < n; i++)
    {
        vs[i] = make_visit_from_row(res, i);
    }
    return arr;
}

visit_t_array *db_tvisit_select_by_viewed(DB_ID DB, int viewed_id)
{
    PGresult *res;
    visit_t_array *arr;
    visit_t **vs;
    char dbuf[16];
    int n;
    int i;

    snprintf(dbuf, sizeof(dbuf), "%d", viewed_id);
    const char *params[1] = { dbuf };

    const char *sql =
      "SELECT id,viewer_id,viewed_id,viewed_at "
      "FROM visits WHERE viewed_id = $1 ORDER BY viewed_at DESC;";

    res = db_query(DB, sql, 1, params);
    if (!res) return NULL;
    n = PQntuples(res);
    arr = malloc(sizeof(*arr));
    vs = malloc(n * sizeof(visit_t*));
    arr->visits    = vs;
    arr->count     = n;
    arr->pg_result = res;

    for (i = 0; i < n; i++)
    {
        vs[i] = make_visit_from_row(res, i);
    }
    return arr;
}

int db_tvisit_delete_by_pk(DB_ID DB, int id)
{
    char ibuf[16];
    int rc;

    snprintf(ibuf, sizeof(ibuf), "%d", id);
    rc = db_gen_delete_by_pk(DB, &m_visits_schema, ibuf);
    return rc;
}

int db_tvisit_free_array(visit_t_array *arr)
{
    size_t i;
    if (!arr) return ERROR;
    for (i = 0; i < arr->count; i++)
        free(arr->visits[i]);

    free(arr->visits);
    PQclear(arr->pg_result);
    free(arr);
    return SUCCESS;
}

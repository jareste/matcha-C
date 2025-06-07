#ifndef DB_TABLE_VISIT_H
#define DB_TABLE_VISIT_H

#include <stdlib.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single visit record
 */
typedef struct
{
    int    id;
    int    viewer_id;
    int    viewed_id;
    time_t viewed_at;
} visit_t;

/*
 * Array of visits
 */
typedef struct
{
    visit_t    **visits;
    size_t       count;
    PGresult    *pg_result;
} visit_t_array;

/*
 * Initialize the visits table
 */
int db_tvisit_init(DB_ID DB);

/*
 * Insert a visit (viewer → viewed). viewed_at defaults to NOW()
 */
int db_tvisit_insert(DB_ID DB, int viewer_id, int viewed_id);

/*
 * Select all visits
 */
visit_t_array *db_tvisit_select_all(DB_ID DB);

/*
 * Select all visits where viewer_id = X
 */
visit_t_array *db_tvisit_select_by_viewer(DB_ID DB, int viewer_id);

/*
 * Select all visits where viewed_id = X
 */
visit_t_array *db_tvisit_select_by_viewed(DB_ID DB, int viewed_id);

/*
 * Delete a visit by its primary key (id)
 */
int db_tvisit_delete_by_pk(DB_ID DB, int id);

/*
 * Free a visit_t_array
 */
int db_tvisit_free_array(visit_t_array *arr);

#endif /* DB_TABLE_VISIT_H */

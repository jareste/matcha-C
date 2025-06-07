#ifndef DB_TABLE_LIKE_H
#define DB_TABLE_LIKE_H

#include <stdlib.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single like record
 */
typedef struct
{
    int    id;
    int    liker_id;
    int    liked_id;
    time_t liked_at;
} like_t;

/*
 * Array of likes
 */
typedef struct
{
    like_t     **likes;
    size_t       count;
    PGresult    *pg_result;
} like_t_array;

/*
 * Initialize the likes table (and its UNIQUE constraint)
 */
int db_tlike_init(DB_ID DB);

/*
 * Insert a new like (liker → liked), liked_at defaults to NOW()
 */
int db_tlike_insert(DB_ID DB, int liker_id, int liked_id);

/*
 * Select all likes
 */
like_t_array *db_tlike_select_all(DB_ID DB);

/*
 * Select all likes by a given liker
 */
like_t_array *db_tlike_select_by_liker(DB_ID DB, int liker_id);

/*
 * Select all likes for a given liked user
 */
like_t_array *db_tlike_select_by_liked(DB_ID DB, int liked_id);

/*
 * Delete a like by its primary key (id)
 */
int db_tlike_delete_by_pk(DB_ID DB, int id);

/*
 * Free a like_t_array
 */
int db_tlike_free_array(like_t_array *arr);

#endif /* DB_TABLE_LIKE_H */

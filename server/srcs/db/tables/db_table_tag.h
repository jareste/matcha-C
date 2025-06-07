#ifndef DB_TABLE_TAG_H
#define DB_TABLE_TAG_H

#include <stdlib.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single tag
 */
typedef struct
{
    int id;
    char* name;
} tag_t;

/*
 * Array of tags
 */
typedef struct
{
    tag_t** tags;
    size_t count;
    PGresult* pg_result;
} tag_t_array;

/*
 * Single user–tag mapping
 */
typedef struct
{
    int user_id;
    int tag_id;
} user_tag_t;

/*
 * Array of user–tag mappings
 */
typedef struct
{
    user_tag_t** mappings;
    size_t count;
    PGresult* pg_result;
} user_tag_array;

/*
 * Initialize both tables:
 *   - tags
 *   - user_tags
 */
int db_ttag_init(DB_ID DB);

/*
 * CRUD for tags:
 */
int             db_ttag_insert_tag(DB_ID DB, const char *name);
tag_t_array*    db_ttag_select_all_tags(DB_ID DB);
int             db_ttag_select_tag_by_id(DB_ID DB, int id, tag_t** tag);
int             db_ttag_select_tag_by_name(DB_ID DB, const char* name, tag_t** tag);
int             db_ttag_update_tag(DB_ID DB, int id, const char* new_name);
int             db_ttag_delete_tag_from_pk(DB_ID DB, int id);
int             db_ttag_free_array(tag_t_array* arr);

/*
 * Mapping user ↔ tag:
 */
int             db_ttag_insert_user_tag(DB_ID DB, int user_id, int tag_id);
user_tag_array* db_ttag_select_tags_for_user(DB_ID DB, int user_id);
int             db_ttag_delete_user_tag(DB_ID DB, int user_id, int tag_id);
int             db_ttag_free_map_array(user_tag_array *arr);

#endif /* DB_TABLE_TAG_H */

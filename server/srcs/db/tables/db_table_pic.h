#ifndef DB_TABLE_PICTURE_H
#define DB_TABLE_PICTURE_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single picture record
 */
typedef struct
{
    int     id;
    int     user_id;
    char   *file_path;
    bool    is_profile;
    time_t  uploaded_at;
} picture_t;

/*
 * Array of pictures
 */
typedef struct
{
    picture_t  **pictures;
    size_t       count;
    PGresult    *pg_result;
} picture_t_array;

/*
 * Initialize the pictures table
 */
int db_tpicture_init(DB_ID DB);

/*
 * Insert a new picture.
 *  - user_id    : user owning this picture
 *  - file_path  : filesystem path (NOT NULL)
 *  - is_profile : whether it's the profile pic (defaults FALSE if you pass false)
 *
 * Returns SUCCESS or ERROR.
 */
int db_tpicture_insert_picture(DB_ID DB,
                               int         user_id,
                               const char *file_path,
                               bool        is_profile);

/*
 * Fetch all pictures
 */
picture_t_array *db_tpicture_select_all(DB_ID DB);

/*
 * Fetch all pictures for a given user
 */
picture_t_array *db_tpicture_select_for_user(DB_ID DB, int user_id);

int db_tpicture_select_picture_by_id(DB_ID DB, int id, picture_t **p);

int db_tpicture_select_picture_by_path(DB_ID DB, const char *file_path, picture_t **p);

/*
 * Update a picture row by its PK (id).
 * You may change user_id, file_path, or is_profile.
 * uploaded_at is skipped (remains unchanged).
 */
int db_tpicture_update_picture(DB_ID DB, const picture_t *p);

/*
 * Delete a picture by primary key (id)
 */
int db_tpicture_delete_picture_from_pk(DB_ID DB, int id);

/*
 * Free a picture_t_array and its contents
 */
int db_tpicture_free_array(picture_t_array *arr);

#endif /* DB_TABLE_PICTURE_H */

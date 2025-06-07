#ifndef DB_TABLE_SESSION_H
#define DB_TABLE_SESSION_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single session record
 */
typedef struct {
    char  *session_id;   // primary key
    int    user_id;
    char  *csrf_token;
    time_t expires_at;
} session_t;

/*
 * Array of sessions
 */
typedef struct {
    session_t  **sessions;
    size_t       count;
    PGresult    *pg_result;
} session_t_array;

/*
 * Initialize the sessions table
 */
int db_tsession_init(DB_ID DB);

/*
 * Insert a new session.
 *   - session_id : VARCHAR(128) primary key
 *   - user_id    : user owning the session
 *   - csrf_token : VARCHAR(64) not null
 *   - expires_at : timestamp when it expires
 */
int db_tsession_insert(DB_ID DB,
                       const char *session_id,
                       int         user_id,
                       const char *csrf_token,
                       time_t      expires_at);

/*
 * Select all sessions
 */
session_t_array *db_tsession_select_all(DB_ID DB);

/*
 * Select sessions for a given user
 */
session_t_array *db_tsession_select_for_user(DB_ID DB, int user_id);

/*
 * Select one session by its ID
 */
session_t *db_tsession_select_by_id(DB_ID DB, const char *session_id);

/*
 * Update a session’s expiration and/or CSRF token by session_id
 */
int db_tsession_update(DB_ID DB,
                       const session_t *s);

/*
 * Delete a session by its ID
 */
int db_tsession_delete_by_id(DB_ID DB, const char *session_id);

/*
 * Free a session_t_array
 */
int db_tsession_free_array(session_t_array *arr);

#endif /* DB_TABLE_SESSION_H */

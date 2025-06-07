#ifndef DB_TABLE_NOTIFICATION_H
#define DB_TABLE_NOTIFICATION_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single notification record
 */
typedef struct
{
    int     id;
    int     user_id;
    char   *type; /* 'visit', 'like', 'match', 'message', 'unlike', etc. */
    int     related_id; /* e.g. visits.id, likes.id, messages.id */
    time_t  created_at;
    bool    is_read;
} notification_t;

/*
 * Array of notifications
 */
typedef struct
{
    notification_t **notifications;
    size_t           count;
    PGresult        *pg_result;
} notification_t_array;

/*
 * Initialize the notifications table
 */
int db_tnotification_init(DB_ID DB);

/*
 * Insert a notification.
 *   - user_id    : who receives
 *   - type       : notification type (NOT NULL)
 *   - related_id : e.g. visits.id, likes.id, messages.id
 *
 * created_at defaults to NOW(), is_read defaults to FALSE.
 */
int db_tnotification_insert(DB_ID DB,
                            int        user_id,
                            const char *type,
                            int        related_id);

/*
 * Select all notifications
 */
notification_t_array *db_tnotification_select_all(DB_ID DB);

/*
 * Select notifications for a given user
 */
notification_t_array *db_tnotification_select_for_user(DB_ID DB, int user_id);

/*
 * Mark notification read/unread by its ID
 */
int db_tnotification_update_read_status(DB_ID DB,
                                         int notification_id,
                                         bool is_read);

/*
 * Delete a notification by PK
 */
int db_tnotification_delete_by_pk(DB_ID DB, int id);

/*
 * Free a notification_t_array
 */
int db_tnotification_free_array(notification_t_array *arr);

#endif /* DB_TABLE_NOTIFICATION_H */

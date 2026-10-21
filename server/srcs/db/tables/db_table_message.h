#ifndef DB_TABLE_MESSAGE_H
#define DB_TABLE_MESSAGE_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <libpq-fe.h>
#include "../db_gen.h"
#include "../db_api.h"
#include "../../../inc/error_codes.h"

/*
 * Single message record
 */
typedef struct
{
    int     id;
    int     sender_id;
    int     recipient_id;
    char   *content;
    time_t  sent_at;
    bool    is_read;
} message_t;

/*
 * Array of messages
 */
typedef struct
{
    message_t **messages;
    size_t      count;
    PGresult   *pg_result;
} message_t_array;

/*
 * Initialize the messages table
 */
int db_tmessage_init(DB_ID DB);

/*
 * Insert a new message.
 *   - sender_id    : ID of the sender
 *   - recipient_id : ID of the recipient
 *   - content      : message text (NOT NULL)
 *   - is_read      : initial read status (usually false)
 *
 * Returns SUCCESS or ERROR.
 */
int db_tmessage_insert(DB_ID DB,
                       int        sender_id,
                       int        recipient_id,
                       const char *content,
                       bool        is_read);

/*
 * Select all messages
 */
message_t_array *db_tmessage_select_all(DB_ID DB);

/*
 * Select messages sent by a given user
 */
message_t_array *db_tmessage_select_by_sender(DB_ID DB, int sender_id);

/*
 * Select messages received by a given user
 */
message_t_array *db_tmessage_select_by_recipient(DB_ID DB, int recipient_id);

/*
 * Select messages between a sender and a recipient
 */
message_t_array* db_tmessage_select_by_sender_and_recipient(DB_ID DB, int sender_id, int recipient_id);

/*
 * Select messages in a specific chat by chat ID
 */
message_t_array* db_tmessage_select_by_chat_id(DB_ID DB, int chat_id);

/*
 * Print a message_t
 */
void db_tmessage_print(message_t *m);

/*
 * Print a message_t_array
 */
void db_tmessage_print_array(message_t_array *arr);

/*
 * Mark a message as read (or unread) by its ID
 */
int db_tmessage_update_read_status(DB_ID DB, int message_id, bool is_read);

/*
 * Delete a message by its primary key (id)
 */
int db_tmessage_delete_by_pk(DB_ID DB, int id);

/*
 * Free a message_t_array and its contents
 */
int db_tmessage_free_array(message_t_array *arr);

#endif /* DB_TABLE_MESSAGE_H */

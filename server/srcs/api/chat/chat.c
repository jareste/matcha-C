#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_gen.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_user.h"
#include "../../db/tables/db_table_message.h"
#include "../../log/log_api.h"
#include <stdio.h>
#include <unistd.h>

DB_ID get_db_id();

/* Get all user chats */
void chat_get_u_chats(void* req_ctx, void* user_data)
{
    /* may check all user matches and just return them all. */
    http_request_ctx_t* ctx = (http_request_ctx_t*)req_ctx;
    char* response = "{\"message\": \"Hello from chat!\"}";

    (void)user_data; // Unused parameter
    
    log_msg(LOG_LEVEL_DEBUG, "Chat message request from fd=%d\n", ctx->fd);
    
    router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);
}

/* Get user specific chat */
void chat_get_u_chat(void* req_ctx, void* user_data)
{
    /* given a uid, return all the messages sent on that specific chat. */
    http_request_ctx_t* ctx = (http_request_ctx_t*)req_ctx;
    message_t* msg;
    cJSON* json_response;
    cJSON* json_messages;
    cJSON* json_msg;
    message_t_array* messages;
    size_t i;
    char* response;
    char timebuf[26];

    (void)user_data;

    messages = db_tmessage_select_by_sender_and_recipient(get_db_id(), ctx->uid, 2);
    if (!messages)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to retrieve messages from database\n");
        router_http_generate_response(ctx->fd, CODE_404_NOT_FOUND, "{\"error\": \"Requested chat not found\"}", NULL);
        return;
    }
    log_msg(LOG_LEVEL_DEBUG, "Retrieved %zu messages from database\n", messages->count);
    json_response = cJSON_CreateObject();
    json_messages = cJSON_CreateArray();

    for (i = 0; i < messages->count; i++)
    {
        msg = messages->messages[i];
        json_msg = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_msg, "id", msg->id);
        cJSON_AddNumberToObject(json_msg, "sender_id", msg->sender_id);
        cJSON_AddNumberToObject(json_msg, "recipient_id", msg->recipient_id);
        cJSON_AddStringToObject(json_msg, "content", msg->content);
        cJSON_AddBoolToObject(json_msg, "is_read", msg->is_read);
        ctime_r(&msg->sent_at, timebuf);
        timebuf[24] = '\0';
        cJSON_AddStringToObject(json_msg, "timestamp", timebuf);
        cJSON_AddItemToArray(json_messages, json_msg);
    }

    cJSON_AddItemToObject(json_response, "messages", json_messages);

    response = cJSON_PrintUnformatted(json_response);

    router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);

    cJSON_Delete(json_response);
    free(response);
    db_tmessage_free_array(messages);

    log_msg(LOG_LEVEL_DEBUG, "Chat details request from fd=%d\n", ctx->fd);
}

void chat_send_msg(void* req_ctx, void* user_data)
{
    /* send msg to the active chat. */
    sio_request_ctx_t* ctx;
    int rc;

    ctx = (sio_request_ctx_t*)req_ctx;
    
    (void)user_data; // Unused parameter
    log_msg(LOG_LEVEL_DEBUG, "************************************************\n");

    log_msg(LOG_LEVEL_DEBUG, "Chat message send request from fd=%d\n", ctx->fd);
    log_msg(LOG_LEVEL_DEBUG, "User ID: %d\n", ctx->uid);
    log_msg(LOG_LEVEL_DEBUG, "Event: %s\n", ctx->event);
    log_msg(LOG_LEVEL_DEBUG, "Payload: %.*s\n", (int)ctx->payload_len, ctx->payload);
    log_msg(LOG_LEVEL_DEBUG, "************************************************\n");

    rc = db_tmessage_insert(get_db_id(), ctx->uid, 2, (const char*)ctx->payload, false);
    if (rc != SUCCESS)
        log_msg(LOG_LEVEL_ERROR, "Failed to insert message into database\n");
    else
        log_msg(LOG_LEVEL_DEBUG, "Message inserted uid[%d] tarid[%d]. \n", ctx->uid, 2);

}

void chat_init()
{
    router_http_add("/api/chat", chat_get_u_chats, NULL, AUTH_REQUIRED);
    router_http_add("/api/chat/2", chat_get_u_chat, NULL, AUTH_REQUIRED);
    router_sio_add("send", chat_send_msg, NULL, FLAG_NONE);
    
    log_msg(LOG_LEVEL_BOOT, "Chat API initialized\n");
}


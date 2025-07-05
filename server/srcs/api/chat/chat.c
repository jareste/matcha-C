#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_gen.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_user.h"
#include "../../log/log_api.h"
#include <stdio.h>
#include <unistd.h>

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
    char* response = "{\"message\": \"Chat details for user!\"}";
    
    (void)user_data; // Unused parameter
    log_msg(LOG_LEVEL_DEBUG, "Chat details request from fd=%d\n", ctx->fd);
    
    router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);
}

void chat_send_msg(void* req_ctx, void* user_data)
{
    /* send msg to the active chat. */
    sio_request_ctx_t* ctx;

    ctx = (sio_request_ctx_t*)req_ctx;
    // char* response = "{\"message\": \"Message sent successfully!\"}";
    
    (void)user_data; // Unused parameter
    log_msg(LOG_LEVEL_DEBUG, "************************************************\n");

    log_msg(LOG_LEVEL_DEBUG, "Chat message send request from fd=%d\n", ctx->fd);
    log_msg(LOG_LEVEL_DEBUG, "User ID: %d\n", ctx->uid);
    log_msg(LOG_LEVEL_DEBUG, "Event: %s\n", ctx->event);
    log_msg(LOG_LEVEL_DEBUG, "Payload: %.*s\n", (int)ctx->payload_len, ctx->payload);
    log_msg(LOG_LEVEL_DEBUG, "************************************************\n");
    
    // router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);
}

void chat_init()
{
    router_http_add("/api/chat", chat_get_u_chats, NULL, AUTH_REQUIRED);
    router_http_add("/api/chat/:chat_id", chat_get_u_chat, NULL, AUTH_REQUIRED);
    router_sio_add("send", chat_send_msg, NULL, FLAG_NONE);
    
    log_msg(LOG_LEVEL_BOOT, "Chat API initialized\n");
}


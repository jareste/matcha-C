#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_gen.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_user.h"
#include "../../db/tables/db_table_tag.h"
#include "../../db/tables/db_table_pic.h"
#include "../../log/log_api.h"
#include <stdio.h>
#include <unistd.h>

DB_ID get_db_id();

void tags_insert(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    cJSON* json_body;
    cJSON* json_tags;
    cJSON* tag_token;
    cJSON* json_response;
    char* response;

    if (!ctx)
        return;

    (void)user_data;

    ASSERT(ctx->parsed_request.body);

    log_msg(LOG_LEVEL_DEBUG, "Tags insert body [%s]\n", ctx->parsed_request.body);

    json_body = cJSON_Parse(ctx->parsed_request.body);
    if (!json_body)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse JSON body\n");
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST, "{\"error\": \"Invalid JSON body\"}", NULL);
        return;
    }

    json_tags = cJSON_GetObjectItemCaseSensitive(json_body, "tags");
    if (!cJSON_IsArray(json_tags))
    {
        log_msg(LOG_LEVEL_ERROR, "Invalid or missing 'tags' in request body\n");
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST, "{\"error\": \"Invalid or missing 'tags' in request body\"}", NULL);
        cJSON_Delete(json_body);
        return;
    }

    log_msg(LOG_LEVEL_DEBUG, "Extracted tags from body\n");

    cJSON_ArrayForEach(tag_token, json_tags)
    {
        if (cJSON_IsNumber(tag_token))
        {
            int tag_id = tag_token->valueint;
            if (tag_id > 0)
            {
                if (db_ttag_insert_user_tag(get_db_id(), ctx->uid, tag_id) != 0)
                {
                    log_msg(LOG_LEVEL_ERROR, "Failed to insert tag ID %d for user ID %d\n", tag_id, ctx->uid);
                }
                else
                {
                    log_msg(LOG_LEVEL_DEBUG, "Inserted tag ID %d for user ID %d\n", tag_id, ctx->uid);
                }
            }
        }
        else
        {
            log_msg(LOG_LEVEL_ERROR, "Invalid tag format in array\n");
        }
    }

    json_response = cJSON_CreateObject();
    cJSON_AddStringToObject(json_response, "message", "Tags successfully added");
    response = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_response);

    log_msg(LOG_LEVEL_DEBUG, "Sent response:\n%s\n", response);
    router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);
    free(response);

    cJSON_Delete(json_body);
}

void api_tags_init()
{
    router_http_add("/api/tags/insert", tags_insert, NULL, AUTH_REQUIRED);
}

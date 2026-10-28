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
    cJSON* json_tags;
    cJSON* tag_token;
    cJSON* json_response;
    char* response;

    if (!ctx)
        return;

    (void)user_data;

    if (!ctx_require_array(ctx, "tags", &json_tags))
        return;

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
    router_http_generate_response(ctx->fd, CODE_200_OK, response, ctx->origin);
    free(response);
}

void api_tags_init()
{
    router_http_add("/api/tags/insert", tags_insert, NULL, AUTH_REQUIRED | M_POST | BODY_JSON);
}

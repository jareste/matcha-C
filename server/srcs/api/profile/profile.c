#include "api_profile.h"
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

DB_ID get_db_id();

#define EXTRACT_JSON_STRING(json, field_name, target_var) \
    do { \
        cJSON* item = cJSON_GetObjectItemCaseSensitive(json, field_name); \
        target_var = (item && cJSON_IsString(item)) ? item->valuestring : NULL; \
    } while(0)

#define UPDATE_USER_FIELD(user, field, new_value) \
    do { \
        if (new_value) { \
            if (user->field) free(user->field); \
            user->field = strdup(new_value); \
        } \
    } while(0)

void profile_update_user(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    user_t* existing_user = NULL;
    int ret;
    cJSON* json;
    char* json_str;
    char* json_token;

    (void)user_data;

    if (!ctx)
        return;

    log_msg(LOG_LEVEL_DEBUG, "Profile update request from fd=%d\n", ctx->fd);

    /* Extract user data from the context */
    json_str = ctx->parsed_request.body;
    if (!json_str)
    {
        log_msg(LOG_LEVEL_ERROR, "No JSON body provided for profile update\n");
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"No data provided\"}", NULL);
        return;
    }

    json = cJSON_Parse(json_str);
    if (!json)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse JSON body for profile update\n");
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Invalid JSON\"}", NULL);
        return;
    }

    log_msg(LOG_LEVEL_DEBUG, "Parsed JSON for profile update: %s\n", json_str);
    
    if (db_select_user_by_id(get_db_id(), ctx->uid, &existing_user) != SUCCESS || !existing_user)
    {
        log_msg(LOG_LEVEL_ERROR, "User with ID %d not found for profile update\n", ctx->uid);
        router_http_generate_response(ctx->fd, CODE_404_NOT_FOUND,
                                      "{\"error\":\"User not found\"}", NULL);
        cJSON_Delete(json);
        return;
    }

    log_msg(LOG_LEVEL_DEBUG, "Updating profile for user ID %d\n", existing_user->id);

    /* Update user fields if provided */
    EXTRACT_JSON_STRING(json, "first_name", json_token);
    UPDATE_USER_FIELD(existing_user, first_name, json_token);
    
    EXTRACT_JSON_STRING(json, "last_name", json_token);
    UPDATE_USER_FIELD(existing_user, last_name, json_token);
    
    EXTRACT_JSON_STRING(json, "email", json_token);
    UPDATE_USER_FIELD(existing_user, email, json_token);
    
    EXTRACT_JSON_STRING(json, "bio", json_token);
    UPDATE_USER_FIELD(existing_user, bio, json_token);

    ret = db_tuser_update_user(get_db_id(), existing_user);
    if (ret == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to update user ID %d in database\n", existing_user->id);
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
                                      "{\"error\":\"Failed to update profile\"}", NULL);
        db_tuser_free_user(existing_user);
        cJSON_Delete(json);
        return;
    }

    log_msg(LOG_LEVEL_DEBUG, "Profile for user ID %d updated successfully\n", existing_user->id);

    router_http_generate_response(ctx->fd, CODE_200_OK,
                                  "{\"success\":true,\"message\":\"Profile updated successfully\"}", NULL);
    
    db_tuser_free_user(existing_user);
    cJSON_Delete(json);
}

void profile_get_user(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    char* id_str;
    int user_id;
    user_t* user = NULL;
    char* response;
    cJSON* json_response;
    cJSON* json_tags;
    cJSON* json_pics;
    tag_t_array* tags;
    picture_t_array* pics;

    if (!ctx)
        return;

    (void)user_data;

    if (ctx->query)
    {
        id_str = strstr(ctx->query, "id=");
        if (id_str)
        {
            id_str += 3;
            user_id = atoi(id_str);
            log_msg(LOG_LEVEL_DEBUG, "Extracted user ID: %d\n", user_id);
        }
        else
        {
            user_id = ctx->uid;
        }
    }
    else
    {
        user_id = ctx->uid;
    }

    log_msg(LOG_LEVEL_DEBUG, "User [%d][%s] requested profile of user ID %d\n", ctx->uid, ctx->username, user_id);

    db_select_user_by_id(get_db_id(), user_id, &user);
    if (!user)
    {
        log_msg(LOG_LEVEL_ERROR, "User with ID %d not found\n", user_id);
        router_http_generate_response(ctx->fd, CODE_404_NOT_FOUND,
                                      "{\"error\":\"User not found\"}", NULL);
        return;
    }

    json_response = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_response, "id", user->id);
    cJSON_AddStringToObject(json_response, "username", user->username);
    cJSON_AddStringToObject(json_response, "first_name", user->first_name);
    cJSON_AddStringToObject(json_response, "last_name", user->last_name);
    cJSON_AddStringToObject(json_response, "email", user->email);
    cJSON_AddStringToObject(json_response, "bio", user->bio ? user->bio : "");
    cJSON_AddStringToObject(json_response, "location", "secretito");
    cJSON_AddNumberToObject(json_response, "fame_rating", user->fame_rating);
    cJSON_AddStringToObject(json_response, "last_online", user->last_online ? user->last_online : "never");
    cJSON_AddBoolToObject(json_response, "email_verified", user->email_verified); /* TODO delete */

    tags = db_ttag_select_tag_objects_for_user(get_db_id(), user->id);
    if (tags && tags->count > 0)
    {
        json_tags = cJSON_CreateArray();
        for (size_t i = 0; i < tags->count; i++)
        {
            cJSON_AddItemToArray(json_tags, cJSON_CreateString(tags->tags[i]->name));
        }
        cJSON_AddItemToObject(json_response, "tags", json_tags);
    }
    else
    {
        cJSON_AddItemToObject(json_response, "tags", cJSON_CreateArray());
    }

    db_ttag_free_array(tags);

    pics = db_tpicture_select_for_user(get_db_id(), user->id);
    if (pics && pics->count > 0)
    {
        json_pics = cJSON_CreateArray();
        for (size_t i = 0; i < pics->count; i++)
        {
            cJSON_AddItemToArray(json_pics, cJSON_CreateString(pics->pictures[i]->file_path));
        }
        cJSON_AddItemToObject(json_response, "pictures", json_pics);
    }
    else
    {
        cJSON_AddItemToObject(json_response, "pictures", cJSON_CreateArray());
    }

    db_tpicture_free_array(pics);

    response = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_response);
    db_tuser_free_user(user);
    log_msg(LOG_LEVEL_DEBUG, "Sent profile:\n%s\n", response);
    router_http_generate_response(ctx->fd, CODE_200_OK, response, NULL);
    free(response);

    return;
}

void api_profile_init()
{
    router_http_add("/api/profile/get", profile_get_user, NULL, AUTH_REQUIRED);
    router_http_add("/api/profile/update", profile_update_user, NULL, AUTH_REQUIRED);
}

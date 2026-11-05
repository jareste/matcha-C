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

#define MAX_TAGS_PER_REQUEST 32

static void m_respond_json(http_request_ctx_t* ctx, HTTP_response_code_t code, const char* body)
{
    router_http_generate_response(ctx->fd, code, body, ctx->origin);
}

static cJSON* m_tags_to_json_array(const tag_t_array* tags)
{
    cJSON* arr;
    cJSON* item;
    size_t i;

    arr = cJSON_CreateArray();
    if (!tags)
        return arr;

    for (i = 0; i < tags->count; i++)
    {
        item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", tags->tags[i]->id);
        cJSON_AddStringToObject(item, "name", tags->tags[i]->name);
        cJSON_AddItemToArray(arr, item);
    }
    return arr;
}

static void m_respond_with_user_tags(http_request_ctx_t* ctx,
                                     const char* count_key, int count,
                                     const char* skipped_key, int skipped)
{
    tag_t_array* mine;
    cJSON* json_response;
    char* response;

    mine = db_ttag_select_tag_objects_for_user(get_db_id(), ctx->uid);

    json_response = cJSON_CreateObject();
    cJSON_AddBoolToObject(json_response, "success", true);
    cJSON_AddNumberToObject(json_response, count_key, count);
    cJSON_AddNumberToObject(json_response, skipped_key, skipped);
    cJSON_AddItemToObject(json_response, "tags", m_tags_to_json_array(mine));

    response = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_response);
    if (mine)
        db_ttag_free_array(mine);

    m_respond_json(ctx, CODE_200_OK, response);
    free(response);
}

static bool m_catalogue_has(const tag_t_array* catalogue, int id)
{
    size_t i;

    for (i = 0; i < catalogue->count; i++)
    {
        if (catalogue->tags[i]->id == id)
            return true;
    }
    return false;
}

static bool m_user_has(const tag_t_array* mine, int id)
{
    size_t i;

    if (!mine)
        return false;

    for (i = 0; i < mine->count; i++)
    {
        if (mine->tags[i]->id == id)
            return true;
    }
    return false;
}

static int m_tag_ids_from_body(http_request_ctx_t* ctx, const tag_t_array* catalogue,
                               int* out_ids, size_t* out_count)
{
    cJSON* json_tags;
    cJSON* token;
    char err[128];
    int n_given;
    int id;
    size_t count = 0;
    size_t i;

    if (!ctx_require_array(ctx, "tags", &json_tags))
        return ERROR;

    n_given = cJSON_GetArraySize(json_tags);
    if (n_given <= 0)
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"tags must not be empty\"}");
        return ERROR;
    }

    if (n_given > MAX_TAGS_PER_REQUEST)
    {
        snprintf(err, sizeof(err),
                 "{\"error\":\"tags takes at most %d ids\"}", MAX_TAGS_PER_REQUEST);
        m_respond_json(ctx, CODE_400_BAD_REQUEST, err);
        return ERROR;
    }

    cJSON_ArrayForEach(token, json_tags)
    {
        if (!cJSON_IsNumber(token) || token->valuedouble != (double)token->valueint
            || token->valueint <= 0)
        {
            m_respond_json(ctx, CODE_400_BAD_REQUEST,
                "{\"error\":\"tags must contain positive integer ids\"}");
            return ERROR;
        }

        id = token->valueint;

        if (!m_catalogue_has(catalogue, id))
        {
            snprintf(err, sizeof(err), "{\"error\":\"Unknown tag id %d\"}", id);
            m_respond_json(ctx, CODE_400_BAD_REQUEST, err);
            return ERROR;
        }

        for (i = 0; i < count; i++)
        {
            if (out_ids[i] == id)
                break;
        }
        if (i == count)
            out_ids[count++] = id;
    }

    *out_count = count;
    return SUCCESS;
}

void tags_get(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    tag_t_array* tags;
    cJSON* json_response;
    char* response;

    if (!ctx)
        return;

    (void)user_data;

    tags = db_ttag_select_all_tags(get_db_id());
    if (!tags)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to retrieve tags from database\n");
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
                       "{\"error\":\"Failed to retrieve tags\"}");
        return;
    }

    json_response = cJSON_CreateObject();
    cJSON_AddItemToObject(json_response, "tags", m_tags_to_json_array(tags));
    response = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_response);
    db_ttag_free_array(tags);

    m_respond_json(ctx, CODE_200_OK, response);
    free(response);
}

void tags_insert(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    tag_t_array* catalogue;
    tag_t_array* mine;
    int ids[MAX_TAGS_PER_REQUEST];
    size_t count = 0;
    size_t i;
    int added = 0;
    int already = 0;
    bool failed = false;

    if (!ctx)
        return;

    (void)user_data;

    catalogue = db_ttag_select_all_tags(get_db_id());
    if (!catalogue)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to retrieve tags from database\n");
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
                       "{\"error\":\"Failed to retrieve tags\"}");
        return;
    }

    if (m_tag_ids_from_body(ctx, catalogue, ids, &count) != SUCCESS)
    {
        db_ttag_free_array(catalogue);
        return;
    }

    /* NULL means the user holds no tag yet, which the select cannot tell from
     * a failure so either way there is nothing to skip as already present.
     */
    mine = db_ttag_select_tag_objects_for_user(get_db_id(), ctx->uid);

    for (i = 0; i < count; i++)
    {
        /* Asking for a tag the user already has is not an error: the request
         * describes a wanted state, so it stays idempotent.
         */
        if (m_user_has(mine, ids[i]))
        {
            already++;
            continue;
        }

        if (db_ttag_insert_user_tag(get_db_id(), ctx->uid, ids[i]) != 0)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to insert tag %d for uid=%d\n", ids[i], ctx->uid);
            failed = true;
            break;
        }
        added++;
    }

    if (mine)
        db_ttag_free_array(mine);
    db_ttag_free_array(catalogue);

    if (failed)
    {
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
                       "{\"error\":\"Failed to add tags\"}");
        return;
    }

    log_msg(LOG_LEVEL_INFO, "uid=%d added %d tag(s), %d already present\n",
            ctx->uid, added, already);
    m_respond_with_user_tags(ctx, "added", added, "already_present", already);
}

void tags_delete(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    tag_t_array* catalogue;
    tag_t_array* mine;
    int ids[MAX_TAGS_PER_REQUEST];
    size_t count = 0;
    size_t i;
    int removed = 0;
    int absent = 0;
    bool failed = false;

    if (!ctx)
        return;

    (void)user_data;

    catalogue = db_ttag_select_all_tags(get_db_id());
    if (!catalogue)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to retrieve tags from database\n");
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
                       "{\"error\":\"Failed to retrieve tags\"}");
        return;
    }

    if (m_tag_ids_from_body(ctx, catalogue, ids, &count) != SUCCESS)
    {
        db_ttag_free_array(catalogue);
        return;
    }

    mine = db_ttag_select_tag_objects_for_user(get_db_id(), ctx->uid);

    for (i = 0; i < count; i++)
    {
        /* Removing a tag the user does not have is a no-op, not an error, for
         * the same reason the insert is idempotent.
         */
        if (!m_user_has(mine, ids[i]))
        {
            absent++;
            continue;
        }

        if (db_ttag_delete_user_tag(get_db_id(), ctx->uid, ids[i]) != 0)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to delete tag %d for uid=%d\n", ids[i], ctx->uid);
            failed = true;
            break;
        }
        removed++;
    }

    if (mine)
        db_ttag_free_array(mine);
    db_ttag_free_array(catalogue);

    if (failed)
    {
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
                       "{\"error\":\"Failed to remove tags\"}");
        return;
    }

    log_msg(LOG_LEVEL_INFO, "uid=%d removed %d tag(s), %d were not present\n",
            ctx->uid, removed, absent);
    m_respond_with_user_tags(ctx, "removed", removed, "not_present", absent);
}

void api_tags_init()
{
    router_http_add("/api/tags/get", tags_get, NULL, AUTH_REQUIRED | M_GET);
    router_http_add("/api/tags/insert", tags_insert, NULL, AUTH_REQUIRED | M_POST | BODY_JSON);
    router_http_add("/api/tags/delete", tags_delete, NULL, AUTH_REQUIRED | M_POST | BODY_JSON);
}

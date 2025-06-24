#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../log/log_api.h"
#include "../parse/config_file.h"
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"
#include "../api/umgmt/token.h"
#include "../db/db_api.h"
#include "../db/db_gen.h"
#include "../db/tables/db_table_user.h"
#include "../../third_party/cJSON/cJSON.h"
#include "router_api.h"

#include "../server/server_api.h"

typedef struct
{
    int uid;
    int fd;

    UT_hash_handle hh;
} sio_data_t;

static sio_data_t *sio_connections = NULL;
static route_entry_t* m_routes = NULL;

void router_sio_add( const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags)
{
    router_add(&m_routes, path, cb, user_data, flags);
}

route_entry_t* router_sio_find(const char* path)
{
    return router_find(&m_routes, path);
}

void router_sio_delete(route_entry_t* entry)
{
    if (entry)
    {
        router_delete(&m_routes, entry);
    }
}

void router_sio_clear()
{
    router_clear(&m_routes);
    m_routes = NULL;
}

static int m_find_uid(int fd, int* uid)
{
    sio_data_t *d;

    HASH_FIND_INT(sio_connections, &fd, d);
    if (!d)
    {
        log_msg(LOG_LEVEL_ERROR, "Unknown fd %d", fd);
        return ERROR;
    }
    *uid = d->uid;
    return SUCCESS;
}

void router_handle_sio_request(int fd, const char *request, size_t request_len)
{
    const char* json_part;
    size_t json_len;
    cJSON* root;
    cJSON* evt_item;
    cJSON* data_item;
    route_entry_t *entry;
    char* json_str;
    char* payload_str;
    size_t payload_len;
    int uid;
    sio_request_ctx_t ctx;

    if (request_len < 2 || request[0] != '2')
    {
        log_msg(LOG_LEVEL_ERROR, "[SIO] Invalid frame (not '2'): '%.*s'",
                (int)request_len, request);
        return;
    }

    if (m_find_uid(fd, &uid) == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "[SIO] Failed to find uid for fd=%d", fd);
        return;
    }

    json_part = request + 1;
    json_len = request_len - 1;
    json_str = malloc(json_len + 1);
    memcpy(json_str, json_part, json_len);
    json_str[json_len] = '\0';

    root = cJSON_Parse(json_str);
    free(json_str);
    if (!root || !cJSON_IsArray(root))
    {
        log_msg(LOG_LEVEL_ERROR, "[SIO] Payload is not a JSON array");
        cJSON_Delete(root);
        return;
    }

    evt_item = cJSON_GetArrayItem(root, 0);
    if (!evt_item || !cJSON_IsString(evt_item))
    {
        log_msg(LOG_LEVEL_ERROR, "[SIO] First array element is not a string (event)");
        cJSON_Delete(root);
        return;
    }
    const char *evt_name = evt_item->valuestring;

    data_item = cJSON_GetArrayItem(root, 1);
    payload_str = NULL;
    payload_len = 0;
    if (data_item)
    {
        payload_str = cJSON_PrintUnformatted(data_item);
        if (payload_str)
        {
            payload_len = strlen(payload_str);
        }
    }

    ctx.fd = fd;
    ctx.uid = uid;
    ctx.event = strdup(evt_name);
    ctx.payload = payload_str;
    ctx.payload_len = payload_len;

    entry = router_sio_find(ctx.event);
    if (entry && entry->handler)
    {
        entry->handler(&ctx, entry->user_data);
    }
    else
    {
        log_msg(LOG_LEVEL_WARN, "[SIO] No handler registered for event '%s'. error for user '%d'", ctx.event, uid);
    }

    free(ctx.event);
    if (ctx.payload) free(ctx.payload);
    cJSON_Delete(root);
}

int sio_on_open(int fd, int uid)
{
    sio_data_t *data;

    data = malloc(sizeof *data);
    data->fd  = fd;
    data->uid = uid;
    HASH_ADD_INT(sio_connections, fd, data);
    log_msg(LOG_LEVEL_INFO, "SIO open fd=%d uid=%d", fd, uid);
    return SUCCESS;
}

int sio_on_close(int fd)
{
    sio_data_t *d;

    HASH_FIND_INT(sio_connections, &fd, d);
    if (d)
    {
        HASH_DEL(sio_connections, d);
        free(d);
        log_msg(LOG_LEVEL_INFO, "SIO closed fd=%d", fd);
    }
    return SUCCESS;
}

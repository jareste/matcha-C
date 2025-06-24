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
#include "router_api.h"

#include "../server/server_api.h"

DB_ID get_db_id();

void router_delete(route_entry_t** routes_hm, route_entry_t* entry)
{
    HASH_DEL(*routes_hm, entry);
    free(entry->path);
    free(entry);
}

void router_clear(route_entry_t** routes_hm)
{
    route_entry_t* current;
    route_entry_t* tmp;

    printf("Router: Clearing all routes\n");
    HASH_ITER(hh, *routes_hm, current, tmp)
    {
        printf("Router: Deleting route '%s'\n", current->path);
        HASH_DEL(*routes_hm, current);
        free(current->path);
        free(current);
    }
}

void router_add(route_entry_t** routes_hm, const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags)
{
    route_entry_t* entry = malloc(sizeof(*entry));

    log_msg(LOG_LEVEL_BOOT, "Router: Route '%s' added successfully.\n", path);

    entry->path = strdup(path);
    entry->handler = cb;
    entry->user_data = user_data;
    entry->flags = flags;
    HASH_ADD_KEYPTR(hh, *routes_hm, entry->path, strlen(entry->path), entry);
}

route_entry_t* router_find(route_entry_t** routes_hm, const char* path)
{
    route_entry_t* entry = NULL;

    HASH_FIND_STR(*routes_hm, path, entry);
    return entry;
}

int router_validate_token_for_server(int fd, const char* request, char** out_username, char** out_email, int* out_uid, char* origin)
{
    int rc;
    user_t* user;
    char* cookies;
    char* auth_cookie;

    cookies = get_header_value(request, "Cookie");
    if (!cookies)
    {
        log_msg(LOG_LEVEL_ERROR, "No cookies in request from fd=%d\n", fd);
        router_http_generate_response(fd, CODE_403_FORBIDDEN, "{\"error\": \"Forbidden\"}", origin);
        free(origin);
        return ERROR;
    }
    auth_cookie = strstr(cookies, "token=");
    if (!auth_cookie)
    {
        free(cookies);
        rc = router_http_generate_response(fd, CODE_403_FORBIDDEN, "{\"error\": \"Forbidden\"}", origin);
        free(origin);
        log_msg(LOG_LEVEL_ERROR, "No token in request from fd=%d\n", fd);
        return ERROR;
    }
    free(origin);

    rc = token_validate(auth_cookie + 6, out_username, out_email, out_uid);
    if (rc != SUCCESS)
    {
        log_msg(LOG_LEVEL_ERROR, "Token validation failed for token: %s\n", auth_cookie + 6);
        if (*out_email) free(*out_email);
        if (*out_username) free(*out_username);
        free(cookies);
        return ERROR;
    }

    rc = db_select_user_by_email(get_db_id(), *out_email, &user);
    if (rc != SUCCESS || !user)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to select user by email %s\n", *out_email);
        if (*out_email) free(*out_email);
        if (*out_username) free(*out_username);
        free(cookies);
        return ERROR;
    }

    if ((strcmp(user->username, *out_username) != 0) || 
        (user->id != *out_uid) ||
        (user->token == NULL) ||
        (strcmp(user->token, auth_cookie + 6) != 0))
    {
        log_msg(LOG_LEVEL_ERROR, "user->username: %s, out_username: %s, user->id: %d, out_uid: %d\n",
                user->username, *out_username, user->id, *out_uid);
        log_msg(LOG_LEVEL_ERROR, "\nutoken: '%s'\natoken: '%s'\n", user->token, auth_cookie + 6);
        log_msg(LOG_LEVEL_ERROR, "Invalid token for user %s (uid=%d),\n'%s'\n", *out_username, *out_uid, user->token);
        if (user) db_tuser_free_user(user);
        if (*out_email) free(*out_email);
        if (*out_username) free(*out_username);
        free(cookies);
        return ERROR;
    }

    if (user)
        db_tuser_free_user(user);

    free(cookies);
    return SUCCESS;
}

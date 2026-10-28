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
#include "../server/conn_api.h"

static route_entry_t* m_routes = NULL;

void router_http_add( const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags)
{
    router_add(&m_routes, path, cb, user_data, flags);
}

route_entry_t* router_http_find(const char* path)
{
    return router_find(&m_routes, path);
}

void router_http_delete(route_entry_t* entry)
{
    if (entry)
    {
        router_delete(&m_routes, entry);
    }
}

void router_http_clear()
{
    router_clear(&m_routes);
    m_routes = NULL;
}

static const char* m_http_code_to_status_text(HTTP_response_code_t code)
{
    switch (code)
    {
        case CODE_200_OK: return "OK";
        case CODE_201_CREATED: return "Created";
        case CODE_204_NO_CONTENT: return "No Content";
        case CODE_400_BAD_REQUEST: return "Bad Request";
        case CODE_401_UNAUTHORIZED: return "Unauthorized";
        case CODE_403_FORBIDDEN: return "Forbidden";
        case CODE_404_NOT_FOUND: return "Not Found";
        case CODE_405_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case CODE_500_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case CODE_503_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

char* get_header_value(const char *req, const char *key)
{
    char* p;
    char* end;
    char* out;
    size_t len;
    
    p = strcasestr(req, key);

    if (!p) return NULL;
    p = strchr(p, ':');
    if (!p) return NULL;
    
    p++;
    while (*p==' '||*p=='\t') p++;

    end = strstr(p, "\r\n");
    if (!end)
        end = p + strlen(p); /* Review it. before it was simply returning NULL. */

    len = end - p;
    out = malloc(len+1);
    memcpy(out, p, len);
    out[len]='\0';
    return out;
}

int router_http_respond_ex(int fd, HTTP_response_code_t code, const char* body,
                           const char* origin, const char* extra_headers)
{
    char body_buf[128];
    char header[1024];
    size_t body_len;
    const char* status_text;
    int header_len;

    if (!origin)
        origin = "http://localhost:8000";

    if (!extra_headers)
        extra_headers = "";

    status_text = m_http_code_to_status_text(code);

    if (code == CODE_204_NO_CONTENT)
    {
        header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: %s\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "%s"
            "Connection: close\r\n\r\n",
            origin, extra_headers);

        if (header_len <= 0 || (size_t)header_len >= sizeof(header))
        {
            log_msg(LOG_LEVEL_ERROR, "Header formatting error for code %d\n", code);
            return ERROR;
        }

        conn_write(fd, header, header_len);
        log_msg(LOG_LEVEL_DEBUG, "Response sent to fd=%d with code %d\n", fd, code);
        return SUCCESS;
    }

    /* No body given: synthesise one from the status text. */
    if (!body)
    {
        snprintf(body_buf, sizeof(body_buf), "{\"error\": \"%s\"}", status_text);
        body = body_buf;
    }

    body_len = strlen(body);

    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: %s\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "%s"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        code, status_text, origin, extra_headers, body_len);

    if (header_len <= 0 || (size_t)header_len >= sizeof(header))
    {
        log_msg(LOG_LEVEL_ERROR, "Header formatting error for code %d\n", code);
        return ERROR;
    }

    conn_write(fd, header, header_len);
    conn_write(fd, body, body_len);

    log_msg(LOG_LEVEL_DEBUG, "Response sent to fd=%d with code %d\n", fd, code);
    return SUCCESS;
}

int router_http_generate_response(int fd, HTTP_response_code_t code, const char* body, const char* origin)
{
    return router_http_respond_ex(fd, code, body, origin, NULL);
}

int router_parse_http_request(const char* request, size_t request_len, http_request_t* out_request)
{
    const char* first_line_end;
    const char* line_start;
    const char* line_end;
    const char* space1;
    const char* space2;
    const char* headers_end;
    const char* body_start;
    char* content_type;
    char *tmp;
    size_t headers_len;
    size_t line_len;
    size_t body_len;

    if (!request || (request_len == 0) || !out_request)
        return ERROR;

    /*
     * Zero first: every error path below used to leave the caller with
     * indeterminate pointers that it would then pass to free_http_request.
     */
    memset(out_request, 0, sizeof(*out_request));

    first_line_end = strstr(request, "\r\n");
    if (!first_line_end)
        return ERROR;

    line_start = request;
    line_end = first_line_end;
    line_len = line_end - line_start;

    space1 = memchr(line_start, ' ', line_len);
    if (!space1 || space1 >= line_end)
        return ERROR;

    out_request->method = strndup(line_start, space1 - line_start);

    space2 = memchr(space1 + 1, ' ', line_end - space1 - 1);
    if (!space2 || space2 >= line_end)
    {
        free_http_request(out_request);
        memset(out_request, 0, sizeof(*out_request));
        return ERROR;
    }

    out_request->route = strndup(space1 + 1, space2 - (space1 + 1));

    headers_end = strstr(first_line_end + 2, "\r\n\r\n");
    if (!headers_end)
    {
        free_http_request(out_request);
        memset(out_request, 0, sizeof(*out_request));
        return ERROR;
    }

    headers_len = headers_end - (first_line_end + 2);
    out_request->headers = strndup(first_line_end + 2, headers_len);

    content_type = get_header_value(out_request->headers, "Content-Type");
    if (content_type)
    {
        log_msg(LOG_LEVEL_DEBUG, "Content-Type: %s\n", content_type);

        if (strstr(content_type, "application/json"))
            out_request->is_binary = false;
        else if (strstr(content_type, "multipart/form-data") || strstr(content_type, "application/octet-stream"))
            out_request->is_binary = true;
        else
            out_request->is_binary = false;

        free(content_type);
    }
    else
    {
        log_msg(LOG_LEVEL_WARN, "No Content-Type header found\n");
        out_request->is_binary = false;
    }

    body_start = headers_end + 4;
    if ((size_t)(body_start - request) < request_len)
    {
        body_len = request_len - (body_start - request);
        out_request->body = malloc(body_len);

        memcpy(out_request->body, body_start, body_len);
        out_request->body_len = body_len;

        if (!out_request->is_binary)
        {
            tmp = realloc(out_request->body, body_len + 1);
            out_request->body = tmp;
            out_request->body[body_len] = '\0';
        }
    }
    else
    {
        out_request->body = NULL;
        out_request->body_len = 0;
    }

    return SUCCESS;
}

void free_http_request(http_request_t* request)
{
    if (!request) return;

    free(request->method);
    free(request->route);
    free(request->headers);
    free(request->body);
}

static int router_validate_request_token(http_request_ctx_t* ctx, char* request, char* origin)
{
    return router_validate_token_for_server(ctx->fd, request, &ctx->username, &ctx->email, &ctx->uid, origin);
}

static void send_cors_preflight(int fd, const char *origin)
{
    char buf[512];
    int n;
    
    n = snprintf(buf, sizeof(buf),
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: %s\r\n"
        "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        origin
    );
    log_msg(LOG_LEVEL_DEBUG, "CORS preflight response: \n%s\n", buf);
    conn_write(fd, buf, n);
}

static http_request_flags_t m_method_flag(const char* method)
{
    if (!method)
        return FLAG_NONE;
    if (strcmp(method, "GET") == 0)
        return M_GET;
    if (strcmp(method, "POST") == 0)
        return M_POST;
    if (strcmp(method, "PUT") == 0)
        return M_PUT;
    if (strcmp(method, "PATCH") == 0)
        return M_PATCH;
    if (strcmp(method, "DELETE") == 0)
        return M_DELETE;
    return FLAG_NONE;
}

static bool m_route_preconditions_ok(http_request_ctx_t* ctx,
                                     const route_entry_t* route_entry,
                                     const char* method)
{
    http_request_flags_t allowed = route_entry->flags & M_ANY_METHOD;

    if (allowed && !(m_method_flag(method) & allowed))
    {
        log_msg(LOG_LEVEL_WARN, "Method %s not allowed on %s\n",
                method ? method : "(none)", route_entry->path);
        router_http_generate_response(ctx->fd, CODE_405_METHOD_NOT_ALLOWED,
                                      "{\"error\":\"Method Not Allowed\"}", ctx->origin);
        return false;
    }

    if ((route_entry->flags & QUERY_NEEDED) && !ctx->query)
    {
        log_msg(LOG_LEVEL_WARN, "Route %s requires a query string\n", route_entry->path);
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Missing query parameters\"}", ctx->origin);
        return false;
    }

    if (route_entry->flags & (BODY_REQUIRED | BODY_JSON))
    {
        if (!ctx->parsed_request.body || ctx->parsed_request.body_len == 0)
        {
            log_msg(LOG_LEVEL_WARN, "Route %s requires a body\n", route_entry->path);
            router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                          "{\"error\":\"Request body is required\"}", ctx->origin);
            return false;
        }
    }

    if (route_entry->flags & BODY_JSON)
    {
        ctx->json = cJSON_Parse(ctx->parsed_request.body);
        if (!ctx->json)
        {
            log_msg(LOG_LEVEL_WARN, "Route %s got a body that is not valid JSON\n", route_entry->path);
            log_msg(LOG_LEVEL_DEBUG, "Body was:\n%s\n", ctx->parsed_request.body);
            router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                          "{\"error\":\"Invalid JSON\"}", ctx->origin);
            return false;
        }
    }

    return true;
}

/********************************************/
/* CTX HELPERS                              */
/********************************************/
static void m_respond_field_required(http_request_ctx_t* ctx, const char* field)
{
    char body[160];

    snprintf(body, sizeof(body), "{\"error\":\"%s is required\"}", field);
    router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST, body, ctx->origin);
}

bool ctx_require_str(http_request_ctx_t* ctx, const char* field, const char** out)
{
    cJSON* item;

    if (out)
        *out = NULL;

    if (!ctx || !ctx->json || !field || !out)
        return false;

    item = cJSON_GetObjectItemCaseSensitive(ctx->json, field);
    if (!cJSON_IsString(item) || !item->valuestring || item->valuestring[0] == '\0')
    {
        m_respond_field_required(ctx, field);
        return false;
    }

    *out = item->valuestring;
    return true;
}

bool ctx_require_array(http_request_ctx_t* ctx, const char* field, cJSON** out)
{
    cJSON* item;

    if (out)
        *out = NULL;

    if (!ctx || !ctx->json || !field || !out)
        return false;

    item = cJSON_GetObjectItemCaseSensitive(ctx->json, field);
    if (!cJSON_IsArray(item))
    {
        m_respond_field_required(ctx, field);
        return false;
    }

    *out = item;
    return true;
}

const char* ctx_opt_str(http_request_ctx_t* ctx, const char* field, const char* fallback)
{
    cJSON* item;

    if (!ctx || !ctx->json || !field)
        return fallback;

    item = cJSON_GetObjectItemCaseSensitive(ctx->json, field);
    if (!cJSON_IsString(item) || !item->valuestring)
        return fallback;

    return item->valuestring;
}

bool ctx_query_int(http_request_ctx_t* ctx, const char* key, int* out)
{
    const char* p;
    size_t key_len;

    if (!ctx || !ctx->query || !key || !out)
        return false;

    key_len = strlen(key);
    p = ctx->query;

    while (*p)
    {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=')
        {
            const char* val = p + key_len + 1;

            if (*val == '\0')
                return false;

            *out = atoi(val);
            return true;
        }

        p = strchr(p, '&');
        if (!p)
            break;
        p++;
    }

    return false;
}

int router_handle_http_request(int fd, const char* request, size_t request_len)
{
    const char* route = NULL;
    const char* first_line_end = NULL;
    char first_line[256] = {0};
    char* space_pos = NULL;
    char* route_end = NULL;
    char* route_query = NULL;
    size_t first_line_len = 0;
    route_entry_t* route_entry = NULL;
    http_request_ctx_t request_ctx;
    char* origin;
    char* method = NULL;
    int rc;
    size_t len;


    first_line_end = strstr(request, "\r\n");
    if (first_line_end)
    {
        first_line_len = first_line_end - request;
        strncpy(first_line, request, first_line_len);

        space_pos = strchr(first_line, ' ');
        if (space_pos)
        {
            method = first_line;
            *space_pos = '\0';
            route = space_pos + 1;

            route_end = strchr(route, ' ');
            if (route_end)
                *route_end = '\0';

            route_query = strchr(route, '?');
            if (route_query)
            {
                *route_query = '\0';
                route_query++;
            }

            len = strlen(route);
            route_end = (char*)(route + len - 1);
            if (len > 1 && *route_end == '/')
                *route_end = '\0';
        }
    }

    if (!route)
        return router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request\"}", NULL);

    route_entry = router_http_find(route);
    if (!route_entry)
    {
        log_msg(LOG_LEVEL_ERROR, "No route found for path: %s\n", route);
        return router_http_generate_response(fd, CODE_404_NOT_FOUND, "{\"error\": \"Not Found\"}", NULL);
    }

    if (!route_entry->handler)
        return router_http_generate_response(fd, CODE_500_INTERNAL_SERVER_ERROR, "{\"error\": \"Internal Server Error\"}", NULL);

    origin = get_header_value(request, "Origin");
    if (!origin)
        origin = strdup("http://localhost:8000");

    if (!method)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse method from request\n");
        rc = router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request\"}", origin);
        free(origin);
        return rc;
    }

    if (strcmp(method, "OPTIONS") == 0)
    {
        log_msg(LOG_LEVEL_DEBUG, "CORS preflight for %s\n", route);
        send_cors_preflight(fd, origin);
        free(origin);
        return SUCCESS;
    }

    memset(&request_ctx, 0, sizeof(request_ctx));
    request_ctx.fd = fd;
    request_ctx.request = request;
    request_ctx.request_len = request_len;
    request_ctx.query = route_query;
    request_ctx.origin = origin;
    request_ctx.uid = -1;

    if (route_entry->flags & AUTH_REQUIRED)
    {
        /* Answers the client itself (401) when the token is missing or bad. */
        if (router_validate_request_token(&request_ctx, (char*)request, origin) != SUCCESS)
        {
            free(origin);
            return SUCCESS;
        }
    }

    if (router_parse_http_request(request, request_len, &request_ctx.parsed_request) != SUCCESS)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse HTTP request\n");
        log_msg(LOG_LEVEL_DEBUG, "Request was:\n%s\n", request);
        rc = router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request\"}", origin);
        goto cleanup;
    }

    if (m_route_preconditions_ok(&request_ctx, route_entry, method))
        route_entry->handler(&request_ctx, route_entry->user_data);

    rc = SUCCESS;

cleanup:
    if (request_ctx.json)
        cJSON_Delete(request_ctx.json);
    free_http_request(&request_ctx.parsed_request);
    if (request_ctx.username) free(request_ctx.username);
    if (request_ctx.email) free(request_ctx.email);
    free(origin);

    return rc;
}

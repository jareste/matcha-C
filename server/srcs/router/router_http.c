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
    if (!end) return NULL;

    len = end - p;
    out = malloc(len+1);
    memcpy(out, p, len);
    out[len]='\0';
    return out;
}

int router_http_generate_response(int fd, HTTP_response_code_t code, const char* body, const char* origin)
{
    char body_buf[128];
    char header[512];
    size_t body_len;
    const char* status_text;
    int header_len;

    if (!origin)
    {
        origin = "http://localhost:8000";
    }

    if (code == CODE_204_NO_CONTENT)
    {
        snprintf(header, sizeof(header), "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");

        send(fd, header, strlen(header), 0);
    }
    else if (body)
    {
        status_text = m_http_code_to_status_text(code);
        body_len = strlen(body);

        header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: %s\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Access-Control-Allow-Methods: POST\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n",
            code, status_text, origin, body_len);

        if (header_len <= 0 || (size_t)header_len >= sizeof(header))
        {
            log_msg(1, "Header formatting error\n");
            return ERROR;
        }

        send(fd, header, header_len, 0);
        send(fd, body, body_len, 0);
    }
    else
    {
        status_text = m_http_code_to_status_text(code);

        body_len = snprintf(body_buf, sizeof(body_buf), "{\"error\": \"%s\"}", status_text);

        header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: %s\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Access-Control-Allow-Methods: POST\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n",
            code, status_text, origin, body_len);

        if (header_len <= 0 || (size_t)header_len >= sizeof(header))
        {
            log_msg(1, "Header formatting error\n");
            return ERROR;
        }

        send(fd, header, header_len, 0);
        send(fd, body_buf, body_len, 0);
    }

    log_msg(0, "Response sent to fd=%d with code %d\n", fd, code);
    return SUCCESS;
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
    size_t headers_len;
    size_t line_len;

    if (!request || (request_len == 0) || !out_request)
        return ERROR;

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
        return ERROR;

    out_request->route = strndup(space1 + 1, space2 - (space1 + 1));

    headers_end = strstr(first_line_end + 2, "\r\n\r\n");
    if (!headers_end)
        return ERROR;

    headers_len = headers_end - (first_line_end + 2);
    out_request->headers = strndup(first_line_end + 2, headers_len);

    body_start = headers_end + 4;
    if ((size_t)(body_start - request) < request_len)
        out_request->body = strndup(body_start, request + request_len - body_start);
    else
        out_request->body = NULL;

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
    write(fd, buf, n);
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

    if ((route_entry->flags & QUERY_NEEDED) && !route_query)
    {
        log_msg(LOG_LEVEL_ERROR, "Route %s requires query parameters but none found\n", route);
        return router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request - Missing Query Parameters\"}", NULL);
    }

    origin = get_header_value(request, "Origin");
    if (!origin) origin = strdup("http://localhost:8000"); /* error instead ? */

    if (!method)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse method from request\n");
        free(origin);
        return router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request\"}", NULL);
    }

    if (strcmp(method, "OPTIONS")==0)
    {
        log_msg(LOG_LEVEL_DEBUG, "CORS preflight for %s\n", route);
        send_cors_preflight(fd, origin);
        free(origin);
        return SUCCESS;
    }

    request_ctx.fd = fd;
    request_ctx.request = request;
    request_ctx.request_len = request_len;
    request_ctx.query = route_query;

    if (route_entry->flags & AUTH_REQUIRED)
    {
        rc = router_validate_request_token(&request_ctx, (char*)request, origin);
        if (rc != SUCCESS)
            return rc;
    }
    else
    {
        request_ctx.uid = -1; // No user authenticated
        request_ctx.username = NULL;
        request_ctx.email = NULL;
        free(origin);
    }

    /* Populate request ctx */
    router_parse_http_request(request, request_len, &request_ctx.parsed_request);

    /* call the handler */
    route_entry->handler(&request_ctx, route_entry->user_data);

    free_http_request(&request_ctx.parsed_request);

    if (request_ctx.username) free(request_ctx.username);
    if (request_ctx.email) free(request_ctx.email);

    return SUCCESS;
}

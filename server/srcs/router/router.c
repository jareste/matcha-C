#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../log/log_api.h"
#include "../parse/config_file.h"
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"
#include "router_api.h"

static route_entry_t* m_routes = NULL;

static const char* m_http_code_to_status_text(HTTP_response_code_t code)
{
    switch (code)
    {
        case CODE_200_OK: return "OK";
        case CODE_201_CREATED: return "Created";
        case CODE_204_NO_CONTENT: return "No Content";
        case CODE_400_BAD_REQUEST: return "Bad Request";
        case CODE_404_NOT_FOUND: return "Not Found";
        case CODE_405_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case CODE_500_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case CODE_503_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

int router_http_generate_response(int fd, HTTP_response_code_t code, const char* body)
{
    char body_buf[128];
    char header[512];
    size_t body_len;
    const char* status_text;
    int header_len;

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
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n",
            code, status_text, body_len);

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
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n",
            code, status_text, body_len);

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

void router_add(const char* path, route_cb_t cb, void* user_data)
{
    route_entry_t* entry = malloc(sizeof(*entry));

    entry->path = strdup(path);
    entry->handler = cb;
    entry->user_data = user_data;
    HASH_ADD_KEYPTR(hh, m_routes, entry->path, strlen(entry->path), entry);
}

static route_entry_t* router_find(const char* path)
{
    route_entry_t* entry = NULL;

    HASH_FIND_STR(m_routes, path, entry);
    return entry;
}

void router_delete(route_entry_t* entry)
{
    HASH_DEL(m_routes, entry);
    free(entry->path);
    free(entry);
}

void router_clear()
{
    route_entry_t* current;
    route_entry_t* tmp;

    HASH_ITER(hh, m_routes, current, tmp)
    {
        HASH_DEL(m_routes, current);
        free(current->path);
        free(current);
    }
}

int router_parse_http_request(const char* request, size_t request_len, char** method, char** route, char** headers, char** body)
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

    if (!request || request_len == 0 || !method || !route || !headers || !body)
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

    *method = strndup(line_start, space1 - line_start);

    space2 = memchr(space1 + 1, ' ', line_end - space1 - 1);
    if (!space2 || space2 >= line_end)
        return ERROR;

    *route = strndup(space1 + 1, space2 - (space1 + 1));

    headers_end = strstr(first_line_end + 2, "\r\n\r\n");
    if (!headers_end)
        return ERROR;

    headers_len = headers_end - (first_line_end + 2);
    *headers = strndup(first_line_end + 2, headers_len);

    body_start = headers_end + 4;
    if ((size_t)(body_start - request) < request_len)
        *body = strndup(body_start, request + request_len - body_start);
    else
        *body = NULL;

    return SUCCESS;
}

int router_handle_http_request(int fd, const char* request, size_t request_len)
{
    const char* route = NULL;
    const char* first_line_end = NULL;
    char first_line[256] = {0};
    char* space_pos = NULL;
    char *route_end = NULL;
    size_t first_line_len = 0;
    route_entry_t* route_entry = NULL;
    http_request_ctx_t request_ctx;

    first_line_end = strstr(request, "\r\n");
    if (first_line_end)
    {
        first_line_len = first_line_end - request;
        strncpy(first_line, request, first_line_len);

        space_pos = strchr(first_line, ' ');
        if (space_pos)
        {
            *space_pos = '\0';
            route = space_pos + 1;

            route_end = strchr(route, ' ');
            if (route_end)
                *route_end = '\0';
        }
    }

    if (!route)
        return router_http_generate_response(fd, CODE_400_BAD_REQUEST, "{\"error\": \"Bad Request\"}");

    route_entry = router_find(route);
    if (!route_entry)
        return router_http_generate_response(fd, CODE_404_NOT_FOUND, "{\"error\": \"Not Found\"}");

    if (!route_entry->handler)
        return router_http_generate_response(fd, CODE_500_INTERNAL_SERVER_ERROR, "{\"error\": \"Internal Server Error\"}");

    /* Populate request ctx */
    request_ctx.fd = fd;
    request_ctx.request = request;
    request_ctx.request_len = request_len;

    /* call the handler */
    route_entry->handler(&request_ctx, route_entry->user_data);

    return SUCCESS;
}

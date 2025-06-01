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

int router_handle_http_request(int fd, const char *request, size_t request_len)
{
    const char *method = NULL;
    const char *route = NULL;
    const char *headers = NULL;
    const char *body = NULL;
    const char *headers_end = NULL;
    const char *first_line_end = NULL;
    char first_line[256] = {0};
    char* err_string = NULL;
    char *space_pos = NULL;
    size_t headers_len = 0;
    size_t first_line_len = 0;
    
    (void)request_len;

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

            char *route_end = strchr(route, ' ');
            if (route_end)
            {
                *route_end = '\0';
            }
        }
    }

    headers_end = strstr(request, "\r\n\r\n");
    if (headers_end)
    {
        headers_len = headers_end - (first_line_end + 2);
        headers = (char *)malloc(headers_len + 1);
        strncpy((char *)headers, first_line_end + 2, headers_len);
        ((char *)headers)[headers_len] = '\0';
    }

    if (headers_end)
    {
        body = headers_end + 4;
    }

    log_msg(LOG_LEVEL_INFO, "Parsed HTTP request:");
    log_msg(LOG_LEVEL_INFO, "Method:\n\t- '%s'\n", method ? method : "Unknown");
    log_msg(LOG_LEVEL_INFO, "Route:\n\t- '%s'\n", route ? route : "Unknown");
    log_msg(LOG_LEVEL_INFO, "Body:\n\t- '%s'\n", body ? body : "None");
    log_msg(LOG_LEVEL_ERROR, "%s\n", request);

    if (method && (strcmp(method, "POST") != 0) && (strcmp(method, "PUT") != 0))
    {
        log_msg(LOG_LEVEL_ERROR, "Unsupported HTTP method: %s\n", method ? method : "Unknown");
        err_string = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\n\r\nMethod Not Allowed\r\n";
        send(fd, err_string, strlen(err_string), 0);
        log_msg(LOG_LEVEL_ERROR, "Response sent to fd=%d\n", fd);
        if (headers)
        {
            free((char *)headers);
        }
        return ERROR;
    }

    if (headers)
    {
        free((char *)headers);
    }

    const char *response =
    "HTTP/1.1 200 OK\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: POST\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Thanks for the report.\n";

    send(fd, response, strlen(response), 0);

    log_msg(LOG_LEVEL_INFO, "Response sent to fd=%d\n", fd);

    return SUCCESS;
}

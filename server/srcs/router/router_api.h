#ifndef ROUTER_API_H
#define ROUTER_API_H

#include "../../third_party/uthash-master/src/uthash.h"

typedef struct 
{
    int fd;
    const char* request;
    size_t request_len;
} http_request_ctx_t;

typedef void (*route_cb_t)(http_request_ctx_t* request_ctx, void *user_data);

typedef struct route_entry
{
    char* path;
    route_cb_t handler;
    void* user_data;
    UT_hash_handle hh;
} route_entry_t;

typedef enum
{
    CODE_200_OK = 200,
    CODE_201_CREATED = 201,
    CODE_204_NO_CONTENT = 204,
    CODE_400_BAD_REQUEST = 400,
    CODE_404_NOT_FOUND = 404,
    CODE_405_METHOD_NOT_ALLOWED = 405,
    CODE_500_INTERNAL_SERVER_ERROR = 500,
    CODE_503_SERVICE_UNAVAILABLE = 503
} HTTP_response_code_t;

void router_add(const char* path, route_cb_t cb, void* user_data);
void router_delete(route_entry_t* entry);
void router_clear();

int router_handle_http_request(int fd, const char* request, size_t request_len);
int router_http_generate_response(int fd, HTTP_response_code_t code, const char* body);
int router_parse_http_request(const char* request, size_t request_len, char** method,\
    char** route, char** headers, char** body);

#endif /* ROUTER_API_H */

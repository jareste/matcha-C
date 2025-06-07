#ifndef ROUTER_API_H
#define ROUTER_API_H

#include "../../third_party/uthash-master/src/uthash.h"

typedef struct 
{
    int fd;
    const char* request;
    size_t request_len;
    const char* method;
    const char* route;
    const char* headers;
    const char* body;
} http_request_ctx_t;

typedef void (*route_cb_t)(http_request_ctx_t* request_ctx, void *user_data);

typedef struct route_entry
{
    char* path;
    route_cb_t handler;
    void* user_data;
    UT_hash_handle hh;
} route_entry_t;

void router_add(const char* path, route_cb_t cb, void* user_data);
void router_delete(route_entry_t* entry);
void router_clear();

int router_handle_http_request(int fd, const char* request, size_t request_len);

#endif /* ROUTER_API_H */

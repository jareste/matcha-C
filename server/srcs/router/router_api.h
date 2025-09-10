#ifndef ROUTER_API_H
#define ROUTER_API_H

#include "../../third_party/uthash-master/src/uthash.h"

typedef struct
{
    char* method;
    char* route;
    char* headers;
    char* body;
} http_request_t;

typedef struct 
{
    int fd;
    const char* request;
    size_t request_len;
    http_request_t parsed_request;
    char* query;
    int uid;
    char* username;
    char* email;
} http_request_ctx_t;

typedef struct
{
    int fd;
    int uid;
    char* event;
    char* payload;
    size_t payload_len;
} sio_request_ctx_t;


typedef void (*route_cb_t)(void* request_ctx, void *user_data);

typedef enum
{
    FLAG_NONE = 0x00,
    AUTH_REQUIRED = 0x01,
    LOCAL_REQUEST = 0x02,
} http_request_flags_t;

typedef struct route_entry
{
    char* path;
    route_cb_t handler;
    void* user_data;
    http_request_flags_t flags;

    UT_hash_handle hh;
} route_entry_t;

typedef enum
{
    CODE_200_OK = 200,
    CODE_201_CREATED = 201,
    CODE_204_NO_CONTENT = 204,
    CODE_400_BAD_REQUEST = 400,
    CODE_401_UNAUTHORIZED = 401,
    CODE_403_FORBIDDEN = 403,
    CODE_404_NOT_FOUND = 404,
    CODE_405_METHOD_NOT_ALLOWED = 405,
    CODE_500_INTERNAL_SERVER_ERROR = 500,
    CODE_503_SERVICE_UNAVAILABLE = 503
} HTTP_response_code_t;

/* ROUTER */
void router_add(route_entry_t** routes_hm, const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags);
void router_delete(route_entry_t** routes_hm, route_entry_t* entry);
void router_clear(route_entry_t** routes_hm);
route_entry_t* router_find(route_entry_t** routes_hm, const char* path);
int router_validate_token_for_server(int fd, const char* request, char** out_username, char** out_email, int* out_uid, char* origin);

/* ROUTER HTTP */
int router_handle_http_request(int fd, const char* request, size_t request_len);
int router_http_generate_response(int fd, HTTP_response_code_t code, const char* body, const char* origin);
int router_parse_http_request(const char* request, size_t request_len, http_request_t* out_request);
void free_http_request(http_request_t* request);
char* get_header_value(const char *req, const char *key);
void router_http_add( const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags);
route_entry_t* router_http_find(const char* path);
void router_http_delete(route_entry_t* entry);
void router_http_clear();

/* ROUTER SIO */
void router_sio_add(const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags);
void router_handle_sio_request(int fd, const char *request, size_t request_len);
int sio_on_open(int fd, int uid);
int sio_on_close(int fd);

#endif /* ROUTER_API_H */

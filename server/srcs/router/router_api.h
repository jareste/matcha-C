#ifndef ROUTER_API_H
#define ROUTER_API_H

#include "../../third_party/uthash-master/src/uthash.h"
#include "../../third_party/cJSON/cJSON.h"
#include <stdbool.h>

typedef struct
{
    char* method;
    char* route;
    char* headers;
    char* body;
    size_t body_len;
    bool is_binary;
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

    /* Owned by the router: the Origin header of this request, never NULL. */
    char* origin;
    cJSON* json;
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

/*
 * Route preconditions, all enforced centrally in router_handle_http_request
 * before the handler is entered. Declaring them at registration keeps a handler
 * from having to remember a check -- and from being reachable in a state it was
 * never written for.
 */
typedef enum
{
    FLAG_NONE     = 0x000,
    AUTH_REQUIRED = 0x001,
    LOCAL_REQUEST = 0x002,
    QUERY_NEEDED  = 0x004,

    /* Allowed methods. If no M_* bit is set the route accepts any method. */
    M_GET         = 0x008,
    M_POST        = 0x010,
    M_PUT         = 0x020,
    M_PATCH       = 0x040,
    M_DELETE      = 0x080,

    /* 400 unless the request carries a non-empty body. */
    BODY_REQUIRED = 0x100,

    /* Implies BODY_REQUIRED; 400 unless the body parses, then sets ctx->json. */
    BODY_JSON     = 0x200
} http_request_flags_t;

#define M_ANY_METHOD (M_GET | M_POST | M_PUT | M_PATCH | M_DELETE)

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
int router_http_respond_ex(int fd, HTTP_response_code_t code, const char* body,
                           const char* origin, const char* extra_headers);
int router_parse_http_request(const char* request, size_t request_len, http_request_t* out_request);
void free_http_request(http_request_t* request);
char* get_header_value(const char *req, const char *key);
void router_http_add( const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags);
route_entry_t* router_http_find(const char* path);
void router_http_delete(route_entry_t* entry);
void router_http_clear();

bool        ctx_require_str(http_request_ctx_t* ctx, const char* field, const char** out);
bool        ctx_require_array(http_request_ctx_t* ctx, const char* field, cJSON** out);
const char* ctx_opt_str(http_request_ctx_t* ctx, const char* field, const char* fallback);
bool        ctx_query_int(http_request_ctx_t* ctx, const char* key, int* out);

/* ROUTER SIO */
void router_sio_add(const char* path, route_cb_t cb, void* user_data, http_request_flags_t flags);
void router_handle_sio_request(int fd, const char *request, size_t request_len);
int sio_on_open(int fd, int uid);
int sio_on_close(int fd);

#endif /* ROUTER_API_H */

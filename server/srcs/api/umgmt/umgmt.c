#include "api_umgmt.h"
#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_gen.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_user.h"

DB_ID get_db_id();

void api_umgmt_register(http_request_ctx_t* ctx, void *user_data)
{
    http_request_t request;
    char* password;
    user_t new_user = {0};
    user_t *existing_user = NULL;
    cJSON *json = NULL;
    cJSON *username_item = NULL;
    cJSON *email_item = NULL;
    cJSON *password_item = NULL;
    cJSON *first_name_item = NULL;
    cJSON *last_name_item = NULL;
    int ret;

    (void)user_data;

    if (router_parse_http_request(ctx->request, ctx->request_len, &request) == ERROR)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Bad Request\"}", NULL);
        goto cleanup;
    }

    if (strcmp(request.method, "POST") != 0)
    {
        router_http_generate_response(ctx->fd, CODE_405_METHOD_NOT_ALLOWED,
                                      "{\"error\":\"Method Not Allowed\"}", NULL);
        goto cleanup;
    }

    /* Parse request body for user registration data */
    json = cJSON_Parse(request.body);
    if (!json)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Invalid JSON\"}", NULL);
        goto cleanup;
    }
    /* Fill user */
    username_item = (cJSON_GetObjectItemCaseSensitive(json, "username"));
    if (!username_item || !cJSON_IsString(username_item) || (username_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Username is required\"}", NULL);
        goto cleanup;
    }
    email_item = (cJSON_GetObjectItemCaseSensitive(json, "email"));
    if (!email_item || !cJSON_IsString(email_item) || (email_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Email is required\"}", NULL);
        goto cleanup;
    }
    password_item = (cJSON_GetObjectItemCaseSensitive(json, "password"));
    if (!password_item || !cJSON_IsString(password_item) || (password_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Password is required\"}", NULL);
        goto cleanup;
    }
    first_name_item = (cJSON_GetObjectItemCaseSensitive(json, "first_name"));
    if (!first_name_item || !cJSON_IsString(first_name_item) || (first_name_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"First name is required\"}", NULL);
        goto cleanup;
    }
    last_name_item = (cJSON_GetObjectItemCaseSensitive(json, "last_name"));
    if (!last_name_item || !cJSON_IsString(last_name_item) || (last_name_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Last name is required\"}", NULL);
        goto cleanup;
    }

    new_user.username = username_item->valuestring;
    new_user.email = email_item->valuestring;
    password = password_item->valuestring;
    new_user.first_name = first_name_item->valuestring;
    new_user.last_name = last_name_item->valuestring;

    new_user.gender = "undefined";
    new_user.orientation = "undefined";
    new_user.password_hash = password; /* TODO hash password */
    new_user.bio = NULL;
    new_user.fame_rating = 0;
    new_user.gps_lat = 0.0;
    new_user.gps_lon = 0.0;
    new_user.location_optout = false;
    new_user.last_online = NULL;
    new_user.created_at = time(NULL);
    new_user.email_verified = false;

    if (!new_user.username || !new_user.email || !password)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Missing required fields\"}", NULL);
        goto cleanup;
    }

    db_select_user_by_username(get_db_id(), new_user.username, &existing_user);
    if (existing_user)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Username already exists\"}", NULL);
        goto cleanup;
    }

    db_select_user_by_email(get_db_id(), new_user.email, &existing_user);
    if (existing_user)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Email already exists\"}", NULL);
        goto cleanup;
    }

    ret = db_tuser_insert_user(get_db_id(), &new_user);
    if (ret == ERROR)
    {
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
                                      "{\"error\":\"Failed to register user\"}", NULL);
        goto cleanup;
    }

    router_http_generate_response(ctx->fd, CODE_201_CREATED,
        "{\"success\":true, \"message\":\"Registration successful\"}", NULL);
        
cleanup:
    if (json)    
        cJSON_Delete(json);

    if (existing_user)
        free(existing_user);

    free_http_request(&request);
}
#include <unistd.h>

void api_umgmt_login(http_request_ctx_t* ctx, void *user_data)
{
    http_request_t request;
    user_t* existing_user = NULL;
    cJSON* json = NULL;
    cJSON* email_item = NULL;
    cJSON* password_item = NULL;

    (void)user_data;

    if (router_parse_http_request(ctx->request, ctx->request_len, &request) == ERROR)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Bad Request\"}", NULL);
        goto cleanup;
    }

    if (strcmp(request.method, "POST") != 0)
    {
        router_http_generate_response(ctx->fd, CODE_405_METHOD_NOT_ALLOWED,
                                      "{\"error\":\"Method Not Allowed\"}", NULL);
        goto cleanup;
    }

    json = cJSON_Parse(request.body);
    if (!json)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Invalid JSON\"}", NULL);
        goto cleanup;
    }

    printf("request body: %s\n", request.body);
    email_item = cJSON_GetObjectItemCaseSensitive(json, "email");
    if (!email_item || !cJSON_IsString(email_item) || (email_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Username is required\"}", NULL);
        goto cleanup;
    }

    db_select_user_by_email(get_db_id(), email_item->valuestring, &existing_user);
    if (!existing_user)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"User not found\"}", NULL);
        goto cleanup;
    }
    password_item = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (!password_item || !cJSON_IsString(password_item) || (password_item->valuestring == NULL))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Password is required\"}", NULL);
        goto cleanup;
    }
    if (strcmp(existing_user->password_hash, password_item->valuestring) != 0)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Invalid credentials\"}", NULL);
        goto cleanup;
    }

    /* TODO not sending emails yet */
    // if (!existing_user->email_verified)
    // {
    //     router_http_generate_response(ctx->fd, CODE_403_FORBIDDEN,
    //                                   "{\"error\":\"Email not verified\"}");
    //     goto cleanup;
    // }

    if (strcmp(existing_user->password_hash, password_item->valuestring) != 0)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                        "{\"error\":\"Invalid credentials\"}", NULL);
        goto cleanup;
    }

    /* TODO make it prettier */
    const char *resp_body = "{\"success\":true, \"message\":\"Login successful\"}";
    size_t blen = strlen(resp_body);
    const char* session_id = "HolaQueTalEstoEsUNTest";

    char header_buf[768];
    int hlen = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: token=%s; HttpOnly; Path=/; SameSite=Strict\r\n"
        "Access-Control-Allow-Origin: %s\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        session_id, "http://localhost:8000", blen
    );

    write(ctx->fd, header_buf, hlen);
    write(ctx->fd, resp_body, blen);
    /* TODO_END */

    // router_http_generate_response(ctx->fd, CODE_200_OK,
    //     "{\"success\":true, \"message\":\"Login successful\"}", NULL);

cleanup:
    free_http_request(&request);
}

void api_umgmt_init()
{
    router_add("/api/register", api_umgmt_register, NULL, FLAG_NONE);
    router_add("/api/login", api_umgmt_login, NULL, AUTH_REQUIRED);
}


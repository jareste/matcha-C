#include "api_umgmt.h"
#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_gen.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_user.h"
#include "../../log/log_api.h"
#include "../../server/conn_api.h"
#include "token.h"
#include "password.h"
#include <stdio.h>
#include <unistd.h>

DB_ID get_db_id();

/*
 * Method, body presence and JSON validity are guaranteed by the route flags in
 * api_umgmt_init(), so these handlers start at their actual business logic.
 */

static void m_send_token_cookie(http_request_ctx_t* ctx, HTTP_response_code_t code,
                                const char* body, const char* token)
{
    char extra[1280];

    snprintf(extra, sizeof(extra),
             "Set-Cookie: token=%s; HttpOnly; Path=/; SameSite=Strict\r\n", token);

    router_http_respond_ex(ctx->fd, code, body, ctx->origin, extra);
}

void api_umgmt_register(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    user_t new_user = {0};
    user_t* existing_user = NULL;
    char hashed[PASSWORD_HASH_MAX];
    const char* username;
    const char* email;
    const char* password;
    const char* first_name;
    const char* last_name;

    (void)user_data;

    if (!ctx_require_str(ctx, "username", &username)) return;
    if (!ctx_require_str(ctx, "email", &email)) return;
    if (!ctx_require_str(ctx, "password", &password)) return;
    if (!ctx_require_str(ctx, "first_name", &first_name)) return;
    if (!ctx_require_str(ctx, "last_name", &last_name)) return;

    db_select_user_by_username(get_db_id(), (char*)username, &existing_user);
    if (existing_user)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Username already exists\"}", ctx->origin);
        goto cleanup;
    }

    db_select_user_by_email(get_db_id(), (char*)email, &existing_user);
    if (existing_user)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Email already exists\"}", ctx->origin);
        goto cleanup;
    }

    if (password_hash(password, hashed, sizeof(hashed)) != SUCCESS)
    {
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
                                      "{\"error\":\"Failed to register user\"}", ctx->origin);
        goto cleanup;
    }

    new_user.username = (char*)username;
    new_user.email = (char*)email;
    new_user.first_name = (char*)first_name;
    new_user.last_name = (char*)last_name;
    new_user.password_hash = hashed;
    new_user.gender = "undefined";
    new_user.orientation = "undefined";
    new_user.bio = NULL;
    new_user.fame_rating = 0;
    new_user.gps_lat = 0.0;
    new_user.gps_lon = 0.0;
    new_user.location_optout = false;
    new_user.last_online = NULL;
    new_user.created_at = time(NULL);
    new_user.email_verified = false;

    if (db_tuser_insert_user(get_db_id(), &new_user) == ERROR)
    {
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
                                      "{\"error\":\"Failed to register user\"}", ctx->origin);
        goto cleanup;
    }

    router_http_generate_response(ctx->fd, CODE_201_CREATED,
        "{\"success\":true, \"message\":\"Registration successful\"}", ctx->origin);

cleanup:
    if (existing_user)
        db_tuser_free_user(existing_user);
}

void api_umgmt_login(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    user_t* existing_user = NULL;
    const char* email;
    const char* password;
    char token[1024];
    char rehashed[PASSWORD_HASH_MAX];
    bool upgrade_hash = false;

    (void)user_data;

    if (!ctx_require_str(ctx, "email", &email))
        return;
    if (!ctx_require_str(ctx, "password", &password))
        return;

    db_select_user_by_email(get_db_id(), (char*)email, &existing_user);

    /*
     * One generic answer for "no such email" and "wrong password" alike: telling
     * them apart lets anyone enumerate which addresses are registered.
     */
    if (!existing_user || !password_verify(password, existing_user->password_hash))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Invalid credentials\"}", ctx->origin);
        goto cleanup;
    }

    /* TODO not sending emails yet
    if (!existing_user->email_verified)
    {
        router_http_generate_response(ctx->fd, CODE_403_FORBIDDEN,
                                      "{\"error\":\"Email not verified\"}", ctx->origin);
        goto cleanup;
    }
    */

    /* Row still holds a pre-hashing plaintext password: replace it now. */
    if (password_is_legacy(existing_user->password_hash) &&
        (password_hash(password, rehashed, sizeof(rehashed)) == SUCCESS))
        upgrade_hash = true;

    if (token_generate(existing_user->username, existing_user->email,
                       existing_user->id, token, sizeof(token)) != SUCCESS)
    {
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
                                      "{\"error\":\"Failed to generate token\"}", ctx->origin);
        goto cleanup;
    }

    log_msg(LOG_LEVEL_DEBUG, "Generated token for user %s\n", existing_user->username);

    if (existing_user->token)
        free(existing_user->token);
    existing_user->token = strdup(token);

    if (upgrade_hash)
    {
        free(existing_user->password_hash);
        existing_user->password_hash = strdup(rehashed);
        log_msg(LOG_LEVEL_INFO, "Upgraded stored password for user %s to PBKDF2\n",
                existing_user->username);
    }

    if (db_tuser_update_user(get_db_id(), existing_user) == ERROR)
        log_msg(LOG_LEVEL_ERROR, "Failed to persist token for user %s\n", existing_user->username);

    m_send_token_cookie(ctx, CODE_200_OK,
                        "{\"success\":true, \"message\":\"Login successful\"}", token);

cleanup:
    if (existing_user)
        db_tuser_free_user(existing_user);
}

void api_umgmt_logout(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;

    (void)user_data;

    router_http_respond_ex(ctx->fd, CODE_200_OK,
        "{\"success\":true, \"message\":\"Logged out\"}", ctx->origin,
        "Set-Cookie: token=deleted; HttpOnly; Path=/; SameSite=Strict; "
        "Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");

    log_msg(LOG_LEVEL_DEBUG, "User logged out successfully\n");
}

void api_umgmt_validate(void* _ctx, void *user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    char response[256];

    (void)user_data;

    snprintf(response, sizeof(response),
             "{\"success\":true, \"message\":\"User is valid\", \"username\":\"%s\"}",
             ctx->username ? ctx->username : "unknown");

    router_http_generate_response(ctx->fd, CODE_200_OK, response, ctx->origin);
}

void api_umgmt_init()
{
    router_http_add("/api/register", api_umgmt_register, NULL, M_POST | BODY_JSON);
    router_http_add("/api/login",    api_umgmt_login,    NULL, M_POST | BODY_JSON);
    router_http_add("/api/validate", api_umgmt_validate, NULL, AUTH_REQUIRED | M_GET);
    router_http_add("/api/logout",   api_umgmt_logout,   NULL, AUTH_REQUIRED | M_POST);
}

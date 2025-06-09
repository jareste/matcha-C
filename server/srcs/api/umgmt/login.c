#include "api_umgmt_login.h"
#include "../../router/router_api.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"

void api_umgmt_login(http_request_ctx_t* ctx, void *user_data)
{
    char *origin = NULL;
    char *method = NULL;
    char *headers = NULL;
    char *route = NULL;
    char *body = NULL;
    (void)user_data;

    if (router_parse_http_request(ctx->request, ctx->request_len,
                                  &method, &route, &headers, &body) == ERROR)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
                                      "{\"error\":\"Bad Request\"}");
        goto cleanup;
    }

    if (strcmp(method, "POST")!=0)
    {
        router_http_generate_response(ctx->fd, CODE_405_METHOD_NOT_ALLOWED,
                                      "{\"error\":\"Method Not Allowed\"}");
        goto cleanup;
    }

    /* DO login logic :) */

    router_http_generate_response(ctx->fd, CODE_200_OK,
        "{\"success\":true, \"message\":\"Registration successful\"}");

cleanup:
    free(method);
    free(route);
    free(headers);
    free(body);
    free(origin);
}

#include "../../../third_party/multipart-parser-c/multipart_parser.h"
#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include "../../router/router_api.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_pic.h"
#include "../../log/log_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>

DB_ID get_db_id();

typedef struct
{
    FILE *file;
    char filename[256];
    char last_field[128];
} upload_ctx_t;

int on_header_field(multipart_parser* p, const char *at, size_t length)
{
    upload_ctx_t* ctx = multipart_parser_get_data(p);
    snprintf(ctx->last_field, sizeof(ctx->last_field), "%.*s", (int)length, at);
    return 0;
}

int on_header_value(multipart_parser* p, const char *at, size_t length)
{
    upload_ctx_t* ctx = multipart_parser_get_data(p);
    char buf[512];
    char* pos;
    char* end;

    snprintf(buf, sizeof(buf), "%.*s", (int)length, at);

    if (strcasecmp(ctx->last_field, "Content-Disposition") == 0)
    {
        pos = strstr(buf, "filename=");
        if (pos)
        {
            pos += 9;
            if (*pos == '"' || *pos == '\'') pos++;
            snprintf(ctx->filename, sizeof(ctx->filename), "%s/%s", PICS_DIR, pos);

            end = ctx->filename + strlen(ctx->filename) - 1;
            while (end > ctx->filename && (*end == '"' || *end == '\r' || *end == '\n'))
                *end-- = '\0';

            ctx->file = fopen(ctx->filename, "wb");
            log_msg(LOG_LEVEL_DEBUG, "Opening upload file %s\n", ctx->filename);
        }
    }
    return 0;
}


int on_part_data(multipart_parser* p, const char *at, size_t length)
{
    upload_ctx_t *ctx = multipart_parser_get_data(p);
    if (ctx->file)
        fwrite(at, 1, length, ctx->file);

    return 0;
}

int on_part_data_end(multipart_parser* p)
{
    upload_ctx_t *ctx = multipart_parser_get_data(p);
    if (ctx->file)
    {
        fclose(ctx->file);
        ctx->file = NULL;
    }
    return 0;
}

void pics_insert(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    char real_boundary[256];
    char *ctype;
    const char *b;
    multipart_parser *parser;
    upload_ctx_t upload = {0};

    (void)user_data;

    ctype = get_header_value(ctx->parsed_request.headers, "Content-Type");
    if (!ctype || !strstr(ctype, "multipart/form-data"))
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
            "{\"error\":\"Expected multipart/form-data\"}", NULL);
        free(ctype);
        return;
    }

    b = strstr(ctype, "boundary=");
    if (!b)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
            "{\"error\":\"Missing boundary\"}", NULL);
        free(ctype);
        return;
    }
    b += 9;

    multipart_parser_settings callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.on_header_field    = on_header_field;
    callbacks.on_header_value    = on_header_value;
    callbacks.on_part_data       = on_part_data;
    callbacks.on_part_data_end   = on_part_data_end;

    snprintf(real_boundary, sizeof(real_boundary), "--%s", b);

    parser = multipart_parser_init(real_boundary, &callbacks);

    multipart_parser_set_data(parser, &upload);

    multipart_parser_execute(parser, ctx->parsed_request.body, ctx->parsed_request.body_len);

    /* Ensure it's closure. */
    if (upload.file)
    {
        fclose(upload.file);
        upload.file = NULL;
    }

    multipart_parser_free(parser);

    if (upload.filename[0])
    {
        log_msg(LOG_LEVEL_DEBUG, "Uploaded file saved to %s\n", upload.filename);

        if (db_tpicture_insert_picture(get_db_id(), ctx->uid, upload.filename, false) == SUCCESS)
        {
            router_http_generate_response(ctx->fd, CODE_200_OK,
                "{\"message\": \"Picture uploaded successfully\"}", NULL);

            /* DEBUG */
            // picture_t_array *pics = db_tpicture_select_all(get_db_id());
            // if (pics) {
            //     log_msg(LOG_LEVEL_DEBUG, "Total pictures in DB: %zu\n", pics->count);
            //     for (size_t i = 0; i < pics->count; i++) {
            //         log_msg(LOG_LEVEL_DEBUG, "Pic ID %d: %s (User ID %d)\n",
            //                 pics->pictures[i]->id,
            //                 pics->pictures[i]->file_path,
            //                 pics->pictures[i]->user_id);
            //     }
            //     db_tpicture_free_array(pics);
            // } else {
            //     log_msg(LOG_LEVEL_DEBUG, "No pictures found in DB\n");
            // }
            /* DEBUG */
            
            free(ctype);
            return;
        }
        else
        {
            unlink(upload.filename);
            log_msg(LOG_LEVEL_ERROR, "Failed to insert picture record into DB\n");
        }
    }
    else
    {
        log_msg(LOG_LEVEL_ERROR, "No file uploaded\n");
    }

    free(ctype);

    router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
        "{\"error\": \"Failed to save picture\"}", NULL);
}

void pics_get(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    char* id_str;
    int id;
    picture_t* pic;
    FILE* f;
    long size;
    char* buf;
    char header[512];
    int header_len;

    if (!ctx) return;
    (void)user_data;

    id_str = strstr(ctx->query, "id=");
    if (!id_str)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
            "{\"error\": \"Missing 'id' parameter\"}", NULL);
        return;
    }

    id_str += 3;
    id = atoi(id_str);
    if (id <= 0)
    {
        router_http_generate_response(ctx->fd, CODE_400_BAD_REQUEST,
            "{\"error\": \"Invalid 'id' parameter\"}", NULL);
        return;
    }

    if (db_tpicture_select_picture_by_id(get_db_id(), id, &pic) != SUCCESS)
    {
        router_http_generate_response(ctx->fd, CODE_404_NOT_FOUND,
            "{\"error\": \"Picture not found\"}", NULL);
        return;
    }

    log_msg(LOG_LEVEL_DEBUG, "Serving picture ID %d from path %s\n", pic->id, pic->file_path);
    f = fopen(pic->file_path, "rb");
    if (!f)
    {
        router_http_generate_response(ctx->fd, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\": \"File missing on disk\"}", NULL);
        return;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    log_msg(LOG_LEVEL_DEBUG, "Picture file size: %ld bytes\n", size);

    buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        size);

    if (header_len <= 0 || (size_t)header_len >= sizeof(header))
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to format HTTP headers\n");
        free(buf);
        free(pic->file_path);
        free(pic);
        return;
    }

    send(ctx->fd, header, header_len, 0);
    send(ctx->fd, buf, size, 0);

    free(buf);
    free(pic->file_path);
    free(pic);
}

void api_pics_init()
{
    router_http_add("/api/pics/insert", pics_insert, NULL, AUTH_REQUIRED);
    router_http_add("/api/pics/get", pics_get, NULL, AUTH_REQUIRED);
}

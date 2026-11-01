#include "../../../third_party/multipart-parser-c/multipart_parser.h"
#include "../../../third_party/cJSON/cJSON.h"
#include "../../../inc/error_codes.h"
#include "../../router/router_api.h"
#include "../../db/db_api.h"
#include "../../db/tables/db_table_pic.h"
#include "../../log/log_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
/* DONT include ft_malloc.h here, we are needing original malloc() to handle the OUT_OF_MEM error */

DB_ID get_db_id();

#define MAX_PICS_PER_USER 5

#define MAX_PIC_BYTES (5 * 1024 * 1024)

#define MAGIC_BYTES 12

/*
 * Unique name format per picture: "u<uid>_<epoch>_<16 hex>.<ext>".
 * Only this basename goes in the database, the path is always built from PICS_DIR + stored_name.
 */
#define STORED_NAME_MAX 64

typedef struct
{
    const char* ext;
    const char* mime;
} img_type_t;

typedef struct
{
    char*  data; /* the file part, accumulated in memory */
    size_t len;
    size_t cap;
    bool in_file_part; /* the part currently being parsed carries a filename */
    bool have_file; /* a file part has already been taken */
    bool too_big;
    bool oom;
    char last_field[128];
} upload_ctx_t;

/*
 * Identifies the format from the leading bytes, not from the filename or the
 * part's Content-Type
 */
static const img_type_t* m_detect_image(const unsigned char* d, size_t n)
{
    static const img_type_t png  = { "png",  "image/png"  };
    static const img_type_t jpeg = { "jpg",  "image/jpeg" };
    static const img_type_t gif  = { "gif",  "image/gif"  };
    static const img_type_t webp = { "webp", "image/webp" };

    if (n >= 8 && memcmp(d, "\x89PNG\r\n\x1a\n", 8) == 0)
        return &png;

    if (n >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF)
        return &jpeg;

    if (n >= 6 && (memcmp(d, "GIF87a", 6) == 0 || memcmp(d, "GIF89a", 6) == 0))
        return &gif;

    if (n >= 12 && memcmp(d, "RIFF", 4) == 0 && memcmp(d + 8, "WEBP", 4) == 0)
        return &webp;

    return NULL;
}

/* Maps a stored name back to a MIME type for the GET response. */
static const char* m_mime_from_stored_name(const char* name)
{
    const char* dot = strrchr(name, '.');

    if (!dot)
        return "application/octet-stream";

    if (strcmp(dot, ".png") == 0)  return "image/png";
    if (strcmp(dot, ".jpg") == 0)  return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)  return "image/gif";
    if (strcmp(dot, ".webp") == 0) return "image/webp";

    return "application/octet-stream";
}

static int m_ensure_pics_dir(void)
{
    char path[512];
    char* p;

    if (snprintf(path, sizeof(path), "%s", PICS_DIR) >= (int)sizeof(path))
        return ERROR;

    for (p = path + 1; *p; p++)
    {
        if (*p != '/')
            continue;

        *p = '\0';
        if (mkdir(path, 0700) != 0 && errno != EEXIST)
            return ERROR;
        *p = '/';
    }

    if (mkdir(path, 0700) != 0 && errno != EEXIST)
        return ERROR;

    return SUCCESS;
}

/* Rejects anything that is not a plain basename, as defence in depth. */
static bool m_is_safe_stored_name(const char* name)
{
    if (!name || !*name)
        return false;

    if (strchr(name, '/') || strchr(name, '\\'))
        return false;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return false;

    return true;
}

static int m_build_pic_path(char* out, size_t out_len, const char* stored_name)
{
    int n;

    if (!m_is_safe_stored_name(stored_name))
        return ERROR;

    n = snprintf(out, out_len, "%s/%s", PICS_DIR, stored_name);
    if (n <= 0 || (size_t)n >= out_len)
        return ERROR;

    return SUCCESS;
}

static void m_respond_json(http_request_ctx_t* ctx, HTTP_response_code_t code, const char* body)
{
    router_http_generate_response(ctx->fd, code, body, ctx->origin);
}

static int m_on_header_field(multipart_parser* p, const char* at, size_t length)
{
    upload_ctx_t* up = multipart_parser_get_data(p);

    snprintf(up->last_field, sizeof(up->last_field), "%.*s", (int)length, at);
    return 0;
}

static int m_on_header_value(multipart_parser* p, const char* at, size_t length)
{
    upload_ctx_t* up = multipart_parser_get_data(p);
    char buf[512];

    if (strcasecmp(up->last_field, "Content-Disposition") != 0)
        return 0;

    snprintf(buf, sizeof(buf), "%.*s", (int)length, at);

    /*
     * The filename is only used to tell a file part from a plain field. Its
     * value is deliberately discarded: it used to be pasted straight into the
     * destination path, so "../../x" wrote wherever the server could reach.
     */
    if (strstr(buf, "filename=") && !up->have_file)
        up->in_file_part = true;

    return 0;
}

static int m_on_part_data(multipart_parser* p, const char* at, size_t length)
{
    upload_ctx_t* up = multipart_parser_get_data(p);
    size_t needed;
    size_t new_cap;
    char* tmp;

    if (!up->in_file_part || up->too_big || up->oom)
        return 0;

    needed = up->len + length;
    if (needed > MAX_PIC_BYTES)
    {
        up->too_big = true;
        return 0;
    }

    if (needed > up->cap)
    {
        new_cap = up->cap ? up->cap * 2 : 64 * 1024;

        while (new_cap < needed)
            new_cap *= 2;

        tmp = realloc(up->data, new_cap);
        if (!tmp)
        {
            up->oom = true;
            return 0;
        }

        up->data = tmp;
        up->cap = new_cap;
    }

    memcpy(up->data + up->len, at, length);
    up->len = needed;
    return 0;
}

static int m_on_part_data_end(multipart_parser* p)
{
    upload_ctx_t* up = multipart_parser_get_data(p);

    if (up->in_file_part)
    {
        up->in_file_part = false;
        up->have_file = true;
    }

    return 0;
}

static int m_parse_boundary(const char* ctype, char* out, size_t out_len)
{
    const char* b;
    const char* end;
    size_t len;

    b = strstr(ctype, "boundary=");
    if (!b)
        return ERROR;

    b += 9;

    if (*b == '"')
    {
        b++;
        end = strchr(b, '"');
        if (!end)
            return ERROR;
    }
    else
    {
        end = b;
        while (*end && *end != ';' && *end != ' ' && *end != '\t' &&
               *end != '\r' && *end != '\n')
            end++;
    }

    len = (size_t)(end - b);
    if (len == 0 || len + 3 > out_len)
        return ERROR;

    out[0] = '-';
    out[1] = '-';
    memcpy(out + 2, b, len);
    out[len + 2] = '\0';
    return SUCCESS;
}

static int m_count_user_pics(int uid, size_t* out_count)
{
    picture_t_array* pics;

    pics = db_tpicture_select_for_user(get_db_id(), uid);
    if (!pics)
        return ERROR;

    *out_count = pics->count;
    db_tpicture_free_array(pics);
    return SUCCESS;
}

static int m_make_stored_name(char* out, size_t out_len, int uid, const char* ext)
{
    unsigned char rnd[8];
    char hex[17];
    int fd;
    int i;
    int n;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return ERROR;

    if (read(fd, rnd, sizeof(rnd)) != (ssize_t)sizeof(rnd))
    {
        close(fd);
        return ERROR;
    }
    close(fd);

    for (i = 0; i < (int)sizeof(rnd); i++)
        sprintf(&hex[i * 2], "%02x", rnd[i]);
    hex[16] = '\0';

    n = snprintf(out, out_len, "u%d_%ld_%s.%s", uid, (long)time(NULL), hex, ext);
    if (n <= 0 || (size_t)n >= out_len)
        return ERROR;

    return SUCCESS;
}

static void pics_insert(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    multipart_parser_settings callbacks;
    multipart_parser* parser;
    upload_ctx_t upload;
    const img_type_t* type;
    char boundary[256];
    char stored_name[STORED_NAME_MAX];
    char path[512];
    char body[128];
    char* ctype;
    size_t parsed;
    size_t count;
    picture_t* created;
    bool first_picture;
    int fd;
    ssize_t written;

    (void)user_data;

    memset(&upload, 0, sizeof(upload));

    ctype = get_header_value(ctx->parsed_request.headers, "Content-Type");
    if (!ctype || !strstr(ctype, "multipart/form-data"))
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"Expected multipart/form-data\"}");
        free(ctype);
        return;
    }

    if (m_parse_boundary(ctype, boundary, sizeof(boundary)) != SUCCESS)
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"Missing or malformed boundary\"}");
        free(ctype);
        return;
    }

    free(ctype);

    /* Refuse before touching the body, so an over-quota upload costs nothing. */
    if (m_count_user_pics(ctx->uid, &count) != SUCCESS)
    {
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to read the existing pictures\"}");
        return;
    }

    if (count >= MAX_PICS_PER_USER)
    {
        snprintf(body, sizeof(body),
            "{\"error\":\"At most %d pictures; delete one first\"}", MAX_PICS_PER_USER);
        m_respond_json(ctx, CODE_409_CONFLICT, body);
        return;
    }

    first_picture = (count == 0);

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.on_header_field  = m_on_header_field;
    callbacks.on_header_value  = m_on_header_value;
    callbacks.on_part_data     = m_on_part_data;
    callbacks.on_part_data_end = m_on_part_data_end;

    parser = multipart_parser_init(boundary, &callbacks);
    if (!parser)
    {
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to start the multipart parser\"}");
        return;
    }

    multipart_parser_set_data(parser, &upload);
    parsed = multipart_parser_execute(parser, ctx->parsed_request.body,
                                      ctx->parsed_request.body_len);
    multipart_parser_free(parser);

    if (upload.oom)
    {
        free(upload.data);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Out of memory\"}");
        return;
    }

    if (upload.too_big)
    {
        free(upload.data);
        snprintf(body, sizeof(body),
            "{\"error\":\"Picture larger than %d bytes\"}", MAX_PIC_BYTES);
        m_respond_json(ctx, CODE_413_CONTENT_TOO_LARGE, body);
        return;
    }

    if (parsed != ctx->parsed_request.body_len)
    {
        free(upload.data);
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"Malformed multipart body\"}");
        return;
    }

    if (!upload.have_file || upload.len == 0)
    {
        free(upload.data);
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"No file part in the request\"}");
        return;
    }

    type = m_detect_image((const unsigned char*)upload.data,
                          upload.len < MAGIC_BYTES ? upload.len : MAGIC_BYTES);
    if (!type)
    {
        free(upload.data);
        m_respond_json(ctx, CODE_415_UNSUPPORTED_MEDIA_TYPE,
            "{\"error\":\"Only PNG, JPEG, GIF and WebP images are accepted\"}");
        return;
    }

    if (m_ensure_pics_dir() != SUCCESS)
    {
        log_msg(LOG_LEVEL_ERROR, "Cannot create upload directory %s: %s\n",
                PICS_DIR, strerror(errno));
        free(upload.data);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Storage unavailable\"}");
        return;
    }

    if (m_make_stored_name(stored_name, sizeof(stored_name), ctx->uid, type->ext) != SUCCESS ||
        m_build_pic_path(path, sizeof(path), stored_name) != SUCCESS)
    {
        free(upload.data);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to name the picture\"}");
        return;
    }

    /* O_EXCL -> if it already exists, fail  */
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
    {
        log_msg(LOG_LEVEL_ERROR, "Cannot create %s: %s\n", path, strerror(errno));
        free(upload.data);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to store the picture\"}");
        return;
    }

    written = write(fd, upload.data, upload.len);
    close(fd);
    free(upload.data);

    if (written != (ssize_t)upload.len)
    {
        log_msg(LOG_LEVEL_ERROR, "Short write to %s (%zd of %zu bytes)\n",
                path, written, upload.len);
        unlink(path);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to store the picture\"}");
        return;
    }

    if (db_tpicture_insert_picture(get_db_id(), ctx->uid, stored_name, first_picture) != SUCCESS)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to insert picture record for %s\n", stored_name);
        unlink(path);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to record the picture\"}");
        return;
    }

    log_msg(LOG_LEVEL_INFO, "Stored picture %s (%zu bytes, %s) for uid=%d\n",
            stored_name, upload.len, type->mime, ctx->uid);

    /* Hand back the id, which is all a client needs to fetch it again. */
    if (db_tpicture_select_picture_by_path(get_db_id(), stored_name, &created) == SUCCESS)
    {
        snprintf(body, sizeof(body), "{\"id\":%d,\"is_profile\":%s}",
                 created->id, created->is_profile ? "true" : "false");
        free(created->file_path);
        free(created);
    }
    else
    {
        snprintf(body, sizeof(body), "{\"is_profile\":%s}", first_picture ? "true" : "false");
    }

    m_respond_json(ctx, CODE_201_CREATED, body);
}

static void pics_get(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    picture_t* pic;
    struct stat st;
    char path[512];
    char* id_str;
    char* buf;
    size_t size;
    int id;
    int fd;
    ssize_t got;

    if (!ctx) return;
    (void)user_data;

    id_str = strstr(ctx->query, "id=");
    if (!id_str)
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST, "{\"error\": \"Missing 'id' parameter\"}");
        return;
    }

    id = atoi(id_str + 3);
    if (id <= 0)
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST, "{\"error\": \"Invalid 'id' parameter\"}");
        return;
    }

    if (db_tpicture_select_picture_by_id(get_db_id(), id, &pic) != SUCCESS)
    {
        m_respond_json(ctx, CODE_404_NOT_FOUND, "{\"error\": \"Picture not found\"}");
        return;
    }

    if (m_build_pic_path(path, sizeof(path), pic->file_path) != SUCCESS)
    {
        log_msg(LOG_LEVEL_WARN, "Picture %d has an unusable stored name '%s'\n",
                pic->id, pic->file_path);
        free(pic->file_path);
        free(pic);
        m_respond_json(ctx, CODE_404_NOT_FOUND, "{\"error\": \"Picture not found\"}");
        return;
    }

    free(pic->file_path);
    free(pic);

    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        log_msg(LOG_LEVEL_WARN, "Picture %d: %s is missing or not a regular file\n", id, path);
        m_respond_json(ctx, CODE_404_NOT_FOUND, "{\"error\": \"Picture not found\"}");
        return;
    }

    if (st.st_size <= 0 || st.st_size > MAX_PIC_BYTES)
    {
        log_msg(LOG_LEVEL_WARN, "Picture %d has an implausible size (%lld bytes)\n",
                id, (long long)st.st_size);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\": \"Stored picture is unusable\"}");
        return;
    }

    size = (size_t)st.st_size;

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\": \"Failed to read the picture\"}");
        return;
    }

    buf = malloc(size);
    if (!buf)
    {
        close(fd);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR, "{\"error\": \"Out of memory\"}");
        return;
    }

    got = read(fd, buf, size);
    close(fd);

    if (got != (ssize_t)size)
    {
        free(buf);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\": \"Failed to read the picture\"}");
        return;
    }

    router_http_respond_binary(ctx->fd, CODE_200_OK, buf, size,
                               m_mime_from_stored_name(path), ctx->origin);
    free(buf);
}

static int m_owned_picture_from_body(http_request_ctx_t* ctx, picture_t** out)
{
    cJSON* id_item;
    picture_t* pic;

    id_item = cJSON_GetObjectItemCaseSensitive(ctx->json, "id");
    if (!cJSON_IsNumber(id_item) || id_item->valueint <= 0)
    {
        m_respond_json(ctx, CODE_400_BAD_REQUEST,
            "{\"error\":\"Expected a positive integer 'id'\"}");
        return ERROR;
    }

    if (db_tpicture_select_picture_by_id(get_db_id(), id_item->valueint, &pic) != SUCCESS)
    {
        m_respond_json(ctx, CODE_404_NOT_FOUND, "{\"error\":\"Picture not found\"}");
        return ERROR;
    }

    /*
     * 404 rather than 403 for someone else's picture: a 403 would confirm that
     * the id exists, which is free enumeration of other users' pictures.
     */
    if (pic->user_id != ctx->uid)
    {
        log_msg(LOG_LEVEL_WARN, "uid=%d tried to act on picture %d owned by uid=%d\n",
                ctx->uid, pic->id, pic->user_id);
        free(pic->file_path);
        free(pic);
        m_respond_json(ctx, CODE_404_NOT_FOUND, "{\"error\":\"Picture not found\"}");
        return ERROR;
    }

    *out = pic;
    return SUCCESS;
}

static void pics_set_profile(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    picture_t_array* pics;
    picture_t* target;
    size_t i;
    bool failed = false;

    (void)user_data;

    if (m_owned_picture_from_body(ctx, &target) != SUCCESS)
        return;

    pics = db_tpicture_select_for_user(get_db_id(), ctx->uid);
    if (!pics)
    {
        free(target->file_path);
        free(target);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to read the existing pictures\"}");
        return;
    }

    /* Exactly one picture may carry the flag, so rewrite the whole set. */
    for (i = 0; i < pics->count; i++)
    {
        bool want = (pics->pictures[i]->id == target->id);

        if (pics->pictures[i]->is_profile == want)
            continue;

        pics->pictures[i]->is_profile = want;
        if (db_tpicture_update_picture(get_db_id(), pics->pictures[i]) != SUCCESS)
            failed = true;
    }

    db_tpicture_free_array(pics);

    if (failed)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to move the profile flag to picture %d\n", target->id);
        free(target->file_path);
        free(target);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to set the profile picture\"}");
        return;
    }

    log_msg(LOG_LEVEL_INFO, "uid=%d set picture %d as the profile picture\n",
            ctx->uid, target->id);
    free(target->file_path);
    free(target);
    m_respond_json(ctx, CODE_200_OK, "{\"message\":\"Profile picture updated\"}");
}

static void pics_delete(void* _ctx, void* user_data)
{
    http_request_ctx_t* ctx = (http_request_ctx_t*)_ctx;
    picture_t_array* pics;
    picture_t* target;
    char path[512];
    bool was_profile;
    size_t i;

    (void)user_data;

    if (m_owned_picture_from_body(ctx, &target) != SUCCESS)
        return;

    if (db_tpicture_delete_picture_from_pk(get_db_id(), target->id) != SUCCESS)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to delete picture %d\n", target->id);
        free(target->file_path);
        free(target);
        m_respond_json(ctx, CODE_500_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Failed to delete the picture\"}");
        return;
    }

    if (m_build_pic_path(path, sizeof(path), target->file_path) == SUCCESS &&
        unlink(path) != 0 && errno != ENOENT)
    {
        log_msg(LOG_LEVEL_WARN, "Deleted picture %d but could not unlink %s: %s\n",
                target->id, path, strerror(errno));
    }

    was_profile = target->is_profile;
    log_msg(LOG_LEVEL_INFO, "uid=%d deleted picture %d\n", ctx->uid, target->id);
    free(target->file_path);
    free(target);

    /* If the deleted picture was the profile picture, we update the user's profile. */
    if (was_profile)
    {
        pics = db_tpicture_select_for_user(get_db_id(), ctx->uid);
        if (pics && pics->count > 0)
        {
            for (i = 0; i < pics->count; i++)
            {
                if (pics->pictures[i]->is_profile)
                    break;
            }

            if (i == pics->count)
            {
                pics->pictures[0]->is_profile = true;
                if (db_tpicture_update_picture(get_db_id(), pics->pictures[0]) != SUCCESS)
                    log_msg(LOG_LEVEL_WARN, "uid=%d has no profile picture left\n", ctx->uid);
            }
        }
        db_tpicture_free_array(pics);
    }

    m_respond_json(ctx, CODE_200_OK, "{\"message\":\"Picture deleted\"}");
}

void api_pics_init()
{
    /* Create the directory at startup too, so a misconfigured path shows up in
     */
    if (m_ensure_pics_dir() != SUCCESS)
        log_msg(LOG_LEVEL_ERROR, "Upload directory %s is not usable: %s\n",
                PICS_DIR, strerror(errno));

    /* Upload is multipart, not JSON: require a body but do not parse it. */
    router_http_add("/api/pics/insert", pics_insert, NULL, AUTH_REQUIRED | M_POST | BODY_REQUIRED);
    router_http_add("/api/pics/get", pics_get, NULL, AUTH_REQUIRED | M_GET | QUERY_NEEDED);
    router_http_add("/api/pics/set_profile", pics_set_profile, NULL, AUTH_REQUIRED | M_POST | BODY_JSON);
    router_http_add("/api/pics/delete", pics_delete, NULL, AUTH_REQUIRED | M_POST | BODY_JSON);
}

#include "../../../inc/error_codes.h"
#include "../../../inc/ft_malloc.h"
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

#define FIXED_KEY "s3cr3t!KEY#123"

static int base64_encode(const unsigned char* input, size_t len, char* output, size_t outlen)
{
    BIO *bmem = NULL, *b64 = NULL;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) return ERROR;
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    if (!bmem)
    {
        BIO_free(b64);
        return ERROR;
    }
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, len);
    if (BIO_flush(b64) != 1)
    {
        BIO_free_all(b64);
        return ERROR;
    }
    BIO_get_mem_ptr(b64, &bptr);
    if (bptr->length >= outlen)
    {
        BIO_free_all(b64);
        return ERROR;
    }
    memcpy(output, bptr->data, bptr->length);
    output[bptr->length] = '\0';
    BIO_free_all(b64);
    return SUCCESS;
}

static int base64_decode(const char* input, unsigned char* output, size_t outlen)
{
    BIO *b64, *bmem;
    int decoded_len;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) return ERROR;
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bmem = BIO_new_mem_buf((void*)input, -1);
    if (!bmem)
    {
        BIO_free(b64);
        return ERROR;
    }

    bmem = BIO_push(b64, bmem);
    decoded_len = BIO_read(bmem, output, outlen);
    if (decoded_len <= 0)
    {
        BIO_free_all(bmem);
        return ERROR;
    }

    output[decoded_len] = '\0';
    BIO_free_all(bmem);
    return decoded_len;
}

int token_generate(const char* username, const char* email, int uid, char* out_token, size_t out_token_len)
{
    char payload[512];
    char full_token[1024];
    unsigned char* hmac;
    size_t payload_len;
    size_t full_len;
    int i;
    time_t timestamp;
    
    if (!username || !email || !out_token || out_token_len == 0)
        return ERROR;

    timestamp = time(NULL);
    snprintf(payload, sizeof(payload), "%s|%s|%d|%ld", username, email, uid, timestamp);

    hmac = HMAC(EVP_sha256(),
                FIXED_KEY, strlen(FIXED_KEY),
                (unsigned char*)payload, strlen(payload),
                NULL, NULL);
    if (!hmac) return ERROR;

    payload_len = strlen(payload);
    memcpy(full_token, payload, payload_len);
    full_token[payload_len] = '|';

    for (i = 0; i < 32; ++i)
        sprintf(&full_token[payload_len + 1 + i * 2], "%02x", hmac[i]);

    full_len = payload_len + 1 + 64;
    if (base64_encode((unsigned char*)full_token, full_len, out_token, out_token_len) != SUCCESS)
        return ERROR;

    return SUCCESS;
}

int token_validate(const char* token, char** out_username, char** out_email, int* out_uid)
{
    char recomputed_hex[65];
    char buffer[1024];
    char payload[512];
    unsigned char decoded[1024];
    char* username;
    char* email;
    char* uid_str;
    char* timestamp;
    char* token_hmac;
    unsigned char* hmac;
    int decoded_len;
    int i;

    if (!token || !out_username || !out_email || !out_uid)
        return ERROR;

    decoded_len = base64_decode(token, decoded, sizeof(decoded) - 1);
    if (decoded_len <= 0)
        return ERROR;

    decoded[decoded_len] = '\0';

    strncpy(buffer, (char*)decoded, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    username = strtok(buffer, "|");
    email = strtok(NULL, "|");
    uid_str = strtok(NULL, "|");
    timestamp = strtok(NULL, "|");
    token_hmac = strtok(NULL, "|");

    if (!username || !email || !uid_str || !timestamp || !token_hmac)
        return ERROR;

    snprintf(payload, sizeof(payload), "%s|%s|%s|%s", username, email, uid_str, timestamp);

    hmac = HMAC(EVP_sha256(),
                FIXED_KEY, strlen(FIXED_KEY),
                (unsigned char*)payload, strlen(payload),
                NULL, NULL);
    if (!hmac)
        return ERROR;

    for (i = 0; i < 32; ++i)
        sprintf(&recomputed_hex[i * 2], "%02x", hmac[i]);
    recomputed_hex[64] = '\0';

    if (strncmp(token_hmac, recomputed_hex, 64) != 0)
        return ERROR;

    *out_uid = atoi(uid_str);
    *out_username = strdup(username);
    *out_email = strdup(email);

    return SUCCESS;
}

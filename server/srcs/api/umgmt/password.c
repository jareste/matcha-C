#include <stdio.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "../../../inc/error_codes.h"
#include "../../log/log_api.h"
#include "password.h"

#define PW_PREFIX      "pbkdf2_sha256$"
#define PW_SALT_LEN    16
#define PW_DIGEST_LEN  32

/*
 * Iteration count. Higher is better against offline cracking, but this server
 * hashes on its single epoll thread, so the cost is paid by every other
 * connection waiting for the loop. 210k keeps a login near 100ms on the
 * hardware this was written for; measure before raising it.
 */
#define PW_ITERATIONS  210000

static void m_to_hex(const unsigned char *in, size_t len, char *out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < len; i++)
    {
        out[i * 2]     = digits[in[i] >> 4];
        out[i * 2 + 1] = digits[in[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

static int m_from_hex(const char *in, unsigned char *out, size_t out_len)
{
    size_t i;

    if (strlen(in) != (out_len * 2))
        return ERROR;

    for (i = 0; i < out_len; i++)
    {
        unsigned int byte;

        if (sscanf(in + i * 2, "%2x", &byte) != 1)
            return ERROR;

        out[i] = (unsigned char)byte;
    }

    return SUCCESS;
}

static int m_derive(const char *password, const unsigned char *salt, size_t salt_len,
                    unsigned int iterations, unsigned char *out, size_t out_len)
{
    if (PKCS5_PBKDF2_HMAC(password, (int)strlen(password),
                          salt, (int)salt_len, (int)iterations,
                          EVP_sha256(), (int)out_len, out) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "PBKDF2 derivation failed\n");
        return ERROR;
    }

    return SUCCESS;
}

int password_hash(const char *password, char *out, size_t out_size)
{
    unsigned char salt[PW_SALT_LEN];
    unsigned char digest[PW_DIGEST_LEN];
    char salt_hex[PW_SALT_LEN * 2 + 1];
    char digest_hex[PW_DIGEST_LEN * 2 + 1];
    int n;

    if (!password || !out || (out_size == 0))
        return ERROR;

    if (RAND_bytes(salt, sizeof(salt)) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "Cannot generate a password salt\n");
        return ERROR;
    }

    if (m_derive(password, salt, sizeof(salt), PW_ITERATIONS, digest, sizeof(digest)) != SUCCESS)
        return ERROR;

    m_to_hex(salt, sizeof(salt), salt_hex);
    m_to_hex(digest, sizeof(digest), digest_hex);

    n = snprintf(out, out_size, PW_PREFIX "%u$%s$%s", PW_ITERATIONS, salt_hex, digest_hex);
    if ((n <= 0) || ((size_t)n >= out_size))
    {
        log_msg(LOG_LEVEL_ERROR, "Password hash buffer too small (%d needed)\n", n);
        OPENSSL_cleanse(out, out_size);
        return ERROR;
    }

    return SUCCESS;
}

bool password_is_legacy(const char *stored)
{
    return stored && (strncmp(stored, PW_PREFIX, strlen(PW_PREFIX)) != 0);
}

bool password_verify(const char *password, const char *stored)
{
    unsigned char salt[PW_SALT_LEN];
    unsigned char expected[PW_DIGEST_LEN];
    unsigned char actual[PW_DIGEST_LEN];
    char salt_hex[PW_SALT_LEN * 2 + 1];
    char digest_hex[PW_DIGEST_LEN * 2 + 1];
    unsigned int iterations = 0;
    const char *fields;
    size_t a;
    size_t b;

    if (!password || !stored)
        return false;

    if (password_is_legacy(stored))
    {
        a = strlen(password);
        b = strlen(stored);

        log_msg(LOG_LEVEL_WARN,
                "Authenticating against a legacy plaintext password; it will be re-hashed\n");

        /* Same length check first: CRYPTO_memcmp needs equal lengths. */
        return (a == b) && CRYPTO_memcmp(password, stored, a) == 0;
    }

    fields = stored + strlen(PW_PREFIX);
    if (sscanf(fields, "%u$%32[0-9a-f]$%64[0-9a-f]", &iterations, salt_hex, digest_hex) != 3)
    {
        log_msg(LOG_LEVEL_ERROR, "Stored password hash is malformed\n");
        return false;
    }

    if ((iterations == 0) || (iterations > 10000000))
    {
        log_msg(LOG_LEVEL_ERROR, "Stored password hash has an implausible iteration count\n");
        return false;
    }

    if ((m_from_hex(salt_hex, salt, sizeof(salt)) != SUCCESS) ||
        (m_from_hex(digest_hex, expected, sizeof(expected)) != SUCCESS))
        return false;

    if (m_derive(password, salt, sizeof(salt), iterations, actual, sizeof(actual)) != SUCCESS)
        return false;

    return (CRYPTO_memcmp(actual, expected, sizeof(actual)) == 0);
}

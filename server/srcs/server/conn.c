#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "../../inc/error_codes.h"
#include "../log/log_api.h"
#include "conn_api.h"

#ifdef USE_SSL
# include <openssl/err.h>
# include <openssl/ssl.h>
#endif

#define CONN_TX_INITIAL_CAP   4096
#define CONN_TX_MAX_CAP       (16 * 1024 * 1024)
#define CONN_READ_EXACT_MS    5000
/* SSL_read/SSL_write take an int length; clamp so a huge size_t cannot wrap. */
#define CONN_SSL_MAX_CHUNK    (1 << 20)

typedef struct
{
    int     fd;
    bool    handshake_done;

    char*   tx_buf;
    size_t  tx_len;
    size_t  tx_cap;

#ifdef USE_SSL
    SSL*    ssl;
#endif
} conn_t;

/*
 * Slot table indexed directly by fd. Response handlers deep in the api/ modules
 * only ever carry an int fd around, so the lookup has to work from that alone.
 */
static conn_t**    m_conns = NULL;
static size_t      m_conns_cap = 0;
static conn_mode_t m_mode = CONN_MODE_PLAIN;

#ifdef USE_SSL
static SSL_CTX*    m_ssl_ctx = NULL;
#endif

/********************************************/
/* slot table                               */
/********************************************/
static conn_t* m_conn_get(int fd)
{
    if (fd < 0 || (size_t)fd >= m_conns_cap)
        return NULL;

    return m_conns[fd];
}

static conn_t* m_conn_new_slot(int fd)
{
    conn_t* c;

    if (fd < 0)
        return NULL;

    if ((size_t)fd >= m_conns_cap)
    {
        size_t new_cap = m_conns_cap ? m_conns_cap * 2 : 64;
        conn_t** tmp;

        while ((size_t)fd >= new_cap)
            new_cap *= 2;

        tmp = realloc(m_conns, new_cap * sizeof(*tmp));
        if (!tmp)
            return NULL;

        memset(tmp + m_conns_cap, 0, (new_cap - m_conns_cap) * sizeof(*tmp));
        m_conns = tmp;
        m_conns_cap = new_cap;
    }

    /* A stale slot means a previous owner of this fd was never unregistered. */
    if (m_conns[fd])
    {
        log_msg(LOG_LEVEL_WARN, "conn: slot for fd=%d still occupied, reclaiming\n", fd);
        conn_unregister(fd);
    }

    c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    c->fd = fd;
    m_conns[fd] = c;

    return c;
}

/********************************************/
/* layer lifecycle                          */
/********************************************/
bool conn_layer_tls_available(void)
{
#ifdef USE_SSL
    return true;
#else
    return false;
#endif
}

conn_mode_t conn_layer_mode(void)
{
    return m_mode;
}

int conn_layer_init(conn_mode_t mode, const char *cert_path, const char *key_path)
{
    if (mode == CONN_MODE_PLAIN)
    {
        m_mode = CONN_MODE_PLAIN;
        log_msg(LOG_LEVEL_BOOT, "conn: transport = plain TCP\n");
        return SUCCESS;
    }

#ifndef USE_SSL
    (void)cert_path;
    (void)key_path;
    log_msg(LOG_LEVEL_ERROR,
            "conn: SSL_ENABLE is set but this binary was built without -DUSE_SSL; "
            "rebuild with USE_SSL in CFLAGS or set SSL_ENABLE=n\n");
    return ERROR;
#else
    if (!cert_path || !key_path)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: TLS requested but CERT_PATH/KEY_PATH are unset\n");
        return ERROR;
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    m_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!m_ssl_ctx)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: SSL_CTX_new failed: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return ERROR;
    }

    SSL_CTX_set_min_proto_version(m_ssl_ctx, TLS1_2_VERSION);
    SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_COMPRESSION);
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
    /* Report a missing close_notify as EOF, the way OpenSSL 1.1.1 did. */
    SSL_CTX_set_options(m_ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

    /*
     * Without this, a partial SSL_write would have to be retried with the exact
     * same pointer; the tx queue memmoves its buffer, so let OpenSSL cope.
     */
    SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                                SSL_MODE_ENABLE_PARTIAL_WRITE);

    if (SSL_CTX_use_certificate_chain_file(m_ssl_ctx, cert_path) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: cannot load certificate '%s': %s\n",
                cert_path, ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = NULL;
        return ERROR;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, key_path, SSL_FILETYPE_PEM) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: cannot load private key '%s': %s\n",
                key_path, ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = NULL;
        return ERROR;
    }

    if (SSL_CTX_check_private_key(m_ssl_ctx) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: private key does not match certificate\n");
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = NULL;
        return ERROR;
    }

    m_mode = CONN_MODE_TLS;
    log_msg(LOG_LEVEL_BOOT, "conn: transport = TLS (cert=%s key=%s)\n", cert_path, key_path);

    return SUCCESS;
#endif
}

void conn_layer_cleanup(void)
{
    size_t i;

    for (i = 0; i < m_conns_cap; i++)
    {
        if (m_conns[i])
            conn_unregister((int)i);
    }

    free(m_conns);
    m_conns = NULL;
    m_conns_cap = 0;

#ifdef USE_SSL
    if (m_ssl_ctx)
    {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = NULL;
    }
#endif
}

/********************************************/
/* per-connection lifecycle                 */
/********************************************/
int conn_register(int fd)
{
    conn_t* c = m_conn_new_slot(fd);

    if (!c)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: out of memory registering fd=%d\n", fd);
        return ERROR;
    }

    if (m_mode == CONN_MODE_PLAIN)
    {
        c->handshake_done = true;
        return SUCCESS;
    }

#ifdef USE_SSL
    c->ssl = SSL_new(m_ssl_ctx);
    if (!c->ssl)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: SSL_new failed for fd=%d: %s\n",
                fd, ERR_error_string(ERR_get_error(), NULL));
        free(c);
        m_conns[fd] = NULL;
        return ERROR;
    }

    if (SSL_set_fd(c->ssl, fd) != 1)
    {
        log_msg(LOG_LEVEL_ERROR, "conn: SSL_set_fd failed for fd=%d\n", fd);
        SSL_free(c->ssl);
        free(c);
        m_conns[fd] = NULL;
        return ERROR;
    }

    SSL_set_accept_state(c->ssl);
#endif

    return SUCCESS;
}

void conn_unregister(int fd)
{
    conn_t* c = m_conn_get(fd);

    if (!c)
        return;

#ifdef USE_SSL
    if (c->ssl)
    {
        /*
         * One non-blocking close_notify. We do not wait for the peer's reply:
         * the fd is about to be closed and a half-done bidirectional shutdown
         * is not worth stalling the event loop for.
         */
        if (c->handshake_done)
            SSL_shutdown(c->ssl);

        SSL_free(c->ssl);
        c->ssl = NULL;
    }
#endif

    free(c->tx_buf);
    free(c);
    m_conns[fd] = NULL;
}

/********************************************/
/* handshake                                */
/********************************************/
int conn_handshake(int fd, bool *want_write)
{
    conn_t* c = m_conn_get(fd);

    if (want_write)
        *want_write = false;

    if (!c)
        return CONN_ERROR;

    if (c->handshake_done)
        return SUCCESS;

#ifdef USE_SSL
    {
        int ret = SSL_accept(c->ssl);
        int err;

        if (ret == 1)
        {
            c->handshake_done = true;
            log_msg(LOG_LEVEL_INFO, "conn: TLS handshake done on fd=%d (%s, %s)\n",
                    fd, SSL_get_version(c->ssl), SSL_get_cipher(c->ssl));
            return SUCCESS;
        }

        err = SSL_get_error(c->ssl, ret);
        if (err == SSL_ERROR_WANT_READ)
            return CONN_AGAIN;

        if (err == SSL_ERROR_WANT_WRITE)
        {
            if (want_write)
                *want_write = true;
            return CONN_AGAIN;
        }

        log_msg(LOG_LEVEL_WARN, "conn: TLS handshake failed on fd=%d: %s\n",
                fd, ERR_error_string(ERR_get_error(), NULL));
        return CONN_ERROR;
    }
#else
    return CONN_ERROR;
#endif
}

/********************************************/
/* read path                                */
/********************************************/
ssize_t conn_read(int fd, void *buf, size_t len)
{
    conn_t* c = m_conn_get(fd);
    ssize_t n;

    if (!c || !buf)
        return CONN_ERROR;

    if (len == 0)
        return 0;

#ifdef USE_SSL
    if (c->ssl)
    {
        int ret = SSL_read(c->ssl, buf, (int)(len > CONN_SSL_MAX_CHUNK ? CONN_SSL_MAX_CHUNK : len));
        int err;

        if (ret > 0)
            return ret;

        err = SSL_get_error(c->ssl, ret);
        switch (err)
        {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return CONN_AGAIN;
        case SSL_ERROR_ZERO_RETURN:
            return CONN_CLOSED;
        case SSL_ERROR_SYSCALL:
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return CONN_AGAIN;
            if (errno == EINTR)
                return CONN_AGAIN;
            /* errno == 0 here means the peer vanished without close_notify. */
            return (errno == 0) ? CONN_CLOSED : CONN_ERROR;
        case SSL_ERROR_SSL:
        {
            unsigned long reason = ERR_GET_REASON(ERR_peek_last_error());

            if (reason == SSL_R_UNEXPECTED_EOF_WHILE_READING ||
                reason == SSL_R_SHUTDOWN_WHILE_IN_INIT)
            {
                ERR_clear_error();
                return CONN_CLOSED;
            }

            log_msg(LOG_LEVEL_DEBUG, "conn: SSL_read protocol error on fd=%d: %s\n",
                    fd, ERR_error_string(ERR_get_error(), NULL));
            return CONN_ERROR;
        }
        default:
            log_msg(LOG_LEVEL_DEBUG, "conn: SSL_read error on fd=%d: %s\n",
                    fd, ERR_error_string(ERR_get_error(), NULL));
            return CONN_ERROR;
        }
    }
#endif

    for (;;)
    {
        n = recv(fd, buf, len, 0);

        if (n > 0)
            return n;

        if (n == 0)
            return CONN_CLOSED;

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return CONN_AGAIN;

        return CONN_ERROR;
    }
}

int conn_read_exact(int fd, void *buf, size_t len)
{
    size_t got = 0;
    int waited_ms = 0;
    int pr;

    while (got < len)
    {
        ssize_t n = conn_read(fd, (char*)buf + got, len - got);

        if (n > 0)
        {
            got += (size_t)n;
            continue;
        }

        if (n == CONN_AGAIN)
        {
            struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };

            if (waited_ms >= CONN_READ_EXACT_MS)
            {
                log_msg(LOG_LEVEL_WARN,
                        "conn: timed out waiting for %zu/%zu bytes on fd=%d\n", got, len, fd);
                return ERROR;
            }

            pr = poll(&pfd, 1, 100);
            if (pr < 0 && errno != EINTR)
                return ERROR;

            if (pr == 0)
                waited_ms += 100;

            continue;
        }

        return ERROR;
    }

    return SUCCESS;
}

/********************************************/
/* write path                               */
/********************************************/
static int m_tx_append(conn_t *c, const void *buf, size_t len)
{
    size_t new_cap;
    char* tmp;
    size_t need = c->tx_len + len;

    if (need > c->tx_cap)
    {
        new_cap = c->tx_cap ? c->tx_cap : CONN_TX_INITIAL_CAP;

        while (new_cap < need)
            new_cap *= 2;

        if (new_cap > CONN_TX_MAX_CAP)
        {
            log_msg(LOG_LEVEL_ERROR,
                    "conn: tx queue for fd=%d would exceed %d bytes, dropping\n",
                    c->fd, CONN_TX_MAX_CAP);
            return ERROR;
        }

        tmp = realloc(c->tx_buf, new_cap);
        if (!tmp)
            return ERROR;

        c->tx_buf = tmp;
        c->tx_cap = new_cap;
    }

    memcpy(c->tx_buf + c->tx_len, buf, len);
    c->tx_len = need;

    return SUCCESS;
}

/* Writes as much of the queue as the transport will take right now. */
static int m_tx_flush(conn_t *c, bool *want_write)
{
    ssize_t n;

    if (want_write)
        *want_write = false;

    while (c->tx_len > 0)
    {
#ifdef USE_SSL
        if (c->ssl)
        {
            int ret;
            int err;

            if (!c->handshake_done)
            {
                if (want_write)
                    *want_write = true;
                return SUCCESS; /* nothing can go out before the handshake */
            }

            ret = SSL_write(c->ssl, c->tx_buf,
                            (int)(c->tx_len > CONN_SSL_MAX_CHUNK ? CONN_SSL_MAX_CHUNK : c->tx_len));
            if (ret > 0)
            {
                n = ret;
            }
            else
            {
                err = SSL_get_error(c->ssl, ret);

                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                {
                    if (want_write)
                        *want_write = true;
                    return SUCCESS;
                }

                if (err == SSL_ERROR_SYSCALL &&
                    (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
                {
                    if (want_write)
                        *want_write = true;
                    return SUCCESS;
                }

                log_msg(LOG_LEVEL_DEBUG, "conn: SSL_write error on fd=%d: %s\n",
                        c->fd, ERR_error_string(ERR_get_error(), NULL));
                return ERROR;
            }
        }
        else
#endif
        {
            n = send(c->fd, c->tx_buf, c->tx_len, MSG_NOSIGNAL);
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    if (want_write)
                        *want_write = true;
                    return SUCCESS;
                }

                return ERROR;
            }
        }

        if ((size_t)n < c->tx_len)
            memmove(c->tx_buf, c->tx_buf + n, c->tx_len - (size_t)n);

        c->tx_len -= (size_t)n;
    }

    return SUCCESS;
}

ssize_t conn_write(int fd, const void *buf, size_t len)
{
    conn_t* c = m_conn_get(fd);

    if (!c || !buf)
        return CONN_ERROR;

    if (len == 0)
        return 0;

    if (m_tx_append(c, buf, len) != SUCCESS)
        return CONN_ERROR;

    if (m_tx_flush(c, NULL) != SUCCESS)
        return CONN_ERROR;

    return (ssize_t)len;
}

ssize_t conn_writev2(int fd, const void *a, size_t alen, const void *b, size_t blen)
{
    conn_t* c = m_conn_get(fd);

    if (!c)
        return CONN_ERROR;

    /* Queued together so the two parts leave in a single transport write. */
    if (a && alen && m_tx_append(c, a, alen) != SUCCESS)
        return CONN_ERROR;

    if (b && blen && m_tx_append(c, b, blen) != SUCCESS)
        return CONN_ERROR;

    if (m_tx_flush(c, NULL) != SUCCESS)
        return CONN_ERROR;

    return (ssize_t)(alen + blen);
}

int conn_printf(int fd, const char *fmt, ...)
{
    char    stack_buf[1024];
    char*   buf = stack_buf;
    va_list ap;
    int     n;
    ssize_t w;

    va_start(ap, fmt);
    n = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
    va_end(ap);

    if (n < 0)
        return ERROR;

    if ((size_t)n >= sizeof(stack_buf))
    {
        buf = malloc((size_t)n + 1);
        if (!buf)
            return ERROR;

        va_start(ap, fmt);
        n = vsnprintf(buf, (size_t)n + 1, fmt, ap);
        va_end(ap);

        if (n < 0)
        {
            free(buf);
            return ERROR;
        }
    }

    {
        w = conn_write(fd, buf, (size_t)n);

        if (buf != stack_buf)
            free(buf);

        return (w < 0) ? ERROR : SUCCESS;
    }
}

int conn_flush(int fd, bool *want_write)
{
    conn_t* c = m_conn_get(fd);

    if (!c)
        return ERROR;

    return m_tx_flush(c, want_write);
}

bool conn_has_pending_tx(int fd)
{
    conn_t* c = m_conn_get(fd);

    return c && c->tx_len > 0;
}

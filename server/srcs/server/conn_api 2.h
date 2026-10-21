#ifndef CONN_API_H
#define CONN_API_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef enum
{
    CONN_MODE_PLAIN = 0,
    CONN_MODE_TLS   = 1
} conn_mode_t;

#define CONN_AGAIN   (-1) /* EWOULDBLOCK -> dont close, wait for more data */
#define CONN_CLOSED  (-2)
#define CONN_ERROR   (-3)

/* TLS layer status checks */
int         conn_layer_init(conn_mode_t mode, const char *cert_path, const char *key_path);
void        conn_layer_cleanup(void);
conn_mode_t conn_layer_mode(void);
bool        conn_layer_tls_available(void); /* was this binary built with -DUSE_SSL? */

int  conn_register(int fd);   /* call right after accept() (registers into the TLS context) */
void conn_unregister(int fd); /* TLS close_notify + free state (just releases from TLS context) */

/*
 * Drive the TLS handshake; a no-op returning SUCCESS in plain mode. Returns
 * CONN_AGAIN while the handshake is still in flight, and sets *want_write when
 * it is blocked on writing rather than reading, so the caller can re-arm epoll
 * with the event the handshake is actually waiting for.
 */
int conn_handshake(int fd, bool *want_write);

/* --- data path --- */
ssize_t conn_read(int fd, void *buf, size_t len);

/* Write data to the connection; returns the number of bytes written, or CONN_ERROR.
 * Receiving the whole number of bytes doesnt mean we just sent them. Means that at least
 * we queued them for sending. The actual send may be delayed by the OS or TLS layer.
 * The caller should call conn_flush() to ensure the data is actually sent.
 * conn_writev2() is a variant that takes two buffers and sends them together in one transport write.
 * conn_printf() is a convenience wrapper that formats a string and sends it.
 */
ssize_t conn_write(int fd, const void *buf, size_t len);
ssize_t conn_writev2(int fd, const void *a, size_t alen, const void *b, size_t blen);
int     conn_printf(int fd, const char *fmt, ...);

/* Blocking read of exactly len bytes */
int conn_read_exact(int fd, void *buf, size_t len);

/* used for flushing the tx in case of some kind of '-1' EWOULDBLOCK-like */
int  conn_flush(int fd, bool *want_write);
bool conn_has_pending_tx(int fd);

#endif /* CONN_API_H */

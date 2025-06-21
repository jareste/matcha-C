#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"
#include "../../inc/ft_list.h"
#include "../log/log_api.h"
#include "../../third_party/uthash-master/src/uthash.h"
#include "server_api.h"

/* defines */
#define SERVER_KEY "SOME_KEY"
#define MAX_LOGIN_ROLES 3

#define REMOVE_CLIENT(fd)                                  \
  do {                                                     \
    client_t *_c;                                          \
    HASH_FIND_INT(clients, &fd, _c);                       \
    if (_c)                                                \
    {                                                      \
      HASH_DEL(clients, _c);                               \
      if (_c->sio_sid)                                     \
        free(_c->sio_sid);                                 \
      free(_c);                                            \
    }                                                      \
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);        \
    log_msg(LOG_LEVEL_INFO, "Removing client %d (invoked from %s:%d in %s)\n", \
            fd, __FILE__, __LINE__, __func__);             \
    close(fd);                                             \
  } while (0)

/* DEBUG */
/*
static struct timeval m_start_time;
static long m_elapsed_us = 0;
static struct timeval m_end_time;
#define START_TIMER \
do { \
    gettimeofday(&m_start_time, NULL); \
} while (0)


#define END_TIMER \
do { \
    gettimeofday(&m_end_time, NULL); \
    m_elapsed_us = (m_end_time.tv_sec - m_start_time.tv_sec) * 1000000L + \
                      (m_end_time.tv_usec - m_start_time.tv_usec); \
    printf("Elapsed time: %ld microseconds\n", m_elapsed_us); \
} while (0)
*/
/* DEBUG_END */

/* Typedefs */
typedef enum
{
    CS_HTTP, /* HTTP */
    CS_SIO_WS_OPEN, /* socket.io */
    CS_WS_OPEN /* raw ws */
} client_state_t;

typedef struct
{
    int fd;
    client_state_t state;
    char* sio_sid;
    UT_hash_handle hh;
} client_t;

static client_t *clients = NULL;


#define MAX_EVENTS 64
static int m_epoll_fd = -1;
static int m_sock_server = -1;
static int m_timer_fd = -1; /* PONG events */
static struct epoll_event m_events[MAX_EVENTS];
static on_request m_http_request_handler = NULL;
static on_request m_ws_request_handler = NULL;
static on_request m_sio_request_handler = NULL;

char* get_header_value(const char *req, const char *key);

/* Definitions */
void server_set_request_handlers(on_request http_handler, on_request ws_handler, on_request sio_handler)
{
    m_http_request_handler = http_handler;
    m_ws_request_handler = ws_handler;
    m_sio_request_handler = sio_handler;

    log_msg(LOG_LEVEL_BOOT, "HTTP request handler set: %p\n", m_http_request_handler);
    log_msg(LOG_LEVEL_BOOT, "WebSocket request handler set: %p\n", m_ws_request_handler);
    log_msg(LOG_LEVEL_BOOT, "Socket.IO request handler set: %p\n", m_sio_request_handler);
}

static int m_send_ws_frame(int fd, uint8_t opcode, const void *data, size_t len, bool is_text)
{
    uint8_t hdr[2] = { (is_text ? 0x81 : 0x80) | opcode, (uint8_t)len };
    struct iovec iov[2] =
    {
        { .iov_base = hdr, .iov_len = 2 },
        { .iov_base = (void*)data, .iov_len = len }
    };

    return writev(fd, iov, 2) >= 0 ? SUCCESS : ERROR;
}

static int m_ws_parse_frame(int fd, uint8_t *hdr, uint8_t **payload, uint64_t *len, uint8_t *mask, bool *masked)
{
    uint64_t i;
    uint16_t ext16;
    uint64_t ext64;
    int n;

    n = recv(fd, hdr, 2, 0);
    if (n <= 0)
    {
        *payload = NULL;
        *len = 0;
        return ERROR;
    }

    *masked = hdr[1] & 0x80;
    *len = hdr[1] & 0x7F;

    if (*len == 126)
    {
        recv(fd, &ext16, 2, MSG_WAITALL);
        *len = ntohs(ext16);
    }
    else if (*len == 127)
    {
        recv(fd, &ext64, 8, MSG_WAITALL);
        *len = be64toh(ext64);
    }

    if (*masked)
    {
        recv(fd, mask, 4, MSG_WAITALL);
    }

    *payload = malloc(*len);

    n = recv(fd, *payload, *len, MSG_WAITALL);
    if (n < 0)
    {
        free(*payload);
        *payload = NULL;
        *len = 0;
        return ERROR;
    }

    if (*masked)
    {
        for (i = 0; i < *len; i++)
        {
            (*payload)[i] ^= mask[i % 4];
        }
    }

    return SUCCESS;
}

static int m_ws_read_text_payload(int fd, char **payload, size_t *len)
{
    uint8_t hdr[2];
    uint8_t *raw_payload;
    uint8_t mask[4];
    bool masked;
    uint64_t payload_len;

    if (m_ws_parse_frame(fd, hdr, &raw_payload, &payload_len, mask, &masked) == ERROR)
    {
        *payload = NULL;
        *len = 0;
        return ERROR;
    }

    raw_payload[payload_len] = '\0';
    *payload = (char *)raw_payload;
    *len = payload_len;

    return SUCCESS;
}

static int m_ws_handle_frame(client_t *cli)
{
    uint8_t hdr[2];
    uint8_t *payload;
    uint8_t mask[4];
    uint8_t op;
    bool masked;
    uint64_t len;

    if (m_ws_parse_frame(cli->fd, hdr, &payload, &len, mask, &masked) == ERROR)
    {
        REMOVE_CLIENT(cli->fd);
        if (payload)
            free(payload);
        return SUCCESS;
    }

    op = hdr[0] & 0x0F;

    switch (op)
    {
    case 0x1: /* TEXT */
        if (m_ws_request_handler)
            m_ws_request_handler(cli->fd, (char *)payload, len);
        break;
    case 0x8: /* close */
        REMOVE_CLIENT(cli->fd);
        break;
    case 0x9: /* PING */
        m_send_ws_frame(cli->fd, 0xA, payload, len, false); /* send PONG */
        break;
    case 0xA: /* PONG */
        log_msg(LOG_LEVEL_DEBUG, "Received pong from fd=%d\n", cli->fd);
        break;
    default:
        log_msg(LOG_LEVEL_WARN, "Unhandled WebSocket opcode %d for fd=%d\n", op, cli->fd);
        break;
    }

    free(payload);
    return SUCCESS;
}

static void m_compute_ws_accept(const char *key, char *out_b64)
{
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha1sum[SHA_DIGEST_LENGTH];
    char buf[256];
    BIO *b64;
    BIO *bmem;
    int len;

    snprintf(buf, sizeof(buf), "%s%s", key, GUID);
    SHA1((unsigned char*)buf, strlen(buf), sha1sum);

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, sha1sum, SHA_DIGEST_LENGTH);
    BIO_flush(b64);

    len = BIO_read(bmem, out_b64, 64);
    out_b64[len] = '\0';
    BIO_free_all(b64);
}

static void m_do_websocket_handshake(int fd, const char *req)
{
    char *key = get_header_value(req, "Sec-WebSocket-Key");
    char accept[64];
    m_compute_ws_accept(key, accept);
    dprintf(fd,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept);
    free(key);
}

/********************************************/
/* Socket-io */
/********************************************/
static char* m_generate_uuid()
{
    char *uuid = malloc(37);
    if (!uuid) {
        log_msg(LOG_LEVEL_ERROR, "Failed to allocate memory for UUID\n");
        return NULL;
    }
    snprintf(uuid, 37, "%08x-%04x-%04x-%04x-%012x",
             rand(), rand() % 0x10000, rand() % 0x10000,
             rand() % 0x10000, (unsigned int)(rand() % 0x1000000000000));
    return uuid;
}

static void m_do_sio_ws_handshake(int fd, const char *req, client_t *c)
{
    char *key;
    char accept[64];
    char open_pkt[512];
    int n;

    key = get_header_value(req, "Sec-WebSocket-Key");
    m_compute_ws_accept(key, accept);

    dprintf(fd,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n", accept);
    free(key);

    c->sio_sid = m_generate_uuid();

    n = snprintf(open_pkt, sizeof(open_pkt),
        "0{\"sid\":\"%s\",\"upgrades\":[\"websocket\"],"
        "\"pingInterval\":25000,\"pingTimeout\":5000}",
        c->sio_sid);

    m_send_ws_frame(fd, 0x1, open_pkt, n, true);

    /* Send the connection accepted frame */
    m_send_ws_frame(fd, 0x1, "40", 2, true);
}

static int m_sio_ws_handle_frame(client_t *c)
{
    char *payload;
    size_t len;
    char code;
    char* body;
    size_t body_len;

    if (m_ws_read_text_payload(c->fd, &payload, &len) <= 0)
    {
        REMOVE_CLIENT(c->fd);
        if (payload)
            free(payload);
        return SUCCESS;
    }

    code = payload[0];
    body = payload + 1;
    body_len = len - 1;

    switch (code)
    {
    case '2': /* ping */
        log_msg(LOG_LEVEL_DEBUG, "Received ping from fd=%d\n", c->fd);
        m_send_ws_frame(c->fd, 0x1, "3", 1, true);
        break;
    case '3': /* PONG */
        break;
    case '5': /* UPGRADE */
        break;
    case '4': /* Socket-io message */
        if (m_sio_request_handler)
            m_sio_request_handler(c->fd, body, body_len);
        break;
    default:
        break;
    }
    free(payload);
    return SUCCESS;
}

static bool m_is_sio_path(const char *req)
{
    /* it must come with 'GET /socket.io/ */
    return (strstr(req, "GET /socket.io/") == req);
}

/********************************************/
/* SERVER */
/********************************************/
static void m_server_send_ping()
{
    uint64_t expirations;
    client_t *c;
    client_t *tmp;

    read(m_timer_fd, &expirations, sizeof(expirations));

    HASH_ITER(hh, clients, c, tmp)
    {
        if (c->state == CS_SIO_WS_OPEN)
        {
            m_send_ws_frame(c->fd, 0x1, "2", 1, true);
        }
        else if (c->state == CS_WS_OPEN)
        {
            m_send_ws_frame(c->fd, 0x9, NULL, 0, false);
        }
    }
}

int init_plain_socket(int port)
{
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return ERROR;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        return ERROR;
    }

    if (listen(sockfd, SOMAXCONN) < 0)
    {
        perror("listen");
        close(sockfd);
        return ERROR;
    }

    return sockfd;
}

int m_handle_client_event(int fd)
{
    char buf[4096];
    int ret;
    client_t* c;

    log_msg(LOG_LEVEL_ERROR, "Handling client event for fd=%d\n", fd);
    HASH_FIND_INT(clients, &fd, c);
    if (!c)
    {
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        log_msg(LOG_LEVEL_ERROR, "Client not found in hash table: fd=%d\n", fd);
        return SUCCESS;
    }

    if (c->state == CS_WS_OPEN)
        return m_ws_handle_frame(c);

    if (c->state == CS_SIO_WS_OPEN) 
        return m_sio_ws_handle_frame(c);

    ret = recv(fd, buf, sizeof(buf) - 1, 0);
    if (ret <= 0)
    {
        log_msg(LOG_LEVEL_INFO, "Client disconnected or error: fd=%d\n", fd);
        REMOVE_CLIENT(fd);
        return SUCCESS;
    }

    buf[ret] = '\0';

    if (m_is_sio_path(buf))
    {
        if (strstr(buf, "Upgrade: websocket") && strstr(buf, "Sec-WebSocket-Key:"))
        {
            m_do_sio_ws_handshake(fd, buf, c);
            c->state = CS_SIO_WS_OPEN;
            return SUCCESS;
        }
        /* means it's not a direct upgrade but 1st get http for checking and later on update request zzz. */
    }
    else if (strstr(buf, "Upgrade: websocket") &&
        strstr(buf, "Connection: Upgrade") &&
        strstr(buf, "Sec-WebSocket-Key:"))
    {
        m_do_websocket_handshake(fd, buf);
        c->state = CS_WS_OPEN;
        return SUCCESS;
    }
    else if (m_http_request_handler)
    {
        ret = m_http_request_handler(fd, buf, ret);
        if (ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "Error handling HTTP request for fd=%d\n", fd);
            REMOVE_CLIENT(fd);
            return ERROR;
        }
        REMOVE_CLIENT(fd);
        return SUCCESS;
    }

    REMOVE_CLIENT(fd);
    return SUCCESS;
}

int m_handle_new_client(int fd)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd;
    int flags;
    struct epoll_event ev;
    client_t* c;

    client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept");
        return ERROR;
    }

    log_msg(LOG_LEVEL_INFO, "New client connected: fd=%d\n", client_fd);

    flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        perror("epoll_ctl: add client");
        close(client_fd);
        return ERROR;
    }

    c = calloc(1, sizeof(*c));
    c->fd = client_fd;
    c->state = CS_HTTP;
    HASH_ADD_INT(clients, fd, c);

    return SUCCESS;
}

int server_select()
{
    int n;
    int i;
    int fd;
    int ret;

    n = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS, 1000); /* 1s */
    if (n < 0)
    {
        perror("epoll_wait");
        return ERROR;
    }

    for (i = 0; i < n; ++i)
    {
        fd = m_events[i].data.fd;

        if (fd == m_sock_server)
        {
            ret = m_handle_new_client(fd);
            if (ret == ERROR)
                log_msg(LOG_LEVEL_ERROR, "Failed to accept new client\n");
        }
        else if (fd == m_timer_fd)
        {
            m_server_send_ping();
        }
        else
        {
            ret = m_handle_client_event(fd);
            if (ret == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR, "Failed to handle client event for fd=%d\n", fd);
                // epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                // close(fd);
                REMOVE_CLIENT(fd);
                log_msg(LOG_LEVEL_INFO, "Client removed: fd=%d\n", fd);
            }
        }
    }

    return SUCCESS;
}

int server_init(int port)
{
    struct epoll_event ev;
    struct epoll_event _ev;
    struct itimerspec its =
    {
        .it_interval = {25, 0}, /* 25s */
        .it_value    = {25, 0}
    };

    m_sock_server = init_plain_socket(port);
    if (m_sock_server == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize plain TCP socket\n");
        return ERROR;
    }

    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1)
    {
        perror("epoll_create1");
        return ERROR;
    }

    ev.events = EPOLLIN;
    ev.data.fd = m_sock_server;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_sock_server, &ev) == -1)
    {
        perror("epoll_ctl: server socket");
        return ERROR;
    }

    m_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    timerfd_settime(m_timer_fd, 0, &its, NULL);

    _ev.events = EPOLLIN;
    _ev.data.fd = m_timer_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_timer_fd, &_ev);

    log_msg(LOG_LEVEL_BOOT, "HTTP server initialized: fd=%d\n", m_sock_server);
    return SUCCESS;
}

void server_cleanup()
{
    client_t *c;
    client_t *tmp;

    if (m_sock_server != -1)
    {
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_sock_server, NULL);
        close(m_sock_server);
        log_msg(LOG_LEVEL_INFO, "Server socket closed: fd=%d\n", m_sock_server);
        m_sock_server = -1;
    }

    if (m_epoll_fd != -1)
    {
        close(m_epoll_fd);
        log_msg(LOG_LEVEL_INFO, "Epoll instance closed\n");
        m_epoll_fd = -1;
    }

    HASH_ITER(hh, clients, c, tmp)
    {
        HASH_DEL(clients, c);
        if (c->sio_sid)
            free(c->sio_sid);
        free(c);
        log_msg(LOG_LEVEL_INFO, "Client removed: fd=%d\n", c->fd);
    }
}

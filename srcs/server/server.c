#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"
#include "../../inc/ft_list.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../log/log_api.h"
#include "server_api.h"

/* defines */
#define SERVER_KEY "SOME_KEY"
#define MAX_LOGIN_ROLES 3

#define REMOVE_CLIENT(fd) \
    do { \
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL); \
        log_msg(LOG_LEVEL_INFO, "Removing client %d\n", fd); \
        close(fd); \
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

typedef struct
{
    list_item_t item;
    int fd;
} client_list_t;

#define MAX_EVENTS 64
static int m_epoll_fd = -1;
static int m_sock_server = -1;
static struct epoll_event m_events[MAX_EVENTS];
static on_http_request m_http_request_handler = NULL;

void server_set_http_request_handler(on_http_request handler)
{
    m_http_request_handler = handler;
}

/* Definitions */
int init_plain_socket(int port)
{
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return ERROR;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return ERROR;
    }

    if (listen(sockfd, SOMAXCONN) < 0) {
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

    ret = recv(fd, buf, sizeof(buf) - 1, 0);
    if (ret <= 0)
    {
        log_msg(LOG_LEVEL_INFO, "Client disconnected or error: fd=%d\n", fd);
        REMOVE_CLIENT(fd);
        return SUCCESS;
    }

    buf[ret] = '\0';

    // log_msg(LOG_LEVEL_INFO, "Server: HTTP Request received:\n%s\n", buf);
    if (m_http_request_handler)
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
        else
        {
            ret = m_handle_client_event(fd);
            if (ret == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR, "Failed to handle client event for fd=%d\n", fd);
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            }
        }
    }

    return SUCCESS;
}

int server_init(int port)
{
    struct epoll_event ev;

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

    log_msg(LOG_LEVEL_BOOT, "HTTP server initialized: fd=%d\n", m_sock_server);
    return SUCCESS;
}

void server_cleanup()
{
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

}

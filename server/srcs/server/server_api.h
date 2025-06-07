#ifndef SERVER_API_H
#define SERVER_API_H

typedef int (*on_http_request)(int fd, const char *request, size_t request_len);

void server_set_http_request_handler(on_http_request handler);
int server_select();
int server_init(int port);
void server_cleanup();

int server_remove_client(int fd);

#endif /* SERVER_API_H */

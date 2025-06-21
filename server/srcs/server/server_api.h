#ifndef SERVER_API_H
#define SERVER_API_H

typedef int (*on_request)(int fd, const char *request, size_t request_len);

void server_set_request_handlers(on_request http_handler, on_request ws_handler, on_request sio_handler);
int server_select();
int server_init(int port);
void server_cleanup();

int server_remove_client(int fd);

#endif /* SERVER_API_H */

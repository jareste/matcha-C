#ifndef ROUTER_API_H
#define ROUTER_API_H

#include <stddef.h>
int router_handle_http_request(int fd, const char *request, size_t request_len);

#endif /* ROUTER_API_H */

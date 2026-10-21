#ifndef API_UMGMT_H
#define API_UMGMT_H

#include "../../router/router_api.h"

void api_umgmt_register(void* ctx, void *user_data);
void api_umgmt_login(void* ctx, void *user_data);
void api_umgmt_init();

#endif /* API_UMGMT_H */

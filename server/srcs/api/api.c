#include "api.h"
#include "umgmt/api_umgmt.h"
#include "chat/chat_api.h"

void api_init()
{
    api_umgmt_init();
    chat_init();
}

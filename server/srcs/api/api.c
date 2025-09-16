#include "api.h"
#include "umgmt/api_umgmt.h"
#include "chat/chat_api.h"
#include "profile/api_profile.h"
#include "tags/api_tags.h"
#include "pics/api_pics.h"

void api_init()
{
    api_umgmt_init();
    chat_init();
    api_profile_init();
    api_tags_init();
    api_pics_init();
}

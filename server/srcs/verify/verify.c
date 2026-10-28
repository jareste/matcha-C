#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

bool verify_email_format(const char* email)
{
    regex_t regex;
    int result;
    const char* pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";

    if (!email || strlen(email) == 0)
        return false;

    result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result != 0)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to compile email regex\n");
        return false;
    }
    
    result = regexec(&regex, email, 0, NULL, 0);
    regfree(&regex);
    
    return (result == 0);
}

bool verify_username_format(const char* username)
{
    regex_t regex;
    int result;
    const char* pattern = "^[a-zA-Z0-9][a-zA-Z0-9_-]{1,18}[a-zA-Z0-9]$";
    
    if (!username || strlen(username) == 0)
        return false;
    
    result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result != 0)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to compile username regex\n");
        return false;
    }
    
    result = regexec(&regex, username, 0, NULL, 0);
    regfree(&regex);
    
    return (result == 0);
}
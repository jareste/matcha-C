#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include <error_codes.h>
#include "../log/log_api.h"
#include "config_file.h"

typedef struct
{   
    log_level LOG_LEVEL;
    char* LOG_FILE_PATH;
    bool LOG_ERASE;

    char* CERT_PATH;
    char* KEY_PATH;
    int PORT;

} config_t;

config_t* m_config_content = NULL;

static void m_init_config_content()
{
    if (!m_config_content)
        return;

    m_config_content->LOG_LEVEL = LOG_LEVEL_WARN;
    m_config_content->LOG_FILE_PATH = strdup("log.txt");
    m_config_content->LOG_ERASE = true;

    m_config_content->CERT_PATH = strdup("cert.pem");
    m_config_content->KEY_PATH = strdup("key.pem");
    m_config_content->PORT = 12345;
}

void parse_set_log_config(log_config* log)
{
    log->LOG_ERASE = m_config_content->LOG_ERASE;
    log->LOG_FILE_PATH = m_config_content->LOG_FILE_PATH;
    log->LOG_LEVEL = m_config_content->LOG_LEVEL;
}

void parse_set_ssl_config(ssl_config* ssl)
{
    ssl->CERT_PATH = m_config_content->CERT_PATH;
    ssl->KEY_PATH = m_config_content->KEY_PATH;
    ssl->PORT = m_config_content->PORT;
}

void parse_free_config()
{
    free(m_config_content);
    m_config_content = NULL;
}

int parse_config(const char *filename)
{
    char    line[256];
    FILE*   fp;
    char*   p;
    char*   eq;
    char*   key;
    char*   val;
    char    c;

    m_config_content = malloc(sizeof(config_t));

    /* Set default values */
    m_init_config_content();

    fp = fopen(filename, "r");
    if (!fp)
    {
        perror("Unable to open config file");
        return ERROR;
    }

    while (fgets(line, sizeof(line), fp))
    {
        p = line;

        while (isspace((unsigned char)*p)) p++;

        if (*p == '#' || *p == '\0' || *p == '\n')
            continue;

        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = p;
        val = eq + 1;

        char *kend = key + strlen(key) - 1;
        while (kend > key && isspace((unsigned char)*kend)) *kend-- = '\0';

        while (isspace((unsigned char)*val)) val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && (isspace((unsigned char)*vend) || *vend=='\r' || *vend=='\n'))
            *vend-- = '\0';

        if (strcmp(key, "LOG_LEVEL") == 0)
            m_config_content->LOG_LEVEL = atoi(val);
        else if (strcmp(key, "LOG_FILE_PATH") == 0)
        {
            free(m_config_content->LOG_FILE_PATH);
            m_config_content->LOG_FILE_PATH = strdup(val);
        }
        else if (strcmp(key, "LOG_ERASE") == 0)
        {
            c = val[0];
            m_config_content->LOG_ERASE = (c=='y'||c=='Y'||c=='1');
        }
        else if (strcmp(key, "CERT_PATH") == 0)
        {
            free(m_config_content->CERT_PATH);
            m_config_content->CERT_PATH = strdup(val);
        }
        else if (strcmp(key, "KEY_PATH") == 0)
        {
            free(m_config_content->KEY_PATH);
            m_config_content->KEY_PATH = strdup(val);
        }
        else if (strcmp(key, "PORT") == 0)
            m_config_content->PORT = atoi(val);

    }

    fclose(fp);
    return SUCCESS;
}

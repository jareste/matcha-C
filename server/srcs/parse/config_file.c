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

    char* DB_HOST;
    char* DB_PORT;
    char* DB_USER;
    char* DB_PASSWORD;
    char* DB_NAME;

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

    m_config_content->DB_HOST = strdup("localhost");
    m_config_content->DB_PORT = strdup("5432");
    m_config_content->DB_USER = strdup("user");
    m_config_content->DB_PASSWORD = strdup("password");
    m_config_content->DB_NAME = strdup("database");
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

void parse_set_db_config(db_config* db)
{
    db->DB_HOST = m_config_content->DB_HOST;
    db->DB_PORT = m_config_content->DB_PORT;
    db->DB_USER = m_config_content->DB_USER;
    db->DB_PASSWORD = m_config_content->DB_PASSWORD;
    db->DB_NAME = m_config_content->DB_NAME;
}

void parse_free_config()
{
    free(m_config_content->LOG_FILE_PATH);

    free(m_config_content->CERT_PATH);
    free(m_config_content->KEY_PATH);

    free(m_config_content->DB_HOST);
    free(m_config_content->DB_PORT);
    free(m_config_content->DB_USER);
    free(m_config_content->DB_PASSWORD);
    free(m_config_content->DB_NAME);

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
        else if (strcmp(key, "DB_HOST") == 0)
        {
            free(m_config_content->DB_HOST);
            m_config_content->DB_HOST = strdup(val);
        }
        else if (strcmp(key, "DB_PORT") == 0)
        {
            free(m_config_content->DB_PORT);
            m_config_content->DB_PORT = strdup(val);
        }
        else if (strcmp(key, "DB_USER") == 0)
        {
            free(m_config_content->DB_USER);
            m_config_content->DB_USER = strdup(val);
        }
        else if (strcmp(key, "DB_PASSWORD") == 0)
        {
            free(m_config_content->DB_PASSWORD);
            m_config_content->DB_PASSWORD = strdup(val);
        }
        else if (strcmp(key, "DB_NAME") == 0)
        {
            free(m_config_content->DB_NAME);
            m_config_content->DB_NAME = strdup(val);
        }
    }

    fclose(fp);
    return SUCCESS;
}

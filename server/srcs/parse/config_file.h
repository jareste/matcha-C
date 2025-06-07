#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

typedef struct
{
    log_level LOG_LEVEL;
    char* LOG_FILE_PATH;
    bool LOG_ERASE;
} log_config;

typedef struct
{
    char* CERT_PATH;
    char* KEY_PATH;
    int PORT;
} ssl_config;

typedef struct
{
    char* DB_HOST;
    char* DB_PORT;
    char* DB_USER;
    char* DB_PASSWORD;
    char* DB_NAME;
} db_config;

int parse_config(const char *filename);
void parse_free_config();

void parse_set_log_config(log_config* log);
void parse_set_ssl_config(ssl_config* ssl);
void parse_set_db_config(db_config* db);

#endif /* CONFIG_FILE_H */

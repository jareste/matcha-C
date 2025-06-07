#ifndef DB_TABLE_USER_H
#define DB_TABLE_USER_H

#include <stdbool.h>
#include <time.h>

typedef struct
{
    int id;
    const char *username;
    const char *email;
    const char *password_hash;
    const char *first_name;
    const char *last_name;
    const char *gender;
    const char *orientation;
    const char *bio;
    int fame_rating;
    double gps_lat;
    double gps_lon;
    bool location_optout;
    const char *last_online;
    time_t created_at;
} user_t;

typedef struct 
{
    user_t **users;
    size_t count;
    PGresult *pg_result;
} user_t_array;

int db_tuser_init(DB_ID DB);
int db_tuser_insert_user(DB_ID DB, user_t* u);
user_t_array* db_tuser_select_all_users(DB_ID DB);
int db_tuser_free_array(user_t_array* users);
int db_tuser_delete_user_from_pk(DB_ID DB, const char* name);

#endif /* DB_TABLE_USER_H */

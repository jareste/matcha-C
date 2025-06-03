#ifndef DB_USERS_H
#define DB_USERS_H

#include <stdbool.h>
#include "db_api.h"
#include <libpq-fe.h>

int DB_user_create_table(DB_ID db);

PGresult *DB_user_get_user_by_uname(DB_ID db, const char *username);

PGresult *DB_user_get_all(DB_ID db);

/*
 * Insert a new row into users.
 *
 * Parameters:
 *   - db:           open DB connection
 *   - username:     must be non-NULL
 *   - email:        must be non-NULL
 *   - password_hash:must be non-NULL
 *   - first_name:   may be NULL
 *   - last_name:    may be NULL
 *   - gender:       may be NULL
 *   - orientation:  may be NULL (if NULL, default 'bisexual' applies)
 *   - bio:          may be NULL
 *   - fame_rating:  integer value (0 or positive)
 *   - gps_lat:      double (e.g. 40.4168)
 *   - gps_lon:      double (e.g. -3.7038)
 *   - location_optout: boolean
 *   - last_online:  ISO‐8601 timestamp string (e.g. "2025-06-03 14:22:00+00"); may be NULL
 *
 * Returns 0 on success, non-zero on error.
 *
 * If you want to omit last_online, pass NULL for that parameter.
 */
int DB_user_insert(DB_ID db,
                const char *username,
                const char *email,
                const char *password_hash,
                const char *first_name,
                const char *last_name,
                const char *gender,
                const char *orientation,
                const char *bio,
                int fame_rating,
                double gps_lat,
                double gps_lon,
                bool location_optout,
                const char *last_online);

int DB_user_delete_by_username(DB_ID db, const char *username);

int DB_user_print_all(DB_ID DB);

#endif /* DB_USERS_H */

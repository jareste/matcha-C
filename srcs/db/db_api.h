#ifndef DB_H
#define DB_H

#include <libpq-fe.h>
#include <stdint.h>

typedef uintptr_t DB_ID;
#define INVALID_DB_ID ((DB_ID)~0)

DB_ID db_connect(const char *conninfo);

/*
 * Execute a non-SELECT SQL command (INSERT, UPDATE, DELETE, DDL, etc.)
 *   db: pointer to an open DB handle
 *   sql: SQL statement with param markers ($1, $2, …). Example: "INSERT INTO users(username, email) VALUES($1, $2)"
 *   nParams: number of parameters
 *   paramValues: array of null-terminated strings (length = nParams). libpq will treat them as text.
 *
 * Returns: 0 on success, non-zero on error (and logs to stderr).
 */
int db_execute(DB_ID db,
               const char *sql,
               int nParams,
               const char *const *paramValues);

/*
 * Execute a SELECT (or any query that returns rows). Returns a PGresult* you must free with db_clear_result().
 *   db: pointer to an open DB handle
 *   sql: SQL statement with param markers ($1, $2, …)
 *   nParams: number of parameters
 *   paramValues: array of null-terminated strings
 *
 * On error, returns NULL.
 */
PGresult *db_query(DB_ID db,
                   const char *sql,
                   int nParams,
                   const char *const *paramValues);

void db_clear_result(PGresult *res);

void db_close(DB_ID db);

#endif /* DB_H */

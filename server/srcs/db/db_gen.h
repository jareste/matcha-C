#ifndef DB_GENERIC_H
#define DB_GENERIC_H

#include <stdbool.h>
#include <stdarg.h>
#include <libpq-fe.h>   // for PGresult
#include "db_api.h"         // for DB_ID, db_execute, db_query, db_clear_result

/*
 * A single column definition. For each column you supply:
 *  - name:        “users”, “email”, “created_at”, etc.
 *  - type:        “VARCHAR(32)”, “INTEGER”, “TIMESTAMP”, “TEXT”, etc.
 *  - is_primary:  true if this column is the PRIMARY KEY
 *  - is_unique:   true if UNIQUE
 *  - not_null:    true if NOT NULL
 *  - default_val: string literal of DEFAULT (e.g. "'now()'", "0", NULL if no default)
 *
 * At run‐time you build an array ColumnDef cols[ ] and pass it to TableSchema.
 */
typedef struct {
    char  *name;
    char  *type;
    bool   is_primary;
    bool   is_unique;
    bool   not_null;
    char  *default_val;  
} columnDef_t;

/*
 * The schema for an entire table:
 *  - name:   table name
 *  - n_cols: number of entries in columns[].
 *  - columns: array of ColumnDef (length = n_cols)
 */
typedef struct {
    const char*         name;
    const int           n_cols;
    const columnDef_t*  columns;  
} tableSchema_t;

/*
 *    Generate & execute a “CREATE TABLE IF NOT EXISTS …” statement
 *    according to TableSchema. Returns 0 on success, non‐zero on error.
 */
int db_gen_create_table(DB_ID db, const tableSchema_t *schema);

/*
 *    Insert a row into ANY table. You must pass exactly schema->n_cols
 *    C‐strings (char *) in the same order as columns[].  If a given
 *    column is NULLable and you want to store SQL NULL, pass NULL for that vararg.
 *
 *    For non‐string types (INTEGER, DOUBLE, etc.), convert to string
 *    yourself before calling (e.g. via sprintf).
 *
 *    Returns 0 on success, non‐zero on error.
 */
int db_gen_insert(DB_ID db, const tableSchema_t *schema, ...);

/*
 *    SELECT * FROM tableName. Returns a PGresult* (must call db_clear_result on it)
 *    or NULL on error.
 */
PGresult *db_gen_select_all_from(DB_ID db, const tableSchema_t *schema);

/*
 *    DELETE FROM tableName WHERE <primary_key> = $1
 *    Find the column marked as is_primary. You must pass exactly one
 *    C‐string (the primary‐key value) in varargs.
 *    Returns 0 on success, non‐zero on error.
 */
int db_gen_delete_by_pk(DB_ID db, const tableSchema_t *schema, const char *pk_value);

/*
  *    UPDATE tableName SET <col1> = $1, <col2> = $2, ... WHERE <primary_key> = $n
  *    Find the column marked as is_primary. You must pass exactly one
  *    C‐string (the primary‐key value) in varargs, followed by the values
  *    for each column to update, in the same order as columns[].
  *    Returns 0 on success, non‐zero on error.
  */
int db_gen_update_by_pk(DB_ID db,
                        const tableSchema_t *schema,
                        const char *pk_value,
                        ...);

int db_gen_parse_timestamp(const char *timestamp_str);

#endif /* DB_GENERIC_H */

#include "db_gen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../../inc/ft_malloc.h"

static char *m_str_concat(const char *a, const char *b)
{
    size_t la;
    size_t lb;
    char *out;
    
    la = strlen(a);
    lb = strlen(b);
    out = malloc(la + lb + 1);

    memcpy(out, a, la);
    memcpy(out + la, b, lb);
    out[la+lb] = '\0';
    return out;
}

int db_gen_create_table(DB_ID db, const tableSchema_t *schema)
{
    char* sql = NULL;
    char* tmp;
    const char* prefix = "CREATE TABLE IF NOT EXISTS ";
    const char* open_paren = " (";
    char buffer[512];
    int  offset;
    int i;
    const columnDef_t* col;
    const char *close = ");";
    int rc;

    if ((db == INVALID_DB_ID) || schema == NULL || schema->n_cols <= 0)
    {
        /* ERROR */
        return -1;
    }
    
    sql = m_str_concat(prefix, schema->name);
    tmp = m_str_concat(sql, open_paren);

    free(sql);
    sql = tmp;

    for (i = 0; i < schema->n_cols; i++)
    {
        offset = 0;
        col = &schema->columns[i];
        offset += snprintf(buffer + offset, sizeof(buffer)-offset,
                           "%s %s", col->name, col->type);

        if (col->not_null)
            offset += snprintf(buffer + offset, sizeof(buffer)-offset, " NOT NULL");
        if (col->is_unique)
            offset += snprintf(buffer + offset, sizeof(buffer)-offset, " UNIQUE");
        if (col->default_val)
            offset += snprintf(buffer + offset, sizeof(buffer)-offset, " DEFAULT %s", col->default_val);
        if (col->is_primary)
            offset += snprintf(buffer + offset, sizeof(buffer)-offset, " PRIMARY KEY");

        if (i < schema->n_cols - 1)
            offset += snprintf(buffer + offset, sizeof(buffer)-offset, ", ");

        tmp = m_str_concat(sql, buffer);
        free(sql);
        sql = tmp;
    }

    tmp = m_str_concat(sql, close);
    free(sql);
    sql = tmp;

    rc = db_execute(db, sql, 0, NULL);
    free(sql);
    return rc;
}

int db_gen_insert(DB_ID db, const tableSchema_t *schema, ...)
{
    va_list ap;
    int total_cols;
    int included_count;
    int i;
    int buf_est;
    char* sql;
    int pos;
    int param_idx;
    int j;
    int placeholder_num;
    int more;
    int rc;

    if (db == INVALID_DB_ID || schema == NULL || schema->n_cols <= 0)
    {
        return -1;
    }

    total_cols = schema->n_cols;

    const char **values = malloc(sizeof(char *) * total_cols);

    va_start(ap, schema);
    for (i = 0; i < total_cols; i++)
        values[i] = va_arg(ap, char *);
    
    va_end(ap);

    included_count = 0;
    for (i = 0; i < total_cols; i++)
    {
        if (values[i] != NULL)
            included_count++;
    }

    if (included_count == 0)
    {
        free(values);
        return -1;
    }

    const char **paramValues = malloc(sizeof(char *) * included_count);

    buf_est = 256 + (included_count * 64);
    sql = malloc(buf_est);

    pos = snprintf(sql, buf_est, "INSERT INTO %s (", schema->name);

    param_idx = 0;
    for (i = 0; i < total_cols; i++)
    {
        if (values[i] == NULL)
            continue;
        
        pos += snprintf(sql + pos, buf_est - pos, "%s", schema->columns[i].name);

        more = 0;
        for (j = i + 1; j < total_cols; j++)
        {
            if (values[j] != NULL)
            {
                more = 1;
                break;
            }
        }
        if (more)
            pos += snprintf(sql + pos, buf_est - pos, ", ");
    }

    pos += snprintf(sql + pos, buf_est - pos, ") VALUES (");

    placeholder_num = 1;
    for (i = 0; i < total_cols; i++)
    {
        if (values[i] == NULL)
        {
            continue;
        }
        pos += snprintf(sql + pos, buf_est - pos, "$%d", placeholder_num);
        
        paramValues[param_idx++] = values[i];
        placeholder_num++;

        more = 0;
        for (j = i + 1; j < total_cols; j++)
        {
            if (values[j] != NULL)
            {
                more = 1;
                break;
            }
        }
        if (more)
            pos += snprintf(sql + pos, buf_est - pos, ", ");
    }
    pos += snprintf(sql + pos, buf_est - pos, ");");

    rc = db_execute(db, sql, included_count, paramValues);

    free(values);
    free(paramValues);
    free(sql);
    return rc;
}

PGresult *db_gen_select_all_from(DB_ID db, const tableSchema_t *schema)
{
    size_t buflen;
    char* sql;
    PGresult* res;

    if (db == INVALID_DB_ID || schema == NULL || schema->n_cols <= 0)
    {
        return NULL;
    }

    buflen = 256 + strlen(schema->name) + strlen(schema->columns[0].name);
    sql = malloc(buflen);
    
    snprintf(sql, buflen,
             "SELECT * FROM %s ORDER BY %s;",
             schema->name,
             schema->columns[0].name);

    res = db_query(db, sql, 0, NULL);
    free(sql);
    return res;
}

int db_gen_delete_by_pk(DB_ID db, const tableSchema_t *schema, const char *pk_value)
{
    int pk_index;
    int i;
    size_t buflen;
    char* sql;
    int rc;

    if (db == INVALID_DB_ID || schema == NULL || pk_value == NULL)
    {
        return -1;
    }

    pk_index = -1;
    for (i = 0; i < schema->n_cols; i++)
    {
        if (schema->columns[i].is_primary)
        {
            if (pk_index >= 0)
            {
                /* ERROR. multiple primary keys */
                return -1;
            }
            pk_index = i;
        }
    }
    if (pk_index < 0)
    {
        /* ERROR no primary key */
        return -1;
    }

    buflen = 256 + strlen(schema->name) + strlen(schema->columns[pk_index].name);
    sql = malloc(buflen);

    snprintf(sql, buflen,
             "DELETE FROM %s WHERE %s = $1;",
             schema->name,
             schema->columns[pk_index].name);

    const char *paramValues[1] = { pk_value };
    rc = db_execute(db, sql, 1, paramValues);
    free(sql);
    return rc;
}

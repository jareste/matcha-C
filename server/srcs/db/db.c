#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "db_api.h"
#include "../../inc/error_codes.h"
#include "../../inc/ft_malloc.h"

static inline DB_ID m_PGconn_ptr_to_id(PGconn  *db)
{
    return (DB_ID)(uintptr_t)db;
}

static inline PGconn *m_db_id_to_PGconn(DB_ID db_id)
{
    return (PGconn *)(uintptr_t)db_id;
}

DB_ID db_connect(const char *conninfo)
{
    PGconn *conn;

    if (!conninfo)
        return INVALID_DB_ID;

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        PQfinish(conn);
        return INVALID_DB_ID;
    }

    return m_PGconn_ptr_to_id(conn);
}

int db_execute(DB_ID db, const char *sql, int nParams, const char *const *paramValues)
{
    PGconn* conn;
    PGresult* res;
    ExecStatusType status;

    if ((db == INVALID_DB_ID) || !sql) return ERROR;

    conn = m_db_id_to_PGconn(db);

    res = PQexecParams(
        conn,
        sql,
        nParams,
        /* paramTypes = */ NULL,         /* let libpq infer parameter types */
        paramValues,
        /* paramLengths = */ NULL,       /* lengths not needed for text */
        /* paramFormats = */ NULL,       /* all text format */
        0                                /* result in text format */
    );

    if (!res)
    {
        fprintf(stderr, "db_execute: PQexecParams failed: %s\n", PQerrorMessage(conn));
        return ERROR;
    }

    status = PQresultStatus(res);
    if (status != PGRES_COMMAND_OK)
    {
        PQclear(res);
        fprintf(stderr, "db_execute: SQL command failed: %s\n", PQerrorMessage(conn));
        return ERROR;
    }

    PQclear(res);
    return SUCCESS;
}

PGresult *db_query(DB_ID db, const char *sql, int nParams, const char *const *paramValues)
{
    PGconn* conn;
    PGresult* res;
    ExecStatusType status;

    if (!db || !sql) return NULL;

    conn = m_db_id_to_PGconn(db);

    res = PQexecParams(
        conn,
        sql,
        nParams,
        /* paramTypes = */ NULL,
        paramValues,
        /* paramLengths = */ NULL,
        /* paramFormats = */ NULL,
        0
    );

    if (!res) return NULL;

    status = PQresultStatus(res);
    if (status != PGRES_TUPLES_OK)
    {
        PQclear(res);
        return NULL;
    }

    return res;
}

void db_clear_result(PGresult *res)
{
    if (res)
        PQclear(res);
}

void db_close(DB_ID db)
{
    PGconn* conn;
    
    if (!db)
        return;
    
    conn = m_db_id_to_PGconn(db);
    if (conn)
        PQfinish(conn);
}

int db_init(DB_ID* DB, char* host, char* port,\
            char* user, char* pwd, char* dbname)
{
    char conninfo[512] = {0};

    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s user=%s password=%s dbname=%s",
             host, port, user, pwd, dbname);

    *DB = db_connect(conninfo);
    if (*DB == INVALID_DB_ID)
        return ERROR;

    return SUCCESS;
}

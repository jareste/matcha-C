#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "db_api.h"

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
    {
        fprintf(stderr, "[db_connect] ERROR: conninfo is NULL\n");
        return INVALID_DB_ID;
    }

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "[db_connect] Connection to database failed: %s\n",
                PQerrorMessage(conn));
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

    if ((db == INVALID_DB_ID) || !sql)
    {
        fprintf(stderr, "[db_execute] Invalid arguments\n");
        return -1;
    }

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
        fprintf(stderr, "[db_execute] PQexecParams returned NULL: %s\n",
                PQerrorMessage(conn));
        return -1;
    }

    status = PQresultStatus(res);
    if (status != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "[db_execute] SQL error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

PGresult *db_query(DB_ID db, const char *sql, int nParams, const char *const *paramValues)
{
    PGconn* conn;
    PGresult* res;
    ExecStatusType status;

    if (!db || !sql)
    {
        fprintf(stderr, "[db_query] Invalid arguments\n");
        return NULL;
    }

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

    if (!res)
    {
        fprintf(stderr, "[db_query] PQexecParams returned NULL: %s\n",
                PQerrorMessage(conn));
        return NULL;
    }

    status = PQresultStatus(res);
    if (status != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "[db_query] Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    return res;
}

void db_clear_result(PGresult *res)
{
    if (res)
    {
        PQclear(res);
    }
}

void db_close(DB_ID db)
{
    PGconn* conn;
    
    if (!db)
        return;
    
    conn = m_db_id_to_PGconn(db);
    if (conn)
    {
        PQfinish(conn);
    }
}

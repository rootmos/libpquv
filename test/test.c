#include <pquv.h>

#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include <postgresql/libpq-fe.h>

#define panic(...) { fprintf(stderr, __VA_ARGS__); exit(1); }
#define info(...) { fprintf(stdout, __VA_ARGS__); exit(1); }

#define test_start() { printf("%s - starting\n", __extension__ __FUNCTION__); }
#define test_ok() { printf("%s - ok\n", __extension__ __FUNCTION__); }

const char* conninfo()
{
    const char* ci = getenv("PG_CONNINFO");
    assert(ci);
    return ci;
}

#define TABLE_NAME_LENGTH 8
#define QUERY_BUFFER_LENGTH 1024

static void fresh_table(PGconn* conn, char* tbl)
{
    assert(TABLE_NAME_LENGTH ==
           snprintf(tbl, TABLE_NAME_LENGTH, "tbl%.5u", rand() % 10000));

    char q[QUERY_BUFFER_LENGTH];
    snprintf(q, QUERY_BUFFER_LENGTH,
             "CREATE TABLE %s (id VARCHAR(40) PRIMARY KEY, blob TEXT)",
             tbl);

    PGresult* r = PQexec(conn, q);

    ExecStatusType st = PQresultStatus(r);
    if(st != PGRES_COMMAND_OK) {
        panic("while creating table %s: %s, %s\n",
              tbl, PQresStatus(st), PQresultErrorMessage(r));
    }

    PQclear(r);
}

static void sanity_check()
{
    test_start();
    PGconn* conn = PQconnectdb(conninfo());
    assert(conn);

    char tbl[TABLE_NAME_LENGTH];
    fresh_table(conn, tbl);

    PQfinish(conn);
    test_ok();
}

int main()
{
    srand(time(NULL));
    sanity_check();
    return 0;
}

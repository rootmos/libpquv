#include <pquv.h>

#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {
        panic("while creating table : %s\n", PQresultErrorMessage(r));
    }
    PQclear(r);
}


#define some(x) char x[100]; snprintf(x, sizeof(x), #x "%.5u", rand() % 1000);

static void sanity_check()
{
    test_start();
    PGconn* conn = PQconnectdb(conninfo());
    assert(conn);

    char tbl[TABLE_NAME_LENGTH];
    fresh_table(conn, tbl);

    /* randomize some data */

    some(key);

    some(data);

    /* insert it */
    char q[QUERY_BUFFER_LENGTH];
    snprintf(q, QUERY_BUFFER_LENGTH,
             "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
             tbl, key, data);

    PGresult* r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    PQclear(r);

    /* fetch it */
    snprintf(q, QUERY_BUFFER_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), data) == 0);
    PQclear(r);

    /* fetch some other key should not return anything */
    some(other_key);

    snprintf(q, QUERY_BUFFER_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    assert(PQntuples(r) == 0);
    PQclear(r);

    /* but after inserting some data for it */
    some(other_data);

    snprintf(q, QUERY_BUFFER_LENGTH,
             "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
             tbl, other_key, other_data);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    PQclear(r);

    /* it should be there as well */
    snprintf(q, QUERY_BUFFER_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
    PQclear(r);

    /* after deleting the first value */
    snprintf(q, QUERY_BUFFER_LENGTH,
             "DELETE FROM %s WHERE id = '%s'", tbl, key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    PQclear(r);

    /* fetching it should return nothing */
    snprintf(q, QUERY_BUFFER_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    assert(PQntuples(r) == 0);
    PQclear(r);

    /* but the other value should still be there  */
    snprintf(q, QUERY_BUFFER_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    r = PQexec(conn, q);
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {
        panic("%s", PQresultErrorMessage(r));
    }
    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
    PQclear(r);

    PQfinish(conn);
    test_ok();
}

int main()
{
    srand(time(NULL));
    sanity_check();
    return 0;
}

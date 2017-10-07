#include <pquv.h>

#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <postgresql/libpq-fe.h>

#define panic(...) {                                            \
    fprintf(stderr, "%s:%d: ",                                  \
            __extension__ __FUNCTION__,__extension__ __LINE__); \
    fprintf(stderr, __VA_ARGS__); exit(1);                      \
}

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

#define some(x) char x[100]; snprintf(x, sizeof(x), #x "%.5u", rand() % 1000);

#define exec_and_expect_ok(...)                                             \
{                                                                           \
    char q[QUERY_BUFFER_LENGTH];                                            \
    snprintf(q, QUERY_BUFFER_LENGTH, __VA_ARGS__);                          \
    PGresult* r = PQexec(conn, q);                                          \
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {                             \
        panic("error while executing %s:%s\n", q, PQresultErrorMessage(r)); \
    }                                                                       \
    PQclear(r);                                                             \
}

#define exec_and_expect(e, ...)                                             \
{                                                                           \
    char q[QUERY_BUFFER_LENGTH];                                            \
    snprintf(q, QUERY_BUFFER_LENGTH, __VA_ARGS__);                          \
    PGresult* r = PQexec(conn, q);                                          \
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {                              \
        panic("error while executing %s:%s\n", q, PQresultErrorMessage(r)); \
    }                                                                       \
    e                                                                       \
    PQclear(r);                                                             \
}

static void fresh_table(PGconn* conn, char* tbl)
{
    assert(TABLE_NAME_LENGTH ==
           snprintf(tbl, TABLE_NAME_LENGTH, "tbl%.5u", rand() % 10000));

    exec_and_expect_ok(
            "CREATE TABLE %s (id VARCHAR(40) PRIMARY KEY, blob TEXT)", tbl);
}

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
    exec_and_expect_ok(
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, key, data);

    /* fetch it */
    exec_and_expect(
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    /* fetch some other key should not return anything */
    some(other_key);
    exec_and_expect( { assert(PQntuples(r) == 0); },
                     "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    /* but after inserting some data for it */
    some(other_data);
    exec_and_expect_ok(
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, other_key, other_data);

    /* it should be there as well */
    exec_and_expect(
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    /* after deleting the first value */
    exec_and_expect_ok("DELETE FROM %s WHERE id = '%s'", tbl, key);

    /* fetching it should return nothing */
    exec_and_expect({ assert(PQntuples(r) == 0); },
                    "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    /* but the other value should still be there  */
    exec_and_expect(
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    PQfinish(conn);
    test_ok();
}

int main()
{
    srand(time(NULL));
    sanity_check();
    return 0;
}

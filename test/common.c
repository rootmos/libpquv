#include "common.h"

#include <stdlib.h>
#include <assert.h>

const char* conninfo()
{
    const char* ci = getenv("PG_CONNINFO");
    assert(ci);
    return ci;
}

void fresh_table(PGconn* conn, char* tbl, size_t n)
{
    assert(8 == snprintf(tbl, n, "tbl%.5u", rand() % 10000));

    exec_and_expect_ok(
            "CREATE TABLE %s (id VARCHAR(40) PRIMARY KEY, blob TEXT)", tbl);
}

PGconn* connect_blk()
{
    PGconn* conn = PQconnectdb(conninfo());
    assert(conn);
    return conn;
}

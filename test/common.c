#include "common.h"

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

const char* conninfo()
{
    const char* ci = getenv("PG_CONNINFO"); assert(ci); return ci;
}

const char* conninfo_dummy()
{
    const char* ci = getenv("PG_DUMMY_CONNINFO"); assert(ci); return ci;
}

void fresh_table(PGconn* conn, char* tbl, size_t n, const char* type)
{
    assert(8 == snprintf(tbl, n, "tbl%.5u", rand() % 100000));

    exec_and_expect_ok(conn,
            "CREATE TABLE %s (id VARCHAR(40) PRIMARY KEY, blob %s)",
            tbl, type);
}

PGconn* connect_blk()
{
    PGconn* conn = PQconnectdb(conninfo());
    assert(conn);
    return conn;
}

void dummy_pg_iptables_drop()
{
    const char* addr = getenv("PG_DUMMY_ADDR"); assert(addr);
    char cmd[1024];
    snprintf(cmd, 1024, "iptables -A OUTPUT -d %s -j DROP", addr);
    assert(system(cmd) == 0);
}

void dummy_pg_iptables_reject()
{
    const char* addr = getenv("PG_DUMMY_ADDR"); assert(addr);
    char cmd[1024];
    snprintf(cmd, 1024, "iptables -A OUTPUT -d %s -j REJECT", addr);
    assert(system(cmd) == 0);
}

void dummy_pg_iptables_flush()
{
    assert(system("iptables -F") == 0);
}

void* random_blob(size_t n)
{
    assert(n < SSIZE_MAX);
    void* blob = malloc(n); assert(blob);

    int fd = open("/dev/urandom", 0); assert(fd >= 0);

    assert(read(fd, blob, n) == (ssize_t)n);
    assert(close(fd) == 0);

    return blob;
}

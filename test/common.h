#ifndef common_h
#define common_h

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <postgresql/libpq-fe.h>

#define panic(...) {                                            \
    fprintf(stderr, "%s:%d: ",                                  \
            __extension__ __FUNCTION__,__extension__ __LINE__); \
    fprintf(stderr, __VA_ARGS__); exit(1);                      \
}

#define info(...) { fprintf(stdout, __VA_ARGS__); exit(1); }

#define test_start() \
    volatile bool ok = false; \
    printf("%s - starting\n", __extension__ __FUNCTION__);
#define test_ok() { assert(ok); printf("%s - ok\n", __extension__ __FUNCTION__); }

#define some(x) char x[100]; snprintf(x, sizeof(x), #x "%.5u", rand() % 1000);

#define exec_and_expect_ok(conn, ...)                                       \
{                                                                           \
    char q[1024];                                                           \
    snprintf(q, 1024, __VA_ARGS__);                                         \
    PGresult* r = PQexec(conn, q);                                          \
    if(PQresultStatus(r) != PGRES_COMMAND_OK) {                             \
        panic("error while executing %s:%s\n", q, PQresultErrorMessage(r)); \
    }                                                                       \
    PQclear(r);                                                             \
}

#define exec_and_expect(conn, e, ...)                                       \
{                                                                           \
    char q[1024];                                                           \
    snprintf(q, 1024, __VA_ARGS__);                                         \
    PGresult* r = PQexec(conn, q);                                          \
    if(PQresultStatus(r) != PGRES_TUPLES_OK) {                              \
        panic("error while executing %s:%s\n", q, PQresultErrorMessage(r)); \
    }                                                                       \
    e                                                                       \
    PQclear(r);                                                             \
}

const char* conninfo();
PGconn* connect_blk();

void fresh_table(PGconn* conn, char* tbl, size_t n);

#define fresh(t) char t[8]; fresh_table(conn, t, sizeof(t));

#define new_loop(l) uv_loop_t l; assert(uv_loop_init(&l) == 0);
#define close_loop(l) { \
    assert(uv_run(&l, UV_RUN_NOWAIT) == 0); \
    assert(uv_loop_close(&l) == 0); \
}

#endif

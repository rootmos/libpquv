#include "common.h"
#include <uv.h>
#include <stdbool.h>
#include <pquv.h>
#include <string.h>

typedef struct {
    volatile bool* running;
    const char* expected_data;
} simple_read_t;

static void simple_read_cb(void* opaque, PGresult* r)
{
    simple_read_t* t = (simple_read_t*)opaque;

    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), t->expected_data) == 0);

    *t->running = false;
    PQclear(r);
}

void simple_read()
{
    test_start();
    PGconn* conn = connect_blk();

    some(key); some(data); fresh(tbl);

    exec_and_expect_ok(conn,
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, key, data);

    pquv_t* pquv = pquv_init(conninfo(), uv_default_loop());

    volatile bool running = true;
    simple_read_t t = {.running = &running, .expected_data = data};

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);
    pquv_query(pquv, q, simple_read_cb, &t);

    while (uv_run(uv_default_loop(), UV_RUN_ONCE) && running);

    pquv_free(pquv);
    test_ok();
}


typedef struct {
    pquv_t* pquv;
    volatile bool* running;
    const char* tbl;
    const char* expected_data;
    int reads;
} multiple_reads_t;

static void multiple_reads_cb(void* opaque, PGresult* r)
{
    multiple_reads_t* t = (multiple_reads_t*)opaque;

    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), t->expected_data) == 0);

    t->reads += 1;

    if(t->reads < 200) {
        char q[MAX_QUERY_LENGTH];
        snprintf(q, MAX_QUERY_LENGTH, "SELECT blob FROM %s", t->tbl);
        pquv_query(t->pquv, q, multiple_reads_cb, t);
    } else {
        *t->running = false;
    }

    PQclear(r);
}

void multiple_reads()
{
    test_start();
    PGconn* conn = connect_blk();

    some(key); some(data); fresh(tbl);

    exec_and_expect_ok(conn,
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, key, data);

    pquv_t* pquv = pquv_init(conninfo(), uv_default_loop());

    volatile bool running = true;
    multiple_reads_t t = {
        .pquv = pquv,
        .running = &running,
        .expected_data = data,
        .tbl = tbl,
        .reads = 0
    };

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    for (int i = 0; i < 100; ++i)
        pquv_query(pquv, q, multiple_reads_cb, &t);

    while (uv_run(uv_default_loop(), UV_RUN_ONCE) && running);

    pquv_free(pquv);
    test_ok();
}


typedef struct {
    volatile bool* running;
    PGconn* blk_conn;
    const char* tbl;
    const char* key;
    const char* data;
} simple_write_t;

static void simple_write_cb(void* opaque, PGresult* r)
{
    assert(PQresultStatus(r) == PGRES_COMMAND_OK);

    simple_write_t* t = (simple_write_t*)opaque;

    exec_and_expect(t->blk_conn,
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), t->key) == 0);
                assert(strcmp(PQgetvalue(r, 0, 1), t->data) == 0);
            },
            "SELECT id, blob FROM %s", t->tbl);

    *t->running = false;
    PQclear(r);
}

void simple_write()
{
    test_start();
    PGconn* conn = connect_blk();

    some(key); some(data); fresh(tbl);

    pquv_t* pquv = pquv_init(conninfo(), uv_default_loop());

    volatile bool running = true;
    simple_write_t t = {
        .running = &running,
        .blk_conn = conn,
        .tbl = tbl,
        .key = key,
        .data = data
    };

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH,
             "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
             tbl, key, data);
    pquv_query(pquv, q, simple_write_cb, &t);

    while (uv_run(uv_default_loop(), UV_RUN_ONCE) && running);

    pquv_free(pquv);
    test_ok();
}



typedef struct {
    volatile bool* running;
} invalid_query_t;

static void invalid_query_cb(void* opaque, PGresult* r)
{
    assert(PQresultStatus(r) == PGRES_FATAL_ERROR);
    invalid_query_t* t = (invalid_query_t*)opaque;
    *t->running = false;
    PQclear(r);
}

void invalid_query()
{
    test_start();
    pquv_t* pquv = pquv_init(conninfo(), uv_default_loop());

    volatile bool running = true;
    invalid_query_t t = { .running = &running, };

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH, "SELECT * FROM lol-table");
    pquv_query(pquv, q, invalid_query_cb, &t);

    while (uv_run(uv_default_loop(), UV_RUN_ONCE) && running);

    pquv_free(pquv);
    test_ok();
}

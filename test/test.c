#include "common.h"
#include <uv.h>
#include <pquv.h>
#include <string.h>

typedef struct {
    volatile bool* ok;
    const char* expected_data;
} simple_read_t;

static void simple_read_cb(void* opaque, PGresult* r)
{
    simple_read_t* t = (simple_read_t*)opaque;

    assert(PQresultStatus(r) == PGRES_TUPLES_OK);
    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), t->expected_data) == 0);

    *t->ok = true;
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

    new_loop(loop);
    pquv_t* pquv = pquv_init(conninfo(), &loop);

    simple_read_t t = {.ok = &ok, .expected_data = data};

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);
    pquv_query(pquv, q, simple_read_cb, &t);

    while (uv_run(&loop, UV_RUN_ONCE) && !ok);

    pquv_free(pquv);
    close_loop(loop);
    test_done();
}


typedef struct {
    pquv_t* pquv;
    volatile bool* ok;
    const char* tbl;
    const char* expected_data;
    int reads;
} multiple_reads_t;

static void multiple_reads_cb(void* opaque, PGresult* r)
{
    multiple_reads_t* t = (multiple_reads_t*)opaque;

    assert(PQresultStatus(r) == PGRES_TUPLES_OK);
    assert(PQntuples(r) == 1);
    assert(strcmp(PQgetvalue(r, 0, 0), t->expected_data) == 0);

    t->reads += 1;

    if(t->reads < 200) {
        char q[MAX_QUERY_LENGTH];
        snprintf(q, MAX_QUERY_LENGTH, "SELECT blob FROM %s", t->tbl);
        pquv_query(t->pquv, q, multiple_reads_cb, t);
    } else {
        *t->ok = true;
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

    new_loop(loop);
    pquv_t* pquv = pquv_init(conninfo(), &loop);

    multiple_reads_t t = {
        .pquv = pquv,
        .ok = &ok,
        .expected_data = data,
        .tbl = tbl,
        .reads = 0
    };

    char q[MAX_QUERY_LENGTH];
    snprintf(q, MAX_QUERY_LENGTH,
             "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    for (int i = 0; i < 100; ++i)
        pquv_query(pquv, q, multiple_reads_cb, &t);

    while (uv_run(&loop, UV_RUN_ONCE) && !ok);

    pquv_free(pquv);
    close_loop(loop);
    test_done();
}


typedef struct {
    volatile bool* ok;
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

    *t->ok = true;
    PQclear(r);
}

void simple_write()
{
    test_start();
    PGconn* conn = connect_blk();

    some(key); some(data); fresh(tbl);

    new_loop(loop);
    pquv_t* pquv = pquv_init(conninfo(), &loop);

    simple_write_t t = {
        .ok = &ok,
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

    while (uv_run(&loop, UV_RUN_ONCE) && !ok);

    pquv_free(pquv);
    close_loop(loop);
    test_done();
}



typedef struct {
    volatile bool* ok;
} invalid_query_t;

static void invalid_query_cb(void* opaque, PGresult* r)
{
    assert(PQresultStatus(r) == PGRES_FATAL_ERROR);
    invalid_query_t* t = (invalid_query_t*)opaque;
    *t->ok = true;
    PQclear(r);
}

void invalid_query()
{
    test_start();
    new_loop(loop);
    pquv_t* pquv = pquv_init(conninfo(), &loop);

    invalid_query_t t = { .ok = &ok };

    pquv_query(pquv, "SELECT * FROM lol-table", invalid_query_cb, &t);

    while (uv_run(&loop, UV_RUN_ONCE) && !ok);

    pquv_free(pquv);
    close_loop(loop);
    test_done();
}

typedef struct {
    volatile bool* ok;
    volatile bool cleaned_up;
    volatile bool tables_flushed;
    uv_timer_t timer;
} initial_bad_connection_t;

static void initial_bad_connection_cb(void* opaque, PGresult* r)
{
    assert(PQresultStatus(r) == PGRES_TUPLES_OK);
    assert(PQntuples(r) == 1);
    assert(1 == ntohl(*((uint32_t *)PQgetvalue(r, 0, 0))));

    initial_bad_connection_t* t = (initial_bad_connection_t*)opaque;
    assert(t->tables_flushed);
    *t->ok = true;
    PQclear(r);
}

static void initial_bad_connection_timer_cb(uv_timer_t* handle)
{
    dummy_pg_iptables_flush();
    initial_bad_connection_t* t = container_of(handle,
                                               initial_bad_connection_t,
                                               timer);
    t->tables_flushed = true;
}

static void initial_bad_connection_timer_free_cb(uv_handle_t* handle)
{
    initial_bad_connection_t* t = container_of(handle,
                                               initial_bad_connection_t,
                                               timer);
    t->cleaned_up = true;
}

void initial_bad_connection(volatile bool* ok)
{
    new_loop(loop);

    initial_bad_connection_t t = {
        .ok = ok,
        .cleaned_up = false,
        .tables_flushed = false
    };

    assert(0 == uv_timer_init(&loop, &t.timer));
    assert(0 == uv_timer_start(&t.timer, initial_bad_connection_timer_cb,
                               1500, 0));

    pquv_t* pquv = pquv_init(conninfo_dummy(), &loop);
    pquv_query(pquv, "SELECT 1", initial_bad_connection_cb, &t);

    while (uv_run(&loop, UV_RUN_ONCE) && !*ok);

    pquv_free(pquv);
    uv_close((uv_handle_t*)&t.timer, initial_bad_connection_timer_free_cb);
    close_loop(loop);
    assert(t.cleaned_up);
}

void initial_drop_connection()
{
    test_start();
    dummy_pg_iptables_drop();
    initial_bad_connection(&ok);
    test_done();
}

void initial_reject_connection()
{
    test_start();
    dummy_pg_iptables_reject();
    initial_bad_connection(&ok);
    test_done();
}

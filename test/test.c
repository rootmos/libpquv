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

    exec_and_expect_ok(
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

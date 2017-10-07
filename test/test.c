#include "common.h"
#include <uv.h>
#include <stdbool.h>
#include <pquv.h>

void simple_read()
{
    test_start();
    PGconn* conn = connect_blk();

    some(key); some(data); fresh(tbl);

    exec_and_expect_ok(
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, key, data);

    pquv_init(conninfo(), uv_default_loop());

    volatile bool running = true;
    while (uv_run(uv_default_loop(), UV_RUN_ONCE) && running);

    test_ok();
}

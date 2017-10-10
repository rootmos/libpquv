#include "common.h"

#include <string.h>

void sanity_check()
{
    test_start();
    PGconn* conn = connect_blk();

    fresh(tbl);

    /* randomize some data */
    some(key);
    some(data);

    /* insert it */
    exec_and_expect_ok(conn,
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, key, data);

    /* fetch it */
    exec_and_expect(conn,
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    /* fetch some other key should not return anything */
    some(other_key);
    exec_and_expect(conn, { assert(PQntuples(r) == 0); },
                    "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    /* but after inserting some data for it */
    some(other_data);
    exec_and_expect_ok(conn,
            "INSERT INTO %s (id, blob) VALUES ('%s', '%s')",
            tbl, other_key, other_data);

    /* it should be there as well */
    exec_and_expect(conn,
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    /* after deleting the first value */
    exec_and_expect_ok(conn, "DELETE FROM %s WHERE id = '%s'", tbl, key);

    /* fetching it should return nothing */
    exec_and_expect(conn, { assert(PQntuples(r) == 0); },
                    "SELECT blob FROM %s WHERE id = '%s'", tbl, key);

    /* but the other value should still be there  */
    exec_and_expect(conn,
            {
                assert(PQntuples(r) == 1);
                assert(strcmp(PQgetvalue(r, 0, 0), other_data) == 0);
            },
            "SELECT blob FROM %s WHERE id = '%s'", tbl, other_key);

    ok = true;
    PQfinish(conn);
    test_done();
}

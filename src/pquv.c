#include "util.h"

#include <postgresql/libpq-fe.h>
#include <uv.h>

#include <assert.h> /* TODO: return proper failures */

void poll_cb(uv_poll_t* handle, int status, int events)
{
    panic("not implemented\n")
}

void pquv_init(const char* conninfo, uv_loop_t* loop)
{
    PGconn* conn = PQconnectStart(conninfo);
    assert(conn);
    assert(PQstatus(conn) != CONNECTION_BAD);

    int fd = PQsocket(conn);
    assert(fd >= 0);

    static uv_poll_t poll;

    int r;
    if((r = uv_poll_init(loop, &poll, fd)) != 0)
        panic("uv_poll_init: %s\n", uv_strerror(r));

    if((r = uv_poll_start(&poll, UV_READABLE | UV_WRITABLE, poll_cb)) != 0)
        panic("uv_poll_start: %s\n", uv_strerror(r));
}

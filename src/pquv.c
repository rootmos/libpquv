#include <pquv.h>
#include "util.h"

#include <postgresql/libpq-fe.h>
#include <uv.h>

#include <assert.h> /* TODO: return proper failures */

typedef struct req_ts {
    const char* q;
    struct req_ts* next;
} req_t;

typedef struct {
    req_t* head;
    req_t* tail;
} queue_t; 

struct pquv_st {
    uv_poll_t poll;
    PGconn* conn;
    req_t* live;
    queue_t queue;
};

req_t* dequeue(queue_t* queue)
{
    if (queue->head == NULL)
        return NULL;
    panic("not implemented\n");
}

void poll_cb(uv_poll_t* handle, int status, int events)
{
    pquv_t* pquv = container_of(handle, pquv_t, poll);
    switch(events) {
    case UV_WRITABLE:
        {
            req_t* r = dequeue(&pquv->queue);
            if (r == NULL)
                return;
            panic("not implemented\n");
        }
    case UV_READABLE:
        panic("not implemented\n");
    default:
        /* not interested */
        return;
    }
}

pquv_t* pquv_init(const char* conninfo, uv_loop_t* loop)
{
    pquv_t* pquv = malloc(sizeof(pquv_t));
    assert(pquv);
    pquv->queue.head = NULL;
    pquv->queue.tail = NULL;
    pquv->live = NULL;

    pquv->conn = PQconnectStart(conninfo);
    assert(pquv->conn);
    assert(PQstatus(pquv->conn) != CONNECTION_BAD);

    int fd = PQsocket(pquv->conn);
    assert(fd >= 0);

    int r;
    if((r = uv_poll_init(loop, &pquv->poll, fd)) != 0)
        panic("uv_poll_init: %s\n", uv_strerror(r));

    if((r = uv_poll_start(&pquv->poll,
                          UV_READABLE | UV_WRITABLE,
                          poll_cb)) != 0)
        panic("uv_poll_start: %s\n", uv_strerror(r));

    return pquv;
}

void pquv_free(pquv_t* pquv)
{
    panic("not implemented\n");
}

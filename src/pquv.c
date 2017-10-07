#include <pquv.h>
#include "util.h"

#include <postgresql/libpq-fe.h>
#include <uv.h>

#include <assert.h> /* TODO: return proper failures */
#include <string.h>

typedef struct req_ts {
    char* q;
    void* opaque;
    req_cb cb;
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

    req_t* t = queue->head;
    queue->head = t->next;
    t->next = NULL;
    return t;
}

static void maybe_send_req(pquv_t* pquv)
{
    if(pquv->live != NULL)
        return;

    assert(PQstatus(pquv->conn) != CONNECTION_BAD);

    req_t* r = dequeue(&pquv->queue);
    if(r == NULL)
        return;

    if(!PQsendQuery(pquv->conn, r->q))
        panic("PQsendQuery: %s\n", PQerrorMessage(pquv->conn));

    assert(PQflush(pquv->conn) >= 0);

    pquv->live = r;
}

void pquv_query(pquv_t* pquv, const char* q, req_cb cb, void* opaque)
{
    req_t* r = malloc(sizeof(pquv_t));
    r->q = strndup(q, MAX_QUERY_LENGTH);
    r->cb = cb;
    r->opaque = opaque;
    r->next = NULL;

    if(pquv->queue.head == NULL) {
        pquv->queue.head = r;
        pquv->queue.tail = r;
    } else {
        assert(pquv->queue.tail);
        assert(pquv->queue.tail->next == NULL);
        pquv->queue.tail->next = r;
    }
}

static void free_req(req_t* r) {
    free(r->q);
    free(r);
}

static void poll_cb(uv_poll_t* handle, int status, int events)
{
    assert(status >= 0);

    pquv_t* pquv = container_of(handle, pquv_t, poll);
    if (events & UV_WRITABLE) {
        int r = PQflush(pquv->conn);
        if (r < 0) {
            panic("PQflush failed");
        } else if (r == 0) {
            maybe_send_req(pquv);
        }
    }

    if (events & UV_READABLE) {
        if(!PQconsumeInput(pquv->conn))
            panic("PQsendQuery: %s\n", PQerrorMessage(pquv->conn));

        if(!PQisBusy(pquv->conn)) {
            assert(pquv->live);
            PGresult* r = PQgetResult(pquv->conn);
            pquv->live->cb(pquv->live->opaque, r);
            free_req(pquv->live);
            pquv->live = NULL;

            /* TODO: handle more results */
            assert(PQgetResult(pquv->conn) == NULL);
        }
    }

    return;
}

static void poll_connection(pquv_t* pquv);

static void connection_cb(uv_poll_t* handle, int status, int events)
{
    assert(status >= 0);
    assert((events & ~(UV_READABLE | UV_WRITABLE)) == 0);
    pquv_t* pquv = container_of(handle, pquv_t, poll);
    poll_connection(pquv);
}

static void poll_connection(pquv_t* pquv)
{
    int events;
    uv_poll_cb cb;

    switch(PQconnectPoll(pquv->conn)) {
    case PGRES_POLLING_READING:
        events = UV_READABLE;
        cb = connection_cb;
        break;
    case PGRES_POLLING_WRITING:
        events = UV_WRITABLE;
        cb = connection_cb;
        break;
    case PGRES_POLLING_OK:
        events = UV_WRITABLE | UV_READABLE;
        cb = poll_cb;
        break;
    default:
        panic("PQconnectPoll unexpected status\n");
    }

    int r;
    if((r = uv_poll_start(&pquv->poll, events, cb)) != 0)
        panic("uv_poll_start: %s\n", uv_strerror(r));
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

    /* TODO: dup socket
     * "The user should not close the socket while the handle is active."
     * */

    int r;
    if((r = uv_poll_init(loop, &pquv->poll, fd)) != 0)
        panic("uv_poll_init: %s\n", uv_strerror(r));

    poll_connection(pquv);

    return pquv;
}

void pquv_free(pquv_t* pquv)
{
    assert(0 == uv_poll_stop(&pquv->poll));

    PQfinish(pquv->conn);

    if(pquv->live != NULL)
        free_req(pquv->live);

    req_t* r = pquv->queue.head;
    while(r != NULL) {
        req_t* n = r->next;
        free_req(r);
        r = n;
    }
}

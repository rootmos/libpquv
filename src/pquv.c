#include <pquv.h>
#include "util.h"

#include <postgresql/libpq-fe.h>
#include <uv.h>

#include <assert.h> /* TODO: return proper failures */
#include <string.h>
#include <unistd.h>

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
    int fd;
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

    if(!PQsendQueryParams(pquv->conn, r->q, 0, NULL, NULL, NULL, NULL, 1))
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
        pquv->queue.tail = r;
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
        } else {
            panic("PQisBusy returned true, what to do?");
        }
    }

    return;
}

/* Non-blocking connection request
 *
 * Quoting from: https://www.postgresql.org/docs/9.6/static/libpq-connect.html
 *
 * To begin a nonblocking connection request,
 * call conn = PQconnectStart("connection_info_string"). If conn is null, then
 * libpq has been unable to allocate a new PGconn structure. Otherwise, a valid
 * PGconn pointer is returned (though not yet representing a valid connection
 * to the database). On return from PQconnectStart, call status =
 * PQstatus(conn). If status equals CONNECTION_BAD, PQconnectStart has failed.
 *
 * If PQconnectStart succeeds, the next stage is to poll libpq so that it can
 * proceed with the connection sequence. Use PQsocket(conn) to obtain the
 * descriptor of the socket underlying the database connection. Loop thus: If
 * PQconnectPoll(conn) last returned PGRES_POLLING_READING, wait until the
 * socket is ready to read (as indicated by select(), poll(), or similar system
 * function). Then call PQconnectPoll(conn) again. Conversely, if
 * PQconnectPoll(conn) last returned PGRES_POLLING_WRITING, wait until the
 * socket is ready to write, then call PQconnectPoll(conn) again. If you have
 * yet to call PQconnectPoll, i.e., just after the call to PQconnectStart,
 * behave as if it last returned PGRES_POLLING_WRITING. Continue this loop
 * until PQconnectPoll(conn) returns PGRES_POLLING_FAILED, indicating the
 * connection procedure has failed, or PGRES_POLLING_OK, indicating the
 * connection has been successfully made.
 */

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

    /*
     * From libuv documentation:
     * "The user should not close a file descriptor while it is being polled
     * by an active poll handle."
     * (http://docs.libuv.org/en/v1.x/poll.html)
     *
     * So: dup the fd so we can close it after we've called uv_poll_close
     *
     * From libpq documentation:
     * "On Unix, forking a process with open libpq connections can lead to
     * unpredictable results because the parent and child processes share the
     * same sockets and operating system resources.
     * (https://www.postgresql.org/docs/9.6/static/libpq-connect.html)
     *
     * So: make sure FD_CLOEXEC is set
     *
     * Note: this is also done internally in libpq, see:
     * https://github.com/postgres/postgres/blob/0ba99c84e8c7138143059b281063d4cca5a2bfea/src/interfaces/libpq/fe-connect.c#L2140
     */

    pquv->fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    assert(pquv->fd >= 0);

    int r;
    if((r = uv_poll_init(loop, &pquv->poll, pquv->fd)) != 0)
        panic("uv_poll_init: %s\n", uv_strerror(r));

    poll_connection(pquv);

    return pquv;
}

static void pquv_free_cb(uv_handle_t* h)
{
    pquv_t* pquv = container_of(h, pquv_t, poll);

    assert(0 == close(pquv->fd));

    PQfinish(pquv->conn);

    if(pquv->live != NULL)
        free_req(pquv->live);

    req_t* r = pquv->queue.head;
    while(r != NULL) {
        req_t* n = r->next;
        free_req(r);
        r = n;
    }

    free(pquv);
}

void pquv_free(pquv_t* pquv)
{
    assert(0 == uv_poll_stop(&pquv->poll));
    uv_close((uv_handle_t*)&pquv->poll, pquv_free_cb);
}

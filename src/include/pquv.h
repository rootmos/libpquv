#ifndef pquv_h
#define pquv_h

#include <uv.h>
#include <postgresql/libpq-fe.h>

struct pquv_st;
typedef struct pquv_st pquv_t;

pquv_t* pquv_init(const char* conninfo, uv_loop_t* loop);
void pquv_free(pquv_t* pquv);

/* its up to the receiver of the callback to call PQclear on `res` */
typedef void (*req_cb)(void* opaque, PGresult* res);

#define MAX_QUERY_LENGTH 2048
void pquv_query(pquv_t* pquv, const char* q, req_cb cb, void* opaque);

#endif

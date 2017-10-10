#ifndef pquv_h
#define pquv_h

#include <uv.h>
#include <postgresql/libpq-fe.h>

struct pquv_st;
typedef struct pquv_st pquv_t;

#define MAX_CONNINFO_LENGTH 1048
pquv_t* pquv_init(const char* conninfo, uv_loop_t* loop);
void pquv_free(pquv_t* pquv);

/* its up to the receiver of the callback to call PQclear on `res` */
typedef void (*req_cb)(void* opaque, PGresult* res);

#define MAX_QUERY_LENGTH 2048
void pquv_query_params(
        pquv_t* pquv,
        const char* q,
        int nParams,
        const char * const *paramValues,
        const int *paramLengths,
        const int *paramFormats,
        req_cb cb,
        void* opaque);

static inline void pquv_query(
        pquv_t* pquv,
        const char* q,
        req_cb cb,
        void* opaque)
{
    pquv_query_params(pquv, q, 0, NULL, NULL, NULL, cb, opaque);
}

#endif

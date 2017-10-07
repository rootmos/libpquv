#ifndef pquv_h
#define pquv_h

#include <uv.h>
#include <postgresql/libpq-fe.h>

struct pquv_st;
typedef struct pquv_st pquv_t;

pquv_t* pquv_init(const char* conninfo, uv_loop_t* loop);
void pquv_free(pquv_t* pquv);

#endif

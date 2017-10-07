#ifndef pquv_h
#define pquv_h

#include <uv.h>
#include <postgresql/libpq-fe.h>

void pquv_init(const char* conninfo, uv_loop_t* loop);

#endif

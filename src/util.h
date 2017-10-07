#ifndef util_h
#define util_h

#include <stdio.h>
#include <stdlib.h>

#define panic(...) {                                            \
    fprintf(stderr, "%s:%d: ",                                  \
            __extension__ __FUNCTION__,__extension__ __LINE__); \
    fprintf(stderr, __VA_ARGS__); exit(1);                      \
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif

#include "util.h"
#include <stdarg.h>

void __failwith(const char* caller, int line, const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    fprintf(stderr, "%s:%d: ", caller, line);
    vfprintf(stderr, fmt, vl);
    va_end(vl);

    abort();
}

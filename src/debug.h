#ifndef DEBUG_H
#define DEBUG_H

#include "memory.h"

#define malloc(size)       Malloc_(size)
#define calloc(n, size)    Malloc_(n * size)
#define realloc(ptr, size) Realloc_(ptr, size)
#define free(ptr)          Free_(ptr)
#define mdump(stream)      Mdump_(stream)
#define mcheck(stream)     Mcheck_(stream)

#endif /* DEBUG_H */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>
#include <stdlib.h>

#define Malloc_(size)       Allocator_malloc(size, __FILE__, __LINE__, __func__)
#define Realloc_(ptr, size) Allocator_realloc(ptr, size, __FILE__, __LINE__, __func__)
#define Free_(ptr)          Allocator_free(ptr)
#define Mdump_(stream)      Allocator_dump(stream, __FILE__, __LINE__, __func__)
#define Mcheck_()           Allocator_check(__FILE__, __LINE__, __func__)

void* Allocator_malloc  (size_t size, const char* file, int line, const char* func);
void* Allocator_realloc (void* ptr, size_t size, const char* file, int line, const char* func);
void  Allocator_free    (void* ptr);
void  Allocator_dump    (FILE* stream, const char* file, int line, const char* func);
void  Allocator_check   (const char* file, int line, const char* func);

#endif /* MEMORY_H */

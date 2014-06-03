#include "memory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct memoryblock MemoryBlock;
struct memoryblock
{
    size_t size;
    const char* file;
    int line;
    const char* func;
    MemoryBlock* next;
    MemoryBlock* prev;
    unsigned char mark[8]; // mark_size
};

static const int init_char = 0xCC;
static const int mark_char = 0xCD;
static const int mark_size = 8;

static MemoryBlock* root = NULL;
static int allocNum = 0;
static int allocSize = 0;
static int mallocs = 0;
static int reallocs = 0;
static int frees = 0;

static void* toPtr(MemoryBlock* mb)
{
    return (void*) (((unsigned char*) mb) + sizeof(MemoryBlock));
}

static MemoryBlock* toMemoryBlock(void* ptr)
{
    return (MemoryBlock*) (((unsigned char*) ptr) - sizeof(MemoryBlock));
}

static void MemoryBlock_init(MemoryBlock* self, size_t size,
                             const char* file, int line, const char* func)
{
    self->size = size;
    self->file = file;
    self->line = line;
    self->func = func;
    self->next = NULL;
    self->prev = NULL;
    memset(toPtr(self) - mark_size, mark_char, mark_size);
    memset(toPtr(self) + self->size, mark_char, mark_size);
    memset(toPtr(self), init_char, size);
}

static MemoryBlock* MemoryBlock_malloc(size_t size,
                                       const char* file, int line, const char* func)
{
    MemoryBlock* mb = (MemoryBlock*) malloc(sizeof(MemoryBlock) + size + mark_size);
    if (mb == NULL) {
        fprintf(stderr, "%s:%d:%s: ERROR: out of memory: size=%d\n", file, line, func, size);
        exit(EXIT_FAILURE);
    }
    MemoryBlock_init(mb, size, file, line, func);
    allocSize += size;
    mallocs++;
    return mb;
}

static MemoryBlock* MemoryBlock_realloc(MemoryBlock* self, size_t size,
                                        const char* file, int line, const char* func)
{
    allocSize -= self->size;
    MemoryBlock* mb = (MemoryBlock*) realloc(self, sizeof(MemoryBlock) + size + mark_size);
    if (mb == NULL) {
        fprintf(stderr, "%s:%d:%s: ERROR: out of memory: size=%d\n", file, line, func, size);
        exit(EXIT_FAILURE);
    }
    MemoryBlock_init(mb, size, file, line, func);
    allocSize += size;
    reallocs++;
    return mb;
}

static void MemoryBlock_free(MemoryBlock* self)
{
    if (self == NULL) {
        return;
    }
    memset(toPtr(self), init_char, sizeof(MemoryBlock) + self->size + mark_size);
    allocSize -= self->size;
    free(self);
    frees++;
}

static void MemoryBlock_print(MemoryBlock* self, FILE* stream)
{
    void* ptr = toPtr(self);
    fprintf(stream, "*** %p: %s:%d:%s, size=%d\n",
            ptr, self->file, self->line, self->func, self->size);
}

static void MemoryBlock_checkMark(MemoryBlock* self, unsigned char* ptr,
                              const char* file, int line, const char* func)
{
    for (int i = 0; i < mark_size; i++) {
        if (ptr[i] != mark_char) {
            fprintf(stderr, "%s:%d:%s: segmentation fault: %p => %d\n",
                    file, line, func, &ptr[i], ptr[i]);
            MemoryBlock_print(self, stderr);
            abort();
        }
    }
}

static void MemoryBlock_check(MemoryBlock* self,
                              const char* file, int line, const char* func)
{
    unsigned char* head = toPtr(self) - mark_size;
    MemoryBlock_checkMark(self, head, file, line, func);
    unsigned char* tail = toPtr(self) + self->size;
    MemoryBlock_checkMark(self, tail, file, line, func);
}

static void insert(MemoryBlock* mb)
{
    if (root == NULL) {
        root = mb;
        mb->prev = NULL;
        mb->next = NULL;
    } else {
        mb->next = root;
        root->prev = mb;
        root = mb;
    }
    allocNum++;
}

static void insertAfter(MemoryBlock* mb, MemoryBlock* at)
{
    if (at == NULL) {
        insert(mb);
    } else {
        mb->prev = at;
        mb->next = at->next;
        if (at->next != NULL) {
            at->next->prev = mb;
        }
        at->next = mb;
        allocNum++;
    }
}

static void delete(MemoryBlock* mb)
{
    if (mb->prev != NULL) {
        mb->prev->next = mb->next;
    } else {
        root = mb->next;
    }
    if (mb->next != NULL) {
        mb->next->prev = mb->prev;
    }
    allocNum--;
}

void* Allocator_malloc(size_t size, const char* file, int line, const char* func)
{
    MemoryBlock* mb = MemoryBlock_malloc(size, file, line, func);
    insert(mb);
    return toPtr(mb);
}

void* Allocator_realloc(void* ptr, size_t size, const char* file, int line, const char* func)
{
    if (ptr) {
        MemoryBlock* mb = toMemoryBlock(ptr);
        MemoryBlock* prev = mb->prev;
        delete(mb);
        MemoryBlock* newmb = MemoryBlock_realloc(mb, size, file, line, func);
        insertAfter(newmb, prev);
        return toPtr(newmb);
    }
    return Allocator_malloc(size, file, line, func);
}

void Allocator_free(void* ptr)
{
    MemoryBlock* mb = toMemoryBlock(ptr);
    delete(mb);
    MemoryBlock_free(mb);
}

void Allocator_dump(FILE* stream,
                    const char* file, int line, const char* func)
{
    fprintf(stream, "%s:%d:%s: memory dump - "
            "[num: %d, size: %d, mallocs: %d, reallocs: %d, frees: %d]\n",
            file, line, func, allocNum, allocSize, mallocs, reallocs, frees);
    for (MemoryBlock* mb = root; mb; mb = mb->next) {
        MemoryBlock_print(mb, stream);
    }
}

void Allocator_check(const char* file, int line, const char* func)
{
    for (MemoryBlock* mb = root; mb; mb = mb->next) {
        MemoryBlock_check(mb, file, line, func);
    }
}

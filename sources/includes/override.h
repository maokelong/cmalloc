#ifndef CMALLOC_OVERRIDE_H
#define CMALLOC_OVERRIDE_H

#include "cmalloc.h"

#define ALIAS(cmalloc_fn) __attribute__((weak, alias(#cmalloc_fn)))

void *malloc(size_t size) ALIAS(cmalloc_malloc);
void free(void *p) ALIAS(cmalloc_free);
void *calloc(size_t nmemb, size_t size) ALIAS(cmalloc_calloc);
void *realloc(void *ptr, size_t size) ALIAS(cmalloc_realloc);
void *memalign(size_t __alignment, size_t __size) ALIAS(cmalloc_memalign);
void *aligned_alloc(size_t alignment, size_t size) ALIAS(cmalloc_aligned_alloc);
int posix_memalign(void **ptr, size_t align, size_t size)
    ALIAS(cmalloc_posix_memalign);
void *valloc(size_t __size) ALIAS(cmalloc_valloc);
void *pvalloc(size_t __size) ALIAS(cmalloc_pvalloc);
void malloc_stats(void) ALIAS(cmalloc_trace);
int mallopt(int cmd, int value) ALIAS(cmalloc_mallopt);

#undef ALIAS

#endif // end of CMALLOC_OVERRIDE_H
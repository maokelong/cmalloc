#ifndef CMALLOC_CMALLOC_H
#define CMALLOC_CMALLOC_H

#include <unistd.h>

void *cmalloc_aligned_alloc(size_t alignment, size_t size);
void *cmalloc_calloc(size_t nmemb, size_t size);
void cmalloc_free(void *ptr);
void *cmalloc_malloc(size_t size);
void *cmalloc_realloc(void *ptr, size_t size);

void *cmalloc_memalign(size_t boundary, size_t size);
int cmalloc_posix_memalign(void **memptr, size_t alignment, size_t size);
void *cmalloc_valloc(size_t size);
void *cmalloc_pvalloc(size_t size);

/*
 * parameter_number:
 * - 0: to change the ratio of cold superblocks to liquid superblocks.
        0: freeze most empty(cold) superblocks
        100: dont freeze any empty(cold) superblocks
        less may save more memory
 * - 1: to change the ratio of frozen superblocks to liquid superblocks.\
        0: return most frozen superblosk to the global pool
        less may save more memory for more metadata reusing.
*/
int cmalloc_mallopt(int parameter_number, int parameter_value);
void cmalloc_trace(void);

#endif // end of CMALLOC_CMALLOC_H
#ifndef CMALLOC_H
#define CMALLOC_H

#include <unistd.h>
/*Standard (ISO)  functions:*/

void *aligned_alloc(size_t alignment, size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

/*Additional functions*/
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
int mallopt(int parameter_number, int parameter_value);
void malloc_trace(void);

#endif // end of CMALLOC_H
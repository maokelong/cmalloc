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
int mallopt(int parameter_number, int parameter_value);
void malloc_trace(void);

#endif // end of CMALLOC_H
#include "includes/cmalloc.h"
#include "includes/assert.h"
#include "includes/heaps.h"
#include "includes/override.h"
#include "includes/size_class.inl.h"
#include "includes/system.inl.h"
#include <stdlib.h>
#include <string.h>

/*******************************************
 * Global definations
 *******************************************/

typedef enum { UNINITIALIZED, INITIALIZED } init_state;

/*******************************************
 * Global metadata
 *******************************************/
init_state GLOBAL_STATE = UNINITIALIZED;
global_pool GLOBAL_POOL;
pthread_once_t GLOBAL_INIT_ONCE_CONTROL = PTHREAD_ONCE_INIT;
pthread_key_t KEY_DESTRUCTOR;

/*******************************************
 * Per-thread metadata
 *******************************************/
thread_local init_state THREAD_STATE = UNINITIALIZED;
thread_local thread_local_heap *THREAD_LOCAL_HEAP = NULL;

/*******************************************
 * internal functions
 *******************************************/

static void thread_exit(void) {
  global_pool_deallocate_heap(THREAD_LOCAL_HEAP);
}

static void global_init(void) {
  // Register the destructor
  pthread_key_create(&KEY_DESTRUCTOR, (void *)thread_exit);

  global_pool_init();
  rev_addr_hashset_init();
  GLOBAL_STATE = INITIALIZED;
}

static void thread_init(void) {
  // Active the destructor
  pthread_setspecific(KEY_DESTRUCTOR, ACTIVE);

  // Initialize thread pool
  THREAD_LOCAL_HEAP = global_pool_allocate_heap();
  THREAD_STATE = INITIALIZED;
}

static inline void check_init(void) {
  if (unlikely(THREAD_STATE != INITIALIZED)) {
    if (unlikely(GLOBAL_STATE != INITIALIZED)) {
      pthread_once(&GLOBAL_INIT_ONCE_CONTROL, global_init);
    }
    thread_init();
  }
}

static inline void small_free(void *ptr) {
  thread_local_heap_deallocate(THREAD_LOCAL_HEAP, ptr);
}

static inline void large_free(void *ptr) { large_block_deallocate(ptr); }

static inline void *small_malloc(int size_class) {
  return thread_local_heap_allocate(THREAD_LOCAL_HEAP, size_class);
}

static inline void *large_malloc(size_t size) {
  return large_block_allocate(size);
}

/*******************************************
 * API implementation
 *******************************************/

void *cmalloc_malloc(size_t size) {
  void *ret = NULL;
  if (size != 0) {
    check_init();
    int size_class = size_to_size_class(size);
    if (likely(size_class_in_tiny_or_medium(size_class)))
      ret = small_malloc(size_class);
    else
      ret = large_malloc(size);
  }
  return ret;
}

void cmalloc_free(void *ptr) {
  if (ptr != NULL) {
    if (likely(global_pool_check_data_addr(ptr)))
      small_free(ptr);
    else
      large_free(ptr);
  }
}

void *cmalloc_aligned_alloc(size_t alignment, size_t size) {
  error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                 "aligned_alloc() called but not implemented");
  return NULL;
}

void *cmalloc_calloc(size_t nmemb, size_t size) {
  void *ptr;
  ptr = cmalloc_malloc(nmemb * size);
  if (!ptr) {
    return NULL;
  }

  int i;
  for (i = 0; i < size; ++i)
    *(char *)(ptr + i) = 0;

  return ptr;
}

void *cmalloc_realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    void *ret = cmalloc_malloc(size);
    return ret;
  }

  if (size == 0) {
    cmalloc_free(ptr);
    return NULL;
  }

  // check if ptr is on the tlh
  if (global_pool_check_data_addr(ptr)) {
    int ori_size_class = super_block_data_to_size_class(ptr);
    int new_size_class = size_to_size_class(size);

    if (ori_size_class == new_size_class)
      return ptr;

    // request a new data block and conduct the copy oper
    size_t ori_size = size_class_block_size(new_size_class);
    size_t smaller_size = ori_size < size ? ori_size : size;
    void *new_ptr = cmalloc_malloc(size);
    memcpy(new_ptr, ptr, smaller_size);
    cmalloc_free(ptr);

    return new_ptr;
  } else {
    int ori_size = large_block_get_header(ptr)->size;
    int new_size = large_block_aligned_size(size);

    if (new_size == ori_size)
      return ptr;

    // try to request a new data block & conduct the copy oper
    size_t smaller_size = ori_size < size ? ori_size : size;

    void *new_ptr = cmalloc_malloc(size);
    memcpy(new_ptr, ptr, smaller_size);
    cmalloc_free(ptr);

    return large_block_init(new_ptr, new_size)->mem;
  }
}

void *cmalloc_memalign(size_t boundary, size_t size) {
  /* Deal with zero-size allocation */
  if (size == 0)
    return NULL;

  if (boundary <= 256 && size <= SIZE_SDB) {
    /* In this case, we handle it as small allocations */
    int boundary_cls = size_to_size_class(boundary);
    int size_cls = size_to_size_class(size);
    int alloc_cls = boundary_cls > size_cls ? boundary_cls : size_cls;
    return small_malloc(alloc_cls);
  } else {
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "memalign() called but not fully implemented!");
    return NULL;
  }
}

int cmalloc_posix_memalign(void **memptr, size_t alignment, size_t size) {
  *memptr = cmalloc_memalign(alignment, size);
  if (*memptr) {
    return 0;
  } else {
    /* We have to "personalize" the return value according to the error */
    return -1;
  }
}

void *cmalloc_valloc(size_t size) { return cmalloc_memalign(PAGE_SIZE, size); }

void *cmalloc_pvalloc(size_t size) {
  error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                 "pvalloc() called but not implemented");
  return NULL;
}

void cmalloc_trace(void) { thread_local_heap_trace(&THREAD_LOCAL_HEAP); }

int cmalloc_mallopt(int parameter_number, int parameter_value) {
  switch (parameter_number) {
  case 0:
    if (parameter_value < 0)
      error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                     "invalid paramteter value");
    setRatioColdLiquid(parameter_value);
    break;
  case 1:
    if (parameter_value < 0)
      error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                     "invalid paramteter value");
    setRatioFrozenLiquid(parameter_value);
    break;
  default:
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "invalid paramteter number");
    return -1;
  }
  return 0;
}

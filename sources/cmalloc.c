#include "includes/cmalloc.h"
#include "includes/heaps.h"
#include "includes/size_classes.h"

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

static void *small_malloc(int size_class) {
  return thread_local_heap_allocate_data_block(THREAD_LOCAL_HEAP, size_class);
}

static void *large_malloc(size_t size) {
  // TODO: add support for large malloc
  return NULL;
}

static void thread_exit(void) {
  global_pool_deallocate_heap(THREAD_LOCAL_HEAP);
}

static void global_init(void) {
  // Register the destructor
  pthread_key_create(&KEY_DESTRUCTOR, (void *)thread_exit);

  // Init inner datastructures
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

inline static void check_init(void) {
  if (unlikely(THREAD_STATE != INITIALIZED)) {
    if (GLOBAL_STATE != INITIALIZED) {
      pthread_once(&GLOBAL_INIT_ONCE_CONTROL, global_init);
    }
    thread_init();
  }
}
/*******************************************
 * API implementation
 *******************************************/

void *malloc(size_t size) {
  void *ret = NULL;
  if (size == 0)
    return ret;

  // 初始化内存分配器
  check_init();

  // 计算 size 对应的 size class
  int size_class = SizeToSizeClass(size);

  // 分配内存
  if (likely(InSmallSize(size_class)))
    ret = small_malloc(size_class);
  else
    ret = large_malloc(size);

  return ret;
}

void free(void *ptr) {
  if (ptr == NULL)
    return;

  if (ptr < GLOBAL_POOL.data_pool.pool_start ||
      ptr > GLOBAL_POOL.data_pool.pool_end)
    ;
  else
    thread_local_heap_deallocate_data_block(THREAD_LOCAL_HEAP, ptr);
}

int mallopt(int parameter_number, int parameter_value);

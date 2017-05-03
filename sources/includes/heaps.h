#ifndef HEAPS_H
#define HEAPS_H
/*
 * @文件名：heaps.h
 *
 * @说明：描述线程本地堆，全局池
 *
 */
#include <pthread.h>
#include <unistd.h>

#include "cds.h"
#include "globals.h"
#include "sds.h"

/*******************************************
 * 结构声明
 *******************************************/

typedef struct super_meta_block_ super_meta_block;
typedef void super_data_block;
typedef struct shadow_block_ shadow_block;
typedef struct global_meta_pool_ global_meta_pool;
typedef struct global_data_pool_ global_data_pool;
typedef struct thread_local_heap_ thread_local_heap;
typedef struct global_pool_ global_pool;
typedef struct global_heap_pool_ global_heap_pool;
typedef struct large_block_header_ large_block_header;

/*******************************************
 * 全局变量声明
 *******************************************/
extern global_pool GLOBAL_POOL;

/*******************************************
 * 超级元数据块
 *******************************************/

typedef enum { hot, warm, cold, frozen } life_cycle;

// 影子块描述符
struct shadow_block_ {
  seq_queue_head prev_head;
};

// 超级元数据块描述符
struct super_meta_block_ {
  union {
    seq_queue_head prev_sb;              // tlh: hot/cold/frozen
    double_list_elem dl_elem;            // tlh: warm
    cache_aligned mc_queue_head mc_elem; // global pool:
  } list_elem;
  sc_queue_head prev_cool_sb; // tlh: cool
  thread_local_heap *owner_tlh;
  int num_allocated_and_remote_blocks;
  void *end_addr;
  seq_queue_head local_free_blocks;
  counted_queue_head remote_freed_blocks;
  void *clean_zone;
  int size_class;
  life_cycle cur_cycle;
  void *own_sdb;
};

// 大内存描述符
struct large_block_header_ {
  void *mem;
  size_t size;
};

/*******************************************
 * 线程本地堆
 *******************************************/

// 线程本地堆定义
struct thread_local_heap_ {
  cache_aligned mc_queue_head freed_list;
  seq_queue_head hot_sbs[NUM_SIZE_CLASSES];
  double_list warm_sbs[NUM_SIZE_CLASSES];
  seq_queue_head cold_sbs[NUM_SIZE_CLASSES];
  seq_queue_head frozen_sbs[NUM_SIZE_CLASSES];
  sc_queue_head cool_sbs[NUM_SIZE_CLASSES];

  size_t num_cold_sbs[NUM_SIZE_CLASSES];
  size_t num_liquid_sbs[NUM_SIZE_CLASSES];
  size_t num_frozen_sbs[NUM_SIZE_CLASSES];

  pthread_t holder_thread;
};
/*******************************************
 * 全局池
 *******************************************/

// 全局元数据池描述符
struct global_meta_pool_ {
  volatile void *pool_start;
  volatile void *pool_end;
  volatile void *pool_clean;

  // reusable_heaps[core id]
  mc_queue_head *reusable_heaps;
  // reusable_sbs[size class][core id]
  mc_queue_head *reusable_sbs[NUM_SIZE_CLASSES];
};

// 全局数据池描述符
struct global_data_pool_ {
  volatile void *pool_start;
  volatile void *pool_end;
  volatile void *pool_clean;
};

// 全局池描述符
struct global_pool_ {
  global_meta_pool meta_pool;
  global_data_pool data_pool;
};

/*******************************************
 interfaces
********************************************/
// 初始化
void global_pool_init(void);
void rev_addr_hashset_init(void);

int global_pool_check_addr(void *addr);
int super_block_data_to_size_class(void *addr);
size_t large_block_aligned_size(size_t ori_size);
large_block_header *large_block_get_header(void *usr_ptr);
large_block_header *large_block_init(void *raw_ptr, size_t size);

thread_local_heap *global_pool_allocate_heap(void);
void global_pool_deallocate_heap(thread_local_heap *tlh);

void *thread_local_heap_allocate(thread_local_heap *tlh, int size_class);
void thread_local_heap_deallocate(thread_local_heap *tlh, void *data_block);

void *large_block_allocate(size_t size);
void large_block_deallocate(void *usr_ptr);

// trace
void thread_local_heap_trace(thread_local_heap **pTlh);

#endif // end of HEAPS_H
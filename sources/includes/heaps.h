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
  seq_queue_head prev_sb;
  sc_queue_head prev_cool_sb;
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

/*******************************************
 * 线程本地堆
 *******************************************/

// 线程本地堆定义
struct thread_local_heap_ {
  cache_aligned mc_queue_head freed_list;
  seq_queue_head hot_sbs[NUM_SIZE_CLASSES + 1];
  seq_queue_head warm_sbs[NUM_SIZE_CLASSES + 1];
  seq_queue_head cold_sbs[NUM_SIZE_CLASSES + 1];
  seq_queue_head frozen_sbs[NUM_SIZE_CLASSES + 1];
  sc_queue_head cool_sbs[NUM_SIZE_CLASSES + 1];

  size_t num_cold_sbs;
  size_t num_liquid_sbs;
  pthread_t hold_thread;
};
/*******************************************
                                全局池
********************************************/

// 全局元数据池描述符
struct global_meta_pool_ {
  volatile void *pool_start;
  volatile void *pool_end;
  volatile void *pool_clean;

  // dropped routine(may be used in future)
  mc_queue_head reusable_sbs[NUM_SIZE_CLASSES + 1];
  mc_queue_head *reusable_heaps; // dynamic array
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

// 验证
int global_pool_check_addr(void *addr);

// 池分配
thread_local_heap *global_pool_allocate_heap(void);
void global_pool_deallocate_heap(thread_local_heap *tlh);

// 堆分配
void *thread_local_heap_allocate(thread_local_heap *tlh, int size_class);
void thread_local_heap_deallocate(thread_local_heap *tlh, void *data_block);

// trace
void thread_local_heap_trace(thread_local_heap **pTlh);

#endif // end of HEAPS_H
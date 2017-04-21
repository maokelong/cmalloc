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
                结构声明
********************************************/

typedef struct super_meta_block_ super_meta_block;
typedef void super_data_block;
typedef struct shadow_block_ shadow_block;
typedef struct global_meta_pool_ global_meta_pool;
typedef struct global_data_pool_ global_data_pool;
typedef struct thread_local_heap_ thread_local_heap;
typedef struct global_pool_ global_pool;
typedef struct global_heap_pool_ global_heap_pool;

/*******************************************
                全局变量声明
********************************************/
extern global_pool GLOBAL_POOL;

/*******************************************
超级元数据块
********************************************/

typedef enum { hot, warm, cold } life_cycle;

// 影子块描述符
struct shadow_block_ {
  seq_queue_head prev_head;
};

// 超级元数据块描述符
struct super_meta_block_ {
  seq_queue_head prev_smb;
  void *end_addr;
  seq_queue_head freed_blocks;
  void *clean_zone;
  int size_class;
  life_cycle cur_cycle;
  void *own_sdb;
};

/*******************************************
线程本地堆
********************************************/

// 线程本地堆定义
struct thread_local_heap_ {
  cache_aligned mc_queue_head freed_list;
  // hot：至少有一个可用block
  seq_queue_head hot_smbs[NUM_SIZE_CLASSES + 1];
  // warm：所有blocks均以用尽
  seq_queue_head warm_smbs[NUM_SIZE_CLASSES + 1];
  // cold：全部 blocks 都可用
  seq_queue_head cold_smbs[NUM_SIZE_CLASSES + 1];

  size_t num_cold_smbs;
};
/*******************************************
                全局池
********************************************/

// 全局元数据池描述符
struct global_meta_pool_ {
  volatile void *pool_start;
  volatile void *pool_end;
  volatile void *pool_clean;
  mc_queue_head reusable_smbs[NUM_SIZE_CLASSES + 1];
  mc_queue_head reusable_heaps;
};

// 全局数据池描述符
struct global_data_pool_ {
  volatile void *pool_start;
  volatile void *pool_end;
  volatile void *pool_clean;
};

// 全局池描述符
struct global_pool_ {
  pthread_spinlock_t spin_lock;
  global_meta_pool meta_pool;
  global_data_pool data_pool;
};

/*******************************************
 interfaces
********************************************/
// 初始化
void global_pool_init(void);
void reverse_addressing_hashset_init(void);

// 池分配
thread_local_heap *global_pool_allocate_heap(void);
void global_pool_deallocate_heap(thread_local_heap *tlh);

// 堆分配
void *thread_local_heap_allocate_data_block(thread_local_heap *tlh,
                                            int size_class);
void thread_local_heap_deallocate_data_block(thread_local_heap *tlh,
                                             void *data_block);

#endif // end of HEAPS_H
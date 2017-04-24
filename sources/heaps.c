#include "includes/heaps.h"
#include "includes/cds.inl.h"
#include "includes/sds.inl.h"
#include "includes/size_classes.h"
#include "includes/system.inl.h"
#include <stdbool.h>
#include <stdio.h>

/*******************************************
 * @ Object : reverse addressing hashset (rah)
 *******************************************/

super_meta_block **REV_ADDR_HASHSET = NULL;

static inline size_t rev_addr_hashset_index(void *data_block) {
  return (size_t)data_block / SIZE_SDB;
}

static inline void rev_addr_hashset_update(super_meta_block *smb, void *sdb) {
  REV_ADDR_HASHSET[rev_addr_hashset_index(sdb)] = smb;
}

static inline super_meta_block *rev_addr_hashset_get(size_t index) {
  return REV_ADDR_HASHSET[index];
}

static inline super_meta_block *rev_addr_hashset_db_to_smb(void *data_block) {
  size_t index_hashset = rev_addr_hashset_index(data_block);
  super_meta_block *ret = REV_ADDR_HASHSET[index_hashset];
  return ret;
}

/*******************************************
 * @ Object :super meta block (smb)
 *******************************************/

static inline size_t super_meta_block_size_class_to_size(int size_class) {
  // size of shadow blocks + size of super meta block
  return SIZE_SDB / SizeClassToBlockSize(size_class) * sizeof(void *) +
         sizeof(super_meta_block);
}

static inline size_t super_meta_block_total_size(int size_class) {
  return sizeof(super_meta_block) + SizeClassToNumDataBlocks(size_class);
}

static inline void *super_meta_block_end_address(super_meta_block *smb) {
  return smb + super_meta_block_total_size(smb->size_class);
}

static inline void *
super_meta_block_allocate_from_clean(super_meta_block *smb) {
  void *ret = smb->clean_zone;
  smb->clean_zone += sizeof(shadow_block);
  smb->num_allocated_blocks++;
  return ret;
}

static inline void
super_meta_block_try_flush_remote_free_blocks(super_meta_block *smb) {
  shadow_block *sb;
  if (smb->num_allocated_blocks == SizeClassToNumDataBlocks(smb->size_class)) {
    while ((sb = sc_dequeue(&smb->remote_free_blocks, 0)) != NULL) {
      seq_enqueue(&smb->local_free_blocks, sb);
    }
  }
}
/*******************************************
 * @ Object :super block (sb)
 *******************************************/

static inline void super_block_init(super_meta_block *smb,
                                    super_data_block *owm_sdb, int size_class) {
  // init smb
  smb->clean_zone = (void *)smb + sizeof(super_meta_block);
  smb->end_addr = (void *)smb + super_meta_block_size_class_to_size(size_class);
  smb->num_allocated_blocks = 0;
  seq_queue_init(&smb->prev_smb);
  seq_queue_init(&smb->local_free_blocks);
  sc_queue_init(&smb->remote_free_blocks);
  smb->cur_cycle = cold;
  smb->size_class = size_class;
  smb->own_sdb = owm_sdb;

  // update rah
  rev_addr_hashset_update(smb, owm_sdb);
}

static inline void super_block_init_except_own_sdb(super_meta_block *smb,
                                                   int size_class) {
  // init smb
  smb->clean_zone = (void *)smb + sizeof(super_meta_block);
  smb->end_addr = (void *)smb + super_meta_block_size_class_to_size(size_class);
  smb->num_allocated_blocks = 0;
  seq_queue_init(&smb->prev_smb);
  seq_queue_init(&smb->local_free_blocks);
  sc_queue_init(&smb->remote_free_blocks);
  smb->cur_cycle = cold;
  smb->size_class = size_class;
}

void super_block_convert_life_cycle(thread_local_heap *tlh,
                                    super_meta_block *smb,
                                    life_cycle target_life_cycle) {
  if (smb->cur_cycle == cold)
    tlh->num_cold_smbs--;

  switch (target_life_cycle) {
  case hot:
    seq_dequeue(&smb->prev_smb);
    seq_enqueue(&tlh->hot_smbs[smb->size_class], smb);
    smb->cur_cycle = hot;
    break;
  case warm:
    seq_dequeue(&smb->prev_smb);
    seq_enqueue(&tlh->warm_smbs[smb->size_class], smb);
    smb->cur_cycle = warm;
    break;
  case cold:
    seq_dequeue(&smb->prev_smb);
    seq_enqueue(&tlh->cold_smbs[smb->size_class], smb);
    smb->cur_cycle = cold;
    tlh->num_cold_smbs++;

    super_block_init_except_own_sdb(smb, smb->size_class);
    break;
  }
}

static inline void *super_block_db_to_shadow(void *data_block) {
  super_meta_block *smb = rev_addr_hashset_db_to_smb(data_block);
  size_t offset_data_block = (size_t)data_block - (size_t)smb->own_sdb;
  size_t size_data_block = SizeClassToBlockSize(smb->size_class);
  size_t sn_data_block = offset_data_block / size_data_block;

  void *correlated_shadow_block = (void *)smb + sizeof(super_meta_block) +
                                  sizeof(shadow_block) * sn_data_block;

  return correlated_shadow_block;
}

static inline void *super_block_shadow_to_db(super_meta_block *smb,
                                             shadow_block *shadowb) {
  // calc the serial number of given shadow block in given smb
  size_t sn_shadow_block;
  sn_shadow_block = ((size_t)shadowb - (size_t)smb - sizeof(super_meta_block)) /
                    sizeof(shadow_block);

  // return the address of correlated data block
  return smb->own_sdb + sn_shadow_block * SizeClassToBlockSize(smb->size_class);
}

static inline void *super_block_allocate_data_block(thread_local_heap *tlh,
                                                    super_meta_block *smb) {
  shadow_block *shadowb = NULL;

  // try to flush remote free blocks
  super_meta_block_try_flush_remote_free_blocks(smb);

  // try to allocate freed memory from hot smbs
  shadowb = seq_dequeue(&smb->local_free_blocks);

  // try to allocate clean memory from hot smb
  if (shadowb == NULL)
    shadowb = super_meta_block_allocate_from_clean(smb);

  // check weather need to convert current smb's life cycle
  if (smb->cur_cycle == cold)
    // cold -> hot
    super_block_convert_life_cycle(tlh, smb, hot);

  if (++smb->num_allocated_blocks == SizeClassToNumDataBlocks(smb->size_class))
    // hot -> warm
    super_block_convert_life_cycle(tlh, smb, warm);

  return super_block_shadow_to_db(smb, shadowb);
}

/*******************************************
 * @ Object :global pool
 *******************************************/

global_pool GLOBAL_POOL;

static inline super_meta_block *
global_meta_pool_allocate_raw_smb(int size_class) {
  super_meta_block *ret = NULL;

  ret = (super_meta_block *)GLOBAL_POOL.meta_pool.pool_clean;
  GLOBAL_POOL.meta_pool.pool_clean +=
      super_meta_block_size_class_to_size(size_class);
  return ret;
}

static inline mc_queue_head *
global_meta_pool_allocate_dynamic_array(size_t array_size) {
  mc_queue_head *ret = NULL;
  pthread_spin_lock(&GLOBAL_POOL.spin_lock);
  ret = (mc_queue_head *)GLOBAL_POOL.meta_pool.pool_clean;
  GLOBAL_POOL.meta_pool.pool_clean += array_size;
  pthread_spin_unlock(&GLOBAL_POOL.spin_lock);

  return ret;
}

static inline thread_local_heap *global_meta_pool_allocate_raw_heap(void) {
  thread_local_heap *ret = NULL;

  pthread_spin_lock(&GLOBAL_POOL.spin_lock);
  ret = (thread_local_heap *)GLOBAL_POOL.meta_pool.pool_clean;
  GLOBAL_POOL.meta_pool.pool_clean += sizeof(thread_local_heap);
  pthread_spin_unlock(&GLOBAL_POOL.spin_lock);
  return ret;
}

static inline super_data_block *global_pool_allocate_sdb(void) {
  super_data_block *ret = NULL;

  pthread_spin_lock(&GLOBAL_POOL.spin_lock);
  ret = (super_data_block *)GLOBAL_POOL.data_pool.pool_clean;
  GLOBAL_POOL.data_pool.pool_clean += SIZE_SDB;
  pthread_spin_unlock(&GLOBAL_POOL.spin_lock);

  return ret;
}

static inline super_meta_block **global_meta_pool_allocate_rah(void) {
  super_meta_block **ret = NULL;

  pthread_spin_lock(&GLOBAL_POOL.spin_lock);
  ret = (super_meta_block **)GLOBAL_POOL.meta_pool.pool_clean;
  GLOBAL_POOL.meta_pool.pool_clean += LENGTH_REV_ADDR_HASHSET;
  pthread_spin_unlock(&GLOBAL_POOL.spin_lock);

  return ret;
}

super_meta_block *global_pool_allocate_smb(int size_class) {
  super_meta_block *ret = NULL;

  // search freed smbs
  ret = (super_meta_block *)mc_dequeue(
      &GLOBAL_POOL.meta_pool.reusable_smbs[size_class], 0);

  if (ret == NULL) {
    // allocate and init a raw smb
    ret = global_meta_pool_allocate_raw_smb(size_class);
    super_data_block *sdb = global_pool_allocate_sdb();
    super_block_init(ret, sdb, size_class);
  } else {
    // init a freed smb
    super_block_init_except_own_sdb(ret, size_class);
  }

  return ret;
}

/*******************************************
 * @ Object :thread local heap(tlb)
 *******************************************/

static inline void thread_local_heap_init(thread_local_heap *tlh) {
  int i;
  for (i = 0; i < NUM_SIZE_CLASSES; ++i) {
    tlh->cold_smbs[i] = NULL;
    tlh->warm_smbs[i] = NULL;
    tlh->hot_smbs[i] = NULL;
  }
  tlh->num_cold_smbs = 0;
  tlh->hold_thread = pthread_self();
}

static inline void *
thread_local_heap_allocate_data_block_from_hot(thread_local_heap *tlh,
                                               int size_class) {
  super_meta_block *hot_smb = NULL;

  // check for hot smbs holding by the tlh
  hot_smb = (super_meta_block *)tlh->hot_smbs[size_class];
  if (hot_smb == NULL)
    return NULL;

  // allocate a data block within the hot sb
  return super_block_allocate_data_block(tlh, hot_smb);
}

static inline void *
thread_local_heap_allocate_data_block_from_cold(thread_local_heap *tlh,
                                                int size_class) {
  super_meta_block *cold_smb = NULL;

  // check for cold smbs within tlh
  cold_smb = (super_meta_block *)tlh->cold_smbs[size_class];
  if (cold_smb == NULL)
    return NULL;

  // allocate a clean block from the cold smb
  return super_block_allocate_data_block(tlh, cold_smb);
}

static inline void thread_local_heap_request_cold(thread_local_heap *tlh,
                                                  int size_class) {
  super_meta_block *cold_smb = NULL;

  // check freed smbs
  cold_smb = (super_meta_block *)mc_dequeue(
      &GLOBAL_POOL.meta_pool.reusable_smbs[size_class], 0);

  // allocate a raw smb
  if (cold_smb == NULL)
    cold_smb = (super_meta_block *)global_pool_allocate_smb(size_class);

  // insert the raw smb into cold smbs list
  super_block_convert_life_cycle(tlh, cold_smb, cold);
}

/*******************************************
 * @ Exposed interfaces
 *******************************************/

void global_pool_init(void) {
  int num_cores = get_num_cores();

  // init mutex lock
  pthread_spin_init(&GLOBAL_POOL.spin_lock, 0);

  // init global meta pool
  int i;
  GLOBAL_POOL.meta_pool.pool_start =
      require_vm(STARTA_ADDR_META_POOL, LENGTH_META_POOL);
  GLOBAL_POOL.meta_pool.pool_clean = GLOBAL_POOL.meta_pool.pool_start;
  GLOBAL_POOL.meta_pool.pool_end =
      GLOBAL_POOL.meta_pool.pool_start + LENGTH_META_POOL;
  GLOBAL_POOL.meta_pool.reusable_heaps =
      global_meta_pool_allocate_dynamic_array(sizeof(mc_queue_head) *
                                              num_cores);
  for (i = 0; i < num_cores; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_heaps[i]);
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_smbs[i]);

  // init global data pool
  GLOBAL_POOL.data_pool.pool_start =
      require_vm(STARTA_ADDR_DATA_POOL, LENGTH_DATA_POOL);
  GLOBAL_POOL.data_pool.pool_clean = GLOBAL_POOL.data_pool.pool_start;
  GLOBAL_POOL.data_pool.pool_end =
      GLOBAL_POOL.data_pool.pool_start + LENGTH_DATA_POOL;
}

void rev_addr_hashset_init(void) {
  REV_ADDR_HASHSET = global_meta_pool_allocate_rah();
}

thread_local_heap *global_pool_allocate_heap(void) {
  thread_local_heap *ret = NULL;

  // try to pick freed heap from global pool
  ret = mc_dequeue(&GLOBAL_POOL.meta_pool.reusable_heaps[get_core_id()], 0);

  // allocate and init a raw heap
  if (ret == NULL) {
    ret = global_meta_pool_allocate_raw_heap();
    thread_local_heap_init(ret);
  }
  return ret;
}

void global_pool_deallocate_heap(thread_local_heap *tlh) {
  mc_enqueue(&GLOBAL_POOL.meta_pool.reusable_heaps[get_core_id()], tlh, 0);
}

void *thread_local_heap_allocate_data_block(thread_local_heap *tlh,
                                            int size_class) {
  void *ret = NULL;

  // try to allocate a data block form hot smbs
  ret = thread_local_heap_allocate_data_block_from_hot(tlh, size_class);

  if (ret != NULL)
    return ret;

  // try to allocate a data block form cold smbs
  while (1) {
    ret = thread_local_heap_allocate_data_block_from_cold(tlh, size_class);

    if (ret == NULL)
      // failed in retrieving a cold super block from current tlh,
      // which shows that there is no cold super block on the tlh,
      // therefore we request a raw super block from global pool and mark it
      // cold
      thread_local_heap_request_cold(tlh, size_class);
    else
      break;
  }

  return ret;
}

void thread_local_heap_deallocate_data_block(thread_local_heap *tlh,
                                             void *data_block) {
  super_meta_block *smb;
  void *correlated_shadow_block;

  // target the correlated shadow block and the smb managing it
  correlated_shadow_block = super_block_db_to_shadow(data_block);
  smb = rev_addr_hashset_db_to_smb(data_block);

  // recycle the correlated shadow block
  if (tlh->hold_thread == pthread_self()) {
    seq_enqueue(&smb->local_free_blocks, correlated_shadow_block);
  } else {
    sc_enqueue(&smb->remote_free_blocks, correlated_shadow_block, 0);
  }
  smb->num_allocated_blocks--;

  // check the smb's life cycle
  switch (smb->cur_cycle) {
  case hot:
    if (smb->num_allocated_blocks == 0) {
      super_block_convert_life_cycle(tlh, smb, cold);
    }
    break;
  case warm:
    if (smb->cur_cycle == warm) {
      super_block_convert_life_cycle(tlh, smb, hot);
    }
    break;
  default:;
    // ERROR: impossible routine
  }
}
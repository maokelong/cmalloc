#include "includes/heaps.h"
#include "includes/cds.inl.h"
#include "includes/sds.inl.h"
#include "includes/size_classes.h"
#include "includes/system.inl.h"
#include <stdio.h>

/*******************************************
 * @ Object : reverse addressing hashset (rah)
 *******************************************/

super_meta_block **REVERSE_ADDRESSING_HASHSET = NULL;

static inline size_t reverse_addressing_hashset_index(void *data_block) {
  return (size_t)data_block / SIZE_SDB;
}

static inline void reverse_addressing_hashset_update(super_meta_block *smb,
                                                     void *sdb) {
  REVERSE_ADDRESSING_HASHSET[reverse_addressing_hashset_index(sdb)] = smb;
}

static inline super_meta_block *reverse_addressing_hashset_get(size_t index) {
  return REVERSE_ADDRESSING_HASHSET[index];
}

/*******************************************
 * @ Object :super meta block (smb)
 *******************************************/

static inline size_t super_meta_block_size_class_to_size(int size_class) {
  // size of shadow blocks + size of super meta block
  return SIZE_SDB / SizeClassToBlockSize(size_class) * sizeof(void *) +
         sizeof(super_meta_block);
}

static inline void *shadow_block_to_data_block(super_meta_block *smb,
                                               shadow_block *shadowb) {
  // calc the serial number of given shadow block in given smb
  size_t sn_shadow_block;
  sn_shadow_block = ((size_t)shadowb - (size_t)smb - sizeof(super_meta_block)) /
                    sizeof(shadow_block);

  // return the address of correlated data block
  return smb->own_sdb + sn_shadow_block * SizeClassToBlockSize(smb->size_class);
}

static inline size_t super_meta_block_total_size(int size_class) {
  return sizeof(super_meta_block) + SizeClassToNumDataBlocks(size_class);
}

static inline void *super_meta_block_end_address(super_meta_block *smb) {
  return smb + super_meta_block_total_size(smb->size_class);
}

/*******************************************
 * @ Object :super block (sb)
 *******************************************/

static inline void super_block_init(super_meta_block *smb,
                                    super_data_block *owm_sdb, int size_class) {
  // init smb
  smb->clean_zone = (void *)smb + sizeof(super_meta_block);
  smb->end_addr = (void *)smb + super_meta_block_size_class_to_size(size_class);
  seq_queue_init(&smb->prev_smb);
  smb->own_sdb = owm_sdb;
  seq_queue_init(&smb->freed_blocks);
  smb->cur_cycle = cold;
  smb->size_class = size_class;

  // update rah
  reverse_addressing_hashset_update(smb, owm_sdb);
}

static inline void super_block_init_except_own_sdb(super_meta_block *smb,
                                                   int size_class) {
  // init smb
  smb->clean_zone = (void *)smb + sizeof(super_data_block);
  smb->end_addr = (void *)smb + super_meta_block_size_class_to_size(size_class);
  seq_queue_init(&smb->prev_smb);
  seq_queue_init(&smb->freed_blocks);
  smb->cur_cycle = cold;
  smb->size_class = size_class;
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

  ret = (super_data_block *)GLOBAL_POOL.data_pool.pool_clean;
  GLOBAL_POOL.data_pool.pool_clean += SIZE_SDB;

  return ret;
}

static inline super_meta_block **global_meta_pool_allocate_rah(void) {
  super_meta_block **ret = NULL;

  ret = (super_meta_block **)GLOBAL_POOL.meta_pool.pool_clean;
  GLOBAL_POOL.meta_pool.pool_clean += LENGTH_REVERSE_ADDRESSING_HASHSET;

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

static inline void thread_local_heap_init(thread_local_heap *tlh) {
  int i;
  for (i = 0; i < NUM_SIZE_CLASSES; ++i) {
    tlh->cold_smbs[i] = NULL;
    tlh->warm_smbs[i] = NULL;
    tlh->hot_smbs[i] = NULL;
  }
  tlh->num_cold_smbs = 0;
}

/*******************************************
 * @ Object :thread local heap(tlb)
 *******************************************/
static inline void *
thread_local_heap_allocate_data_block_from_hot(thread_local_heap *tlh,
                                               int size_class) {
  shadow_block *shadowb = NULL;
  super_meta_block *hot_smb = NULL;

  // check hot smb
  hot_smb = (super_meta_block *)tlh->hot_smbs[size_class];
  if (hot_smb == NULL)
    return NULL;

  // hot smb < freed >
  shadowb = seq_dequeue(&hot_smb->freed_blocks);
  if (shadowb != NULL)
    return shadow_block_to_data_block(hot_smb, shadowb);

  // hot smb < new >
  shadowb = hot_smb->clean_zone;
  hot_smb->clean_zone += sizeof(shadowb);

  // hot -> warm ?
  if (hot_smb->clean_zone + sizeof(shadow_block) * 2 > hot_smb->end_addr) {
    seq_dequeue(&hot_smb->prev_smb);
    seq_enqueue(&tlh->warm_smbs[size_class], hot_smb);
  }

  return shadow_block_to_data_block(hot_smb, shadowb);
}

static inline void *
thread_local_heap_allocate_data_block_from_cold(thread_local_heap *tlh,
                                                int size_class) {
  shadow_block *shadowb = NULL;
  super_meta_block *hot_smb = NULL, *cold_smb = NULL;

  // check cold smb
  cold_smb = (super_meta_block *)tlh->cold_smbs[size_class];
  if (cold_smb == NULL)
    return NULL;

  // cold smb < new >
  shadowb = cold_smb->clean_zone;
  cold_smb->clean_zone += sizeof(shadow_block);

  // cold -> hot
  hot_smb = (super_meta_block *)&tlh->hot_smbs[size_class];
  seq_dequeue(&cold_smb->prev_smb);
  seq_enqueue(&hot_smb->prev_smb, cold_smb);

  return shadow_block_to_data_block(cold_smb, shadowb);
}

static inline void thread_local_heap_require_cold(thread_local_heap *tlh,
                                                  int size_class) {
  super_meta_block *cold_smb = NULL;

  // check freed smbs
  cold_smb = (super_meta_block *)mc_dequeue(
      &GLOBAL_POOL.meta_pool.reusable_smbs[size_class], 0);

  // allocate a raw smb
  if (cold_smb == NULL)
    cold_smb = (super_meta_block *)global_pool_allocate_smb(size_class);

  seq_enqueue(&tlh->cold_smbs[size_class], cold_smb);
}

/*******************************************
 * @ Exposed interfaces
 *******************************************/

void global_pool_init(void) {
  // init global meta pool
  int i;
  GLOBAL_POOL.meta_pool.pool_start =
      require_vm(STARTA_ADDR_META_POOL, LENGTH_META_POOL);
  GLOBAL_POOL.meta_pool.pool_clean = GLOBAL_POOL.meta_pool.pool_start;
  GLOBAL_POOL.meta_pool.pool_end =
      GLOBAL_POOL.meta_pool.pool_start + LENGTH_META_POOL;
  mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_heaps);
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_smbs[i]);

  // init global data pool
  GLOBAL_POOL.data_pool.pool_start =
      require_vm(STARTA_ADDR_DATA_POOL, LENGTH_DATA_POOL);
  GLOBAL_POOL.data_pool.pool_clean = GLOBAL_POOL.data_pool.pool_start;
  GLOBAL_POOL.data_pool.pool_end =
      GLOBAL_POOL.data_pool.pool_start + LENGTH_DATA_POOL;

  // init mutex lock
  pthread_spin_init(&GLOBAL_POOL.spin_lock, 0);
}

void reverse_addressing_hashset_init(void) {
  REVERSE_ADDRESSING_HASHSET = global_meta_pool_allocate_rah();
}

thread_local_heap *global_pool_allocate_heap(void) {
  thread_local_heap *ret = NULL;

  // try to pick freed heap from global pool
  ret = mc_dequeue(&GLOBAL_POOL.meta_pool.reusable_heaps, 0);

  // allocate and init a raw heap
  if (ret == NULL) {
    ret = global_meta_pool_allocate_raw_heap();
    thread_local_heap_init(ret);
  }
  return ret;
}

void global_pool_deallocate_heap(thread_local_heap *tlh) {
  mc_enqueue(&GLOBAL_POOL.meta_pool.reusable_heaps, tlh, 0);
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
      thread_local_heap_require_cold(tlh, size_class);
    else
      break;
  }

  return ret;
}

void thread_local_heap_deallocate_data_block(thread_local_heap *tlh,
                                             void *data_block) {
  super_meta_block *smb = NULL;

  // target the correlated smb
  size_t index_hashset = reverse_addressing_hashset_index(data_block);
  smb = REVERSE_ADDRESSING_HASHSET[index_hashset];

  // target the correlated shadow block
  size_t offset_data_block = (size_t)data_block - (size_t)smb->own_sdb;
  size_t size_data_block = SizeClassToBlockSize(smb->size_class);
  size_t sn_data_block = offset_data_block / size_data_block;

  void *correlated_shadow_block = (void *)smb + sizeof(super_meta_block) +
                                  sizeof(shadow_block) * sn_data_block;

  // recycle the correlated shadow block
  seq_enqueue(&smb->freed_blocks, correlated_shadow_block);

  // check the target smb's life cycle
  if (smb->cur_cycle == warm) {
    seq_dequeue(&smb->prev_smb);
    seq_enqueue(&tlh->hot_smbs[smb->size_class], smb);
    smb->cur_cycle = hot;
  }

  // TODO: put the extra cold super blocks on the global pool
}

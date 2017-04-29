#include "includes/heaps.h"
#include "includes/cds.inl.h"
#include "includes/sds.inl.h"
#include "includes/size_classes.h"
#include "includes/system.inl.h"
#include <stdbool.h>
#include <stdio.h>

/* reverse addressing hashset */
super_meta_block **REV_ADDR_HASHSET = NULL;
static inline size_t rev_addr_hashset_index(void *data_block);
static inline void rev_addr_hashset_update(super_meta_block *smb, void *sdb);
static inline super_meta_block *rev_addr_hashset_get(size_t index);
static inline super_meta_block *rev_addr_hashset_db_to_smb(void *data_block);

/* super block */
static inline size_t super_meta_block_size_class_to_size(int size_class);
static inline size_t super_meta_block_total_size(int size_class);
static inline void *
super_meta_block_allocate_freed_shadow(super_meta_block *smb);
static inline void *
super_meta_block_allocate_clean_shadow(super_meta_block *smb);

static inline void super_block_assemble(thread_local_heap *tlh,
                                        super_meta_block *raw_smb,
                                        super_data_block *raw_sdb,
                                        int size_class);
static inline void super_block_init_cached(thread_local_heap *tlh,
                                           super_meta_block *smb,
                                           int size_class);
static inline void *super_block_data_to_shadow(void *data_block);
static inline void *super_block_shadow_to_data(super_meta_block *smb,
                                               shadow_block *shadowb);
static inline void *super_block_allocate_data_block(thread_local_heap *tlh,
                                                    super_meta_block *smb);
static inline bool super_block_satisfy_frozen(thread_local_heap *tlh,
                                              super_meta_block *sb);
static inline bool super_block_satisfy_cold(super_meta_block *sb);
static inline bool super_block_satisfy_hot(super_meta_block *sb);
static inline bool super_block_satisfy_warm(super_meta_block *sb);
void super_block_check_life_cycle_and_update_counter_when_malloc(
    thread_local_heap *tlh, super_meta_block *sb);
void super_block_check_life_cycle_and_update_counter_when_free(
    thread_local_heap *tlh, super_meta_block *sb);
void super_block_convert_life_cycle(thread_local_heap *tlh,
                                    super_meta_block *sb,
                                    life_cycle target_life_cycle);
/* global pool */
global_pool GLOBAL_POOL;
static inline void global_meta_pool_init(void);
static inline void global_data_pool_init(void);
static inline bool global_pool_check_addr(void *addr);
static inline super_meta_block *global_pool_make_raw_smb(int size_class);
static inline void **global_pool_make_dynamic_array(size_t array_size);
static inline super_data_block *global_pool_make_raw_sdb(void);
static inline super_meta_block **global_pool_make_raw_rah(void);
static inline super_meta_block *global_pool_make_new_sb(thread_local_heap *tlh,
                                                        int size_class);
static inline super_meta_block *
global_pool_fetch_cached_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *global_pool_fetch_cached_or_make_new_sb(thread_local_heap *tlh,
                                                        int size_class);

/* thread local heap */
static inline void thread_local_heap_init(thread_local_heap *tlh);
static inline super_meta_block *thread_local_heap_get_sb(thread_local_heap *tlh,
                                                         int size_class);
static inline super_meta_block *
thread_local_heap_fetch_cached_hot_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *
thread_local_heap_fetch_cached_cold_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *
thread_local_heap_fetch_and_flush_cool_sb(thread_local_heap *tlh,
                                          int size_class);
static inline super_meta_block *
thread_local_heap_fetch_cached_or_new_frozen_sb(thread_local_heap *tlh,
                                                int size_class);

/*******************************************
 * @ Definition
 * @ Object : reverse addressing hashset (rah)
 *******************************************/
void rev_addr_hashset_init(void) {
  REV_ADDR_HASHSET = global_pool_make_raw_rah();
}

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
  super_meta_block *ret = rev_addr_hashset_get(index_hashset);
  return ret;
}

/*******************************************
 * @ Definition
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

static inline void *
super_meta_block_allocate_freed_shadow(super_meta_block *smb) {
  shadow_block *shadowb = NULL;
  shadowb = seq_dequeue(&smb->local_free_blocks);
  return shadowb;
}

static inline void *
super_meta_block_allocate_clean_shadow(super_meta_block *smb) {
  shadow_block *shadowb = NULL;
  shadowb = smb->clean_zone;
  smb->clean_zone += sizeof(shadow_block);
  return shadowb;
}
/*******************************************
 * @ Definition
 * @ Object :superblock (sb)
 *******************************************/

static inline void super_block_assemble(thread_local_heap *tlh,
                                        super_meta_block *raw_smb,
                                        super_data_block *raw_sdb,
                                        int size_class) {
  // init the raw super meta block first
  seq_queue_init(&raw_smb->prev_sb);
  sc_queue_init(&raw_smb->prev_cool_sb);
  raw_smb->owner_tlh = tlh;
  raw_smb->num_allocated_and_remote_blocks = 0;
  raw_smb->end_addr =
      (void *)raw_smb + super_meta_block_size_class_to_size(size_class);
  seq_queue_init(&raw_smb->local_free_blocks);
  sc_queue_init(&raw_smb->remote_freed_blocks);
  raw_smb->clean_zone = (void *)raw_smb + sizeof(super_meta_block);
  raw_smb->size_class = size_class;
  raw_smb->cur_cycle = cold;

  // Establish the link between the raw_smb and the raw_sdb
  // by recording the address of the super data block on the super meta block
  // and updating the reverse addressing hashset.
  raw_smb->own_sdb = raw_sdb;
  rev_addr_hashset_update(raw_smb, raw_sdb);
}

static inline void super_block_init_cached(thread_local_heap *tlh,
                                           super_meta_block *smb,
                                           int size_class) {
  // init the cached super meta block
  seq_queue_init(&smb->prev_sb);
  sc_queue_init(&smb->prev_cool_sb);
  smb->owner_tlh = tlh;
  smb->num_allocated_and_remote_blocks = 0;
  smb->end_addr = (void *)smb + super_meta_block_size_class_to_size(size_class);
  seq_queue_init(&smb->local_free_blocks);
  sc_queue_init(&smb->remote_freed_blocks);
  smb->clean_zone = (void *)smb + sizeof(super_meta_block);
  smb->size_class = size_class;
  smb->cur_cycle = cold;
}

static inline void *super_block_data_to_shadow(void *data_block) {
  super_meta_block *smb = rev_addr_hashset_db_to_smb(data_block);
  size_t offset_data_block = (size_t)data_block - (size_t)smb->own_sdb;
  size_t size_data_block = SizeClassToBlockSize(smb->size_class);
  size_t sn_data_block = offset_data_block / size_data_block;

  void *correlated_shadow_block = (void *)smb + sizeof(super_meta_block) +
                                  sizeof(shadow_block) * sn_data_block;

  return correlated_shadow_block;
}

static inline void *super_block_shadow_to_data(super_meta_block *smb,
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

  // try to fetch a freed shadow block from the superblock
  shadowb = super_meta_block_allocate_freed_shadow(smb);

  // try to fetch a clean shadow block from the superblock
  if (shadowb == NULL)
    shadowb = super_meta_block_allocate_clean_shadow(smb);

  smb->num_allocated_and_remote_blocks++;

  // check weather need to convert current superblock's life cycle
  super_block_check_life_cycle_and_update_counter_when_malloc(tlh, smb);

  return super_block_shadow_to_data(smb, shadowb);
}

static inline bool super_block_satisfy_frozen(thread_local_heap *tlh,
                                              super_meta_block *sb) {
  // Since the number of liquild superblocks held by the thread local heap is
  // too small,
  // we will not try to freeze any of these liquild superblocks.
  if (!tlh->num_liquid_sbs)
    return false;

  // Else, if the superblock satisfies the characteristics of cold
  // and the ratio of cold superblocks to liquid superblocks is larger than
  // FROZEN_RATIO,
  // we will freeze the superblock.
  int ratio_cold_liquild = tlh->num_cold_sbs * 100 / tlh->num_liquid_sbs;
  return super_block_satisfy_cold(sb) && (ratio_cold_liquild > FROZEN_RATIO);
}

static inline bool super_block_satisfy_cold(super_meta_block *sb) {
  // We think that a superblock does not allocate any datablock is a cold
  // superblock,
  // even though it could be a frozen one either
  return sb->num_allocated_and_remote_blocks == 0;
}

static inline bool super_block_satisfy_hot(super_meta_block *sb) {
  // We think that a superblock that could allocate datablock is a hot
  // superblock
  return sb->num_allocated_and_remote_blocks <
         SizeClassToNumDataBlocks(sb->size_class);
}

static inline bool super_block_satisfy_warm(super_meta_block *sb) {
  // We think that a superblock that could not allocate datablock is a warm
  // superblock,
  // even though it may maintain some remote freed datatblocks inside.
  return sb->num_allocated_and_remote_blocks >=
         SizeClassToNumDataBlocks(sb->size_class);
}

void super_block_check_life_cycle_and_update_counter_when_malloc(
    thread_local_heap *tlh, super_meta_block *sb) {
  sb->num_allocated_and_remote_blocks++;
  switch (sb->cur_cycle) {
  case frozen:
    // frozen -> warm ?
    if (super_block_satisfy_warm(sb)) {
      super_block_convert_life_cycle(tlh, sb, warm);
      tlh->num_liquid_sbs--;
    }
    // frozen -> hot ?
    else if (super_block_satisfy_hot(sb)) {
      super_block_convert_life_cycle(tlh, sb, hot);
      tlh->num_liquid_sbs--;
    }
    break;
  case cold:
    // cold -> warm ?
    if (super_block_satisfy_warm(sb)) {
      super_block_convert_life_cycle(tlh, sb, warm);
      tlh->num_cold_sbs--;
    }
    // cold -> hot ?
    else if (super_block_satisfy_hot(sb)) {
      super_block_convert_life_cycle(tlh, sb, hot);
      tlh->num_cold_sbs--;
    }
    break;
  case hot:
    // hot -> warm ?
    if (super_block_satisfy_warm(sb))
      super_block_convert_life_cycle(tlh, sb, warm);
    break;
  case warm:
    // warm -> frozen ?
    if (super_block_satisfy_frozen(tlh, sb)) {
      super_block_convert_life_cycle(tlh, sb, frozen);
      tlh->num_liquid_sbs--;
    }
    // warm -> cold ?
    else if (super_block_satisfy_cold(sb)) {
      super_block_convert_life_cycle(tlh, sb, cold);
      tlh->num_cold_sbs++;
    }
    // warm -> hot ?
    else if (super_block_satisfy_hot(sb))
      super_block_convert_life_cycle(tlh, sb, hot);
    break;
  }
}

void super_block_check_life_cycle_and_update_counter_when_free(
    thread_local_heap *tlh, super_meta_block *sb) {
  sb->num_allocated_and_remote_blocks--;
  switch (sb->cur_cycle) {
  case frozen:
  case cold:
    break;
  case hot:
    // hot -> frozen ?
    if (super_block_satisfy_frozen(tlh, sb)) {
      super_block_convert_life_cycle(tlh, sb, frozen);
      tlh->num_liquid_sbs--;
    }
    // hot -> cold ?
    else if (super_block_satisfy_cold(sb)) {
      super_block_convert_life_cycle(tlh, sb, cold);
      tlh->num_cold_sbs++;
    }
    break;
  case warm:
    // warm -> hot ?
    if (super_block_satisfy_frozen(tlh, sb)) {
      super_block_convert_life_cycle(tlh, sb, frozen);
      tlh->num_liquid_sbs--;
    }
    // warm -> frozen ?
    else if (super_block_satisfy_cold(sb)) {
      super_block_convert_life_cycle(tlh, sb, cold);
      tlh->num_cold_sbs++;
    }
    // warm -> cold ?
    else if (super_block_satisfy_hot(sb)) {
      super_block_convert_life_cycle(tlh, sb, hot);
    }
    break;
  }
}
void super_block_convert_life_cycle(thread_local_heap *tlh,
                                    super_meta_block *sb,
                                    life_cycle target_life_cycle) {
  switch (target_life_cycle) {
  case hot:
    seq_dequeue(&sb->prev_sb);
    seq_enqueue(&tlh->hot_sbs[sb->size_class], sb);
    sb->cur_cycle = hot;
    break;
  case warm:
    seq_dequeue(&sb->prev_sb);
    seq_enqueue(&tlh->warm_sbs[sb->size_class], sb);
    sb->cur_cycle = warm;
    break;
  case cold:
    seq_dequeue(&sb->prev_sb);
    seq_enqueue(&tlh->cold_sbs[sb->size_class], sb);
    sb->cur_cycle = cold;
    break;
  case frozen:
    seq_dequeue(&sb->prev_sb);
    seq_enqueue(&tlh->frozen_sbs[sb->size_class], sb);
    sb->cur_cycle = frozen;

    freeze_vm(sb->own_sdb, SIZE_SDB);
    break;
  }
}
/*******************************************
 * @ Definition
 * @ Object :global pool
 *******************************************/

void global_pool_init(void) {
  global_meta_pool_init();
  global_data_pool_init();
}

static inline void global_meta_pool_init(void) {
  int i, num_cores = get_num_cores();
  GLOBAL_POOL.meta_pool.pool_start =
      require_vm(STARTA_ADDR_META_POOL, LENGTH_META_POOL);
  GLOBAL_POOL.meta_pool.pool_clean = GLOBAL_POOL.meta_pool.pool_start;
  GLOBAL_POOL.meta_pool.pool_end =
      GLOBAL_POOL.meta_pool.pool_start + LENGTH_META_POOL;
  GLOBAL_POOL.meta_pool.reusable_heaps =
      (mc_queue_head *)global_pool_make_dynamic_array(sizeof(mc_queue_head) *
                                                       num_cores);
  for (i = 0; i < num_cores; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_heaps[i]);
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_sbs[i]);
}

static inline void global_data_pool_init(void) {
  GLOBAL_POOL.data_pool.pool_start =
      require_vm(STARTA_ADDR_DATA_POOL, LENGTH_DATA_POOL);
  GLOBAL_POOL.data_pool.pool_clean = GLOBAL_POOL.data_pool.pool_start;
  GLOBAL_POOL.data_pool.pool_end =
      GLOBAL_POOL.data_pool.pool_start + LENGTH_DATA_POOL;
}

static inline bool global_pool_check_addr(void *addr) {
  // check if the given address is valid,
  // i.e. either within the global meta pool or the global data pool
  if ((addr >= GLOBAL_POOL.meta_pool.pool_start &&
          addr <= GLOBAL_POOL.meta_pool.pool_end) ||
      (addr >= GLOBAL_POOL.data_pool.pool_start &&
          addr <= GLOBAL_POOL.data_pool.pool_end))
    return true;
  else
    return false;
}

static inline super_meta_block *global_pool_make_raw_smb(int size_class) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap
	volatile void **gmpool_start_addr = &GLOBAL_POOL.meta_pool.pool_clean;
  size_t smb_size = super_meta_block_size_class_to_size(size_class);

  // atomicly fetch and increase the start address of the global meta pool
  return (super_meta_block *)__sync_fetch_and_add(gmpool_start_addr, smb_size);
}

static inline void **global_pool_make_dynamic_array(size_t array_size) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap
  volatile void **gmpool_start_addr = &GLOBAL_POOL.meta_pool.pool_clean;

  // atomicly fetch and increase the start address of the global meta pool
  return __sync_fetch_and_add(gmpool_start_addr, array_size);
}

thread_local_heap *global_pool_make_raw_heap(void) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap
	volatile void **gmpool_start_addr = &GLOBAL_POOL.meta_pool.pool_clean;
  size_t heap_size = sizeof(thread_local_heap);

  // atomicly fetch and increase the start address of the global meta pool
  return (thread_local_heap *)__sync_fetch_and_add(gmpool_start_addr,
                                                   heap_size);
}

static inline super_data_block *global_pool_make_raw_sdb(void) {
  // get the address of the global data pool's start address
	volatile void **gdpool_start_addr = &GLOBAL_POOL.data_pool.pool_clean;

  // atomicly fetch and increase the start address of the global data pool
  return (super_data_block *)__sync_fetch_and_add(gdpool_start_addr, SIZE_SDB);
}

static inline super_meta_block **global_pool_make_raw_rah(void) {
  // get the address of the global meta pool's start address
	volatile void **gmpool_start_addr = &GLOBAL_POOL.meta_pool.pool_clean;

  // atomicly fetch and increase the start address of the global meta pool
  return (super_meta_block **)__sync_fetch_and_add(gmpool_start_addr,
                                                   LENGTH_REV_ADDR_HASHSET);
}

static inline super_meta_block *global_pool_make_new_sb(thread_local_heap *tlh,
                                                        int size_class) {
  super_meta_block *new_sb = NULL;

  // make a raw super meta block on the global meta pool
  super_meta_block *raw_smb = NULL;
  raw_smb = global_pool_make_raw_smb(size_class);

  // make a raw super data block on the global data pool
  super_data_block *raw_sdb = NULL;
  raw_sdb = global_pool_make_raw_sdb();

  // assemble the raw super meta block and the raw super data block
  // into a new superblock
  super_block_assemble(tlh, raw_smb, raw_sdb, size_class);

  return new_sb;
}

static inline super_meta_block *
global_pool_fetch_cached_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *cached_sb = NULL;

  // fetch a sb cached on the global pool
  cached_sb = (super_meta_block *)mc_dequeue(
      &GLOBAL_POOL.meta_pool.reusable_sbs[size_class], 0);

  // init the cached sb
  super_block_init_cached(tlh, cached_sb, size_class);

  return cached_sb;
}

static inline super_meta_block *global_pool_fetch_cached_or_make_new_sb(thread_local_heap *tlh,
                                                        int size_class) {
  super_meta_block *sb = NULL;

  // try to fetch a superblock cached on the global pool.
  sb = global_pool_fetch_cached_sb(tlh, size_class);

  if (sb == NULL)
    sb = global_pool_make_new_sb(tlh, size_class);

  return sb;
}

void global_pool_deallocate_heap(thread_local_heap *tlh) {
  mc_enqueue(&GLOBAL_POOL.meta_pool.reusable_heaps[get_core_id()],
             &tlh->freed_list, 0);
}

/*******************************************
 * @ Definition
 * @ Object :thread local heap(tlh)
 *******************************************/

static inline void thread_local_heap_init(thread_local_heap *tlh) {
  int i;
  for (i = 0; i < NUM_SIZE_CLASSES; ++i) {
    seq_queue_init(tlh->cold_sbs[i]);
    seq_queue_init(tlh->warm_sbs[i]);
    seq_queue_init(tlh->hot_sbs[i]);
    seq_queue_init(tlh->frozen_sbs[i]);
    mc_queue_init(&tlh->cool_sbs[i]);
  }

  mc_queue_init(&tlh->freed_list);
  tlh->num_cold_sbs = 0;
  tlh->num_liquid_sbs = 0;
  tlh->hold_thread = pthread_self();
}

void *thread_local_heap_allocate(thread_local_heap *tlh, int size_class) {
  void *ret = NULL;
  super_meta_block *sb = NULL;

  // get a available superblock from the thread local heap.
  sb = thread_local_heap_get_sb(tlh, size_class);

  if (sb == NULL)
    // ERROR: OOM
    ;

  // get a available data block form the superblock.
  ret = super_block_allocate_data_block(tlh, sb);

  return ret;
}

static inline super_meta_block *thread_local_heap_get_sb(thread_local_heap *tlh,
                                                         int size_class) {
  super_meta_block *sb = NULL;

  // try to fetch a hot superblock within the tlh.
  sb = thread_local_heap_fetch_cached_hot_sb(tlh, size_class);

  // try to fetch a cold superblock within the tlh.
  if (sb == NULL)
    sb = thread_local_heap_fetch_cached_cold_sb(tlh, size_class);

  // try to fetch a cool superblock within the tlh,
  // and flush all the remote free memory into the local free list
  if (sb == NULL)
    sb = thread_local_heap_fetch_and_flush_cool_sb(tlh, size_class);

  // try to fetch a frozen superblock within the tlh,
  // If failed, request raw memory from the global pool.
  if (sb == NULL)
    sb = thread_local_heap_fetch_cached_or_new_frozen_sb(tlh, size_class);

  return sb;
}

static inline super_meta_block *
thread_local_heap_fetch_cached_hot_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *hot_sb = NULL;
  hot_sb = seq_dequeue(&tlh->hot_sbs[size_class]);
  return hot_sb;
}

static inline super_meta_block *
thread_local_heap_fetch_cached_cold_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *cold_sb = NULL;
  cold_sb = seq_dequeue(&tlh->cold_sbs[size_class]);
  return cold_sb;
}

static inline super_meta_block *
thread_local_heap_fetch_and_flush_cool_sb(thread_local_heap *tlh,
                                          int size_class) {
  // Try to fetch a cool superblock
  super_meta_block *cool_sb = sc_dequeue(&tlh->cool_sbs[size_class], 0);

  if (cool_sb != NULL) {
    // Get the head pointer of the remote free list
    // and the number of remote freed datablocks
    uint32_t num_remote_freed_blocks;
    void *remote_freed_list;
    remote_freed_list = counted_chain_dequeue(&cool_sb->remote_freed_blocks,
                                              &num_remote_freed_blocks);

    // The number of remote freed blocks on the cool superblock
    // is possible to be equal to zero in the following scenario:
    //
    // Current thread(Thread A) fetch a cool superblock within its tlh
    // and then another thread(Thread B) frees a datablock remotely.
    // Thread A flush the memory freed remotely both by Thread B and Thread ?
    // into local,
    // which makes the cool superblock maintain no memory.
    // Thread that fetched the cool superblock will find the
    // num_remote_freed_blocks to be zero.
    if (num_remote_freed_blocks != 0) {
      // We will fetch memory from remote freed datablocks
      // only if there is no local freed memory nor clean memory within the tlh.
      // Therefore the field local_free_blocks of cool_sb should be NULL
      // and that's why we just update it's head pointer.
      cool_sb->local_free_blocks = remote_freed_list;
      cool_sb->num_allocated_and_remote_blocks -= num_remote_freed_blocks;
      return cool_sb;
    }
  }
  return NULL;
}

static inline super_meta_block *
thread_local_heap_fetch_cached_or_new_frozen_sb(thread_local_heap *tlh,
                                                int size_class) {
  super_meta_block *frozen_sb = NULL;
  frozen_sb = seq_dequeue(&tlh->frozen_sbs[size_class]);

  if (frozen_sb == NULL)
    global_pool_fetch_cached_or_make_new_sb(tlh, size_class);

  return frozen_sb;
}

void thread_local_heap_deallocate(thread_local_heap *tlh, void *data_block) {
  super_meta_block *smb;
  void *correlated_shadow_block;

  // target the correlated shadow block and the smb managing it
  correlated_shadow_block = super_block_data_to_shadow(data_block);
  smb = rev_addr_hashset_db_to_smb(data_block);

  // recycle the correlated shadow block
  if (tlh->hold_thread == pthread_self()) {
    // local free routine
    seq_enqueue(&smb->local_free_blocks, correlated_shadow_block);
    super_block_check_life_cycle_and_update_counter_when_free(tlh, smb);
  } else {
    // remote free routine
    void *old_head = NULL;
    old_head =
        counted_enqueue(&smb->remote_freed_blocks, correlated_shadow_block);

    // if this is the first time the superblock free a datablock remotely,
    // mark it cool.
    if (old_head == NULL)
      sc_enqueue(&smb->owner_tlh->cool_sbs[smb->size_class], &smb->prev_cool_sb,
                 0);
  }
}
#include "includes/heaps.h"
#include "includes/acc_unit.inl.h"
#include "includes/assert.h"
#include "includes/cds.inl.h"
#include "includes/sds.inl.h"
#include "includes/size_class.inl.h"
#include "includes/system.inl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

static inline super_meta_block *super_block_assemble(thread_local_heap *tlh,
                                                     super_meta_block *raw_smb,
                                                     super_data_block *raw_sdb,
                                                     int size_class);
static inline void super_block_init_cached(thread_local_heap *tlh,
                                           super_meta_block *smb,
                                           int size_class);
static inline void *super_block_data_to_shadow(super_meta_block *smb,
                                               void *data_block);
static inline void *super_block_shadow_to_data(super_meta_block *smb,
                                               shadow_block *shadowb);
static inline void *super_block_allocate_data_block(thread_local_heap *tlh,
                                                    super_meta_block *smb);
static inline bool super_block_satisfy_frozen(thread_local_heap *tlh,
                                              super_meta_block *sb);
static inline bool super_block_satisfy_cold(super_meta_block *sb);
static inline bool super_block_satisfy_hot(super_meta_block *sb);
static inline bool super_block_satisfy_warm(super_meta_block *sb);
static inline void super_block_check_life_cycle_and_update_counter_when_malloc(
    thread_local_heap *tlh, super_meta_block *sb);
static inline void super_block_check_life_cycle_and_update_counter_when_free(
    thread_local_heap *tlh, super_meta_block *sb);
static inline void super_block_convert_life_cycle_when_malloc(
    thread_local_heap *tlh, super_meta_block *sb, life_cycle target_life_cycle);
static inline void super_block_convert_life_cycle_when_free(
    thread_local_heap *tlh, super_meta_block *sb, life_cycle target_life_cycle);
void super_block_trace(seq_queue_head queue);

/* global pool */
global_pool GLOBAL_POOL;
static inline size_t global_meta_pool_index_sdb(void *sdb);
static inline void global_meta_pool_init(void);
static inline void global_data_pool_init(void);
static inline super_meta_block *global_pool_make_raw_smb(int size_class);
static inline void **global_pool_make_dynamic_array(size_t array_size);
static inline thread_local_heap *global_pool_make_raw_heap(void);
static inline super_data_block *global_pool_make_raw_sdb(void);
static inline super_meta_block **global_pool_make_raw_rah(void);
static inline thread_local_heap *global_pool_make_new_heap(void);
static inline super_meta_block *global_pool_make_new_sb(thread_local_heap *tlh,
                                                        int size_class);
static inline super_meta_block *
global_pool_fetch_cached_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *
global_pool_fetch_cached_or_make_new_sb(thread_local_heap *tlh, int size_class);
static inline thread_local_heap *global_pool_fetch_cached_heap(void);

/* thread local heap */
static inline void
thread_local_heap_init_excpt_owner_thread(thread_local_heap *tlh);
static inline super_meta_block *thread_local_heap_get_sb(thread_local_heap *tlh,
                                                         int size_class);
static inline super_meta_block *
thread_local_heap_load_cached_hot_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *
thread_local_heap_load_cached_cold_sb(thread_local_heap *tlh, int size_class);
static inline super_meta_block *
thread_local_heap_fetch_and_flush_cool_sb(thread_local_heap *tlh,
                                          int size_class);
static inline super_meta_block *
thread_local_heap_load_cached_or_request_new_frozen_sb(thread_local_heap *tlh,
                                                       int size_class);

/*******************************************
* @ Definition
* @ About the life cycle
*******************************************/
char *life_cycle_enum_to_name(life_cycle life_cycle_) {
  char *str_life_cycle = NULL;
  ;
  switch (life_cycle_) {
  case hot:
    str_life_cycle = "hot";
    break;
  case warm:
    str_life_cycle = "warm";
    break;
  case cold:
    str_life_cycle = "cold";
    break;
  case frozen:
    str_life_cycle = "frozen";
    break;
  }
  return str_life_cycle;
}

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
  return SIZE_SDB / size_class_block_size(size_class) * sizeof(void *) +
         sizeof(super_meta_block);
}

static inline size_t super_meta_block_total_size(int size_class) {
  return sizeof(super_meta_block) + size_class_num_blocks(size_class);
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

static inline void super_block_init_cached(thread_local_heap *tlh,
                                           super_meta_block *smb,
                                           int size_class) {
  // init the cached super meta block
  memset(smb, 0, sizeof(smb->list_elem));
  smb->owner_tlh = tlh;

  tlh->num_frozen_sbs[size_class]++;
  seq_enqueue(&tlh->frozen_sbs[size_class], smb);
}

static inline super_meta_block *super_block_assemble(thread_local_heap *tlh,
                                                     super_meta_block *raw_smb,
                                                     super_data_block *raw_sdb,
                                                     int size_class) {
  // Establish the link between the raw_smb and the raw_sdb
  // by recording the address of the super data block on the super meta block
  // and updating the reverse addressing hashset.
  raw_smb->own_sdb = raw_sdb;
  rev_addr_hashset_update(raw_smb, raw_sdb);

  super_block_init_cached(tlh, raw_smb, size_class);

  sc_queue_init(&raw_smb->prev_cool_sb);
  raw_smb->num_allocated_and_remote_blocks = 0;
  seq_queue_init(&raw_smb->local_free_blocks);
  sc_queue_init(&raw_smb->remote_freed_blocks);
  raw_smb->clean_zone = (void *)raw_smb + sizeof(super_meta_block);
  raw_smb->size_class = size_class;
  raw_smb->cur_cycle = frozen;

  return raw_smb;
}

int super_block_data_to_size_class(void *addr) {
  super_meta_block *smb = rev_addr_hashset_db_to_smb(addr);
  int size_class = smb->size_class;
  return size_class;
}

static inline void *super_block_data_to_shadow(super_meta_block *smb,
                                               void *data_block) {
  size_t offset_data = (size_t)data_block - (size_t)smb->own_sdb;
  int size_class = smb->size_class;
  int offset_meta = acc_unit_offdata_to_offshadow(offset_data, size_class);

  // return the address of correlated shadow block
  return (void *)smb + sizeof(super_meta_block) + offset_meta;
  ;
}

static inline void *super_block_shadow_to_data(super_meta_block *smb,
                                               shadow_block *shadowb) {
  size_t offset_meta = (size_t)shadowb - (size_t)smb - sizeof(super_meta_block);
  int size_class = smb->size_class;
  size_t offset_data = offset_meta * size_class_ratio_data_shadow(size_class);

  // return the address of correlated data block
  return smb->own_sdb + offset_data;
}

static inline void *super_block_allocate_data_block(thread_local_heap *tlh,
                                                    super_meta_block *smb) {
  shadow_block *shadowb = NULL;

  // try to fetch a freed shadow block from the superblock
  shadowb = super_meta_block_allocate_freed_shadow(smb);

  // try to fetch a clean shadow block from the superblock
  if (shadowb == NULL)
    shadowb = super_meta_block_allocate_clean_shadow(smb);

  // check weather need to convert current superblock's life cycle
  super_block_check_life_cycle_and_update_counter_when_malloc(tlh, smb);

  return super_block_shadow_to_data(smb, shadowb);
}

static inline bool super_block_satisfy_frozen(thread_local_heap *tlh,
                                              super_meta_block *sb) {
  // Since the number of liquid superblocks held by the thread local heap is
  // too small,
  // we will not try to freeze any of these liquid superblocks.
  int size_class = sb->size_class;
  if (tlh->num_liquid_sbs[size_class] == 0)
    return false;

  // Else, if the superblock satisfies the characteristics of cold
  // and the ratio of cold superblocks to liquid superblocks is larger than
  // MAX_RATIO_COLD_LIQUID,
  // we will freeze the superblock.
  int num_cold = tlh->num_cold_sbs[size_class];
  int num_liquid = tlh->num_liquid_sbs[size_class];
  int ratio_cold_liquid = num_cold * 100 / num_liquid;
  return ratio_cold_liquid > getRatioColdLiquid();
}

static inline bool
super_block_satisfy_return_to_global_pool(thread_local_heap *tlh,
                                          super_meta_block *sb) {
  // we will return the superblock to the global pool
  // if the thread local heap holds to many frozen blocks
  // for load balancing
  int size_class = sb->size_class;
  int num_liquid = tlh->num_liquid_sbs[size_class];
  int num_frozen = tlh->num_frozen_sbs[size_class];

  int ratio_frozen_liquid = num_frozen * 100 / num_liquid;
  return ratio_frozen_liquid > getRatioFrozenLiquid();
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
         size_class_num_blocks(sb->size_class);
}

static inline bool super_block_satisfy_warm(super_meta_block *sb) {
  // We think that a superblock that could not allocate datablock is a warm
  // superblock,
  // even though it may maintain some remote freed datatblocks inside.
  return sb->num_allocated_and_remote_blocks >=
         size_class_num_blocks(sb->size_class);
}

static inline void super_block_check_life_cycle_and_update_counter_when_malloc(
    thread_local_heap *tlh, super_meta_block *sb) {
  int size_class = sb->size_class;
  sb->num_allocated_and_remote_blocks++;
  switch (sb->cur_cycle) {
  case frozen:
    // frozen -> hot ?
    if (super_block_satisfy_hot(sb)) {
      super_block_convert_life_cycle_when_malloc(tlh, sb, hot);
      tlh->num_liquid_sbs[size_class]++;
      tlh->num_frozen_sbs[size_class]--;

      return;
    }
    // frozen -> warm ?
    super_block_convert_life_cycle_when_malloc(tlh, sb, warm);
    tlh->num_liquid_sbs[size_class]++;
    tlh->num_frozen_sbs[size_class]--;
    break;
  case cold:
    // cold -> hot ?
    if (super_block_satisfy_hot(sb)) {
      super_block_convert_life_cycle_when_malloc(tlh, sb, hot);
      tlh->num_cold_sbs[size_class]--;

      return;
    }
    // cold -> warm ?
    super_block_convert_life_cycle_when_malloc(tlh, sb, warm);
    tlh->num_cold_sbs[size_class]--;
    break;
  case hot:
    // hot -> warm
    if (super_block_satisfy_warm(sb))
      super_block_convert_life_cycle_when_malloc(tlh, sb, warm);
    break;
  case warm:
    // warm -> hot ?
    if (super_block_satisfy_hot(sb))
      super_block_convert_life_cycle_when_malloc(tlh, sb, hot);
    break;
  }
}

static inline void
super_block_convert_life_cycle_when_malloc(thread_local_heap *tlh,
                                           super_meta_block *sb,
                                           life_cycle target_life_cycle) {
  // dequeue
  switch (sb->cur_cycle) {
  case hot:
    double_list_remove(sb, &tlh->hot_sbs[sb->size_class]);
    break;
  case warm:
    break;
  case cold:
    seq_dequeue(&tlh->cold_sbs[sb->size_class]);
    break;
  case frozen:
    seq_dequeue(&tlh->frozen_sbs[sb->size_class]);
    break;
  }

  // enqueue
  switch (target_life_cycle) {
  case hot:
    double_list_insert_front(sb, &tlh->hot_sbs[sb->size_class]);
    sb->cur_cycle = hot;
    break;
  case warm:
#ifdef TRACE_WARM_BLOCKS
    double_list_insert_front(sb, &tlh->warm_sbs[sb->size_class]);
#endif
    sb->cur_cycle = warm;
    break;
  default:;
  }
}

static inline void super_block_check_life_cycle_and_update_counter_when_free(
    thread_local_heap *tlh, super_meta_block *sb) {
  int size_class = sb->size_class;
  sb->num_allocated_and_remote_blocks--;
  switch (sb->cur_cycle) {
  case hot:
    // hot -> cold ?
    if (super_block_satisfy_cold(sb)) {
      // hot -> frozen ?
      if (super_block_satisfy_frozen(tlh, sb)) {
        super_block_convert_life_cycle_when_free(tlh, sb, frozen);
        tlh->num_liquid_sbs[size_class]--;
        tlh->num_frozen_sbs[size_class]++;
        return;
      }

      super_block_convert_life_cycle_when_free(tlh, sb, cold);
      tlh->num_cold_sbs[size_class]++;
    }
    break;
  case warm:
    // warm -> cold ?
    if (super_block_satisfy_cold(sb)) {
      // warm -> frozen ?
      if (super_block_satisfy_frozen(tlh, sb)) {
        super_block_convert_life_cycle_when_free(tlh, sb, frozen);
        tlh->num_liquid_sbs[size_class]--;
        tlh->num_frozen_sbs[size_class]++;

        return;
      }

      super_block_convert_life_cycle_when_free(tlh, sb, cold);
      tlh->num_cold_sbs[size_class]++;
      return;
    }

    // warm -> hot ?
    super_block_convert_life_cycle_when_free(tlh, sb, hot);
    break;
  default:;
  }
}

static inline void
super_block_convert_life_cycle_when_free(thread_local_heap *tlh,
                                         super_meta_block *sb,
                                         life_cycle target_life_cycle) {
  // dequeue
  if (sb->cur_cycle == hot)
    double_list_remove(sb, &tlh->hot_sbs[sb->size_class]);

  // enqueue
  switch (target_life_cycle) {
  case hot:
    double_list_insert_front(sb, &tlh->hot_sbs[sb->size_class]);
    sb->cur_cycle = hot;
    break;
  case cold:
    seq_enqueue(&tlh->cold_sbs[sb->size_class], sb);
    sb->cur_cycle = cold;
    break;
  case frozen:
    freeze_vm(sb->own_sdb, SIZE_SDB);

    if (super_block_satisfy_return_to_global_pool(tlh, sb)) {
      mc_queue_head *reusable_list;
      int sc = sb->size_class;
      reusable_list = &GLOBAL_POOL.meta_pool.reusable_sbs[sc][get_core_id()];
      tlh->num_frozen_sbs[sb->size_class]--;
      mc_enqueue(reusable_list, sb, 0);
    } else {
      seq_enqueue(&tlh->frozen_sbs[sb->size_class], sb);
      sb->cur_cycle = frozen;
    }
    break;
  default:;
  }
}

void super_block_trace(void *elem) {
  super_meta_block *sb = (super_meta_block *)elem;
  printf("\t\tsuperblock( SN: %lu ):\n",
         global_meta_pool_index_sdb(sb->own_sdb));
  printf("\t\t\tsize calss: %d\n", sb->size_class);
  printf("\t\t\tblock size: %lu\n", size_class_block_size(sb->size_class));
  printf("\t\t\tlife cycle: %s\n", life_cycle_enum_to_name(sb->cur_cycle));
  printf("\t\t\tnum alloced / remotely freed: %d\n",
         sb->num_allocated_and_remote_blocks);
  printf("\t\t\tnum locally freed: %d\n",
         seq_visit(sb->local_free_blocks, NULL));
  printf("\t\t\tnum remotely freed: %d\n",
         counted_num_elems(&sb->remote_freed_blocks));
  printf("\t\t\tnum total: %lu\n", size_class_num_blocks(sb->size_class));
  printf("\t\t\tstart addr: %p\n", sb);
  printf("\t\t\tclean addr: %p\n", sb->clean_zone);
}
/*******************************************
 * @ Definition
 * @ Object :global pool
 *******************************************/

void global_pool_init(void) {
  global_meta_pool_init();
  global_data_pool_init();
}

static inline size_t global_meta_pool_index_sdb(void *sdb) {
  return (sdb - GLOBAL_POOL.data_pool.pool_start) / SIZE_SDB;
}

static inline void global_meta_pool_init(void) {
  int i, j, num_cores = get_num_cores();
  GLOBAL_POOL.meta_pool.pool_start =
      request_vm(STARTA_ADDR_META_POOL, LENGTH_META_POOL);
  GLOBAL_POOL.meta_pool.pool_clean = GLOBAL_POOL.meta_pool.pool_start;
  GLOBAL_POOL.meta_pool.pool_end =
      GLOBAL_POOL.meta_pool.pool_start + LENGTH_META_POOL;
  GLOBAL_POOL.meta_pool.reusable_heaps =
      (mc_queue_head *)global_pool_make_dynamic_array(sizeof(mc_queue_head) *
                                                      num_cores);
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    GLOBAL_POOL.meta_pool.reusable_sbs[i] =
        (mc_queue_head *)global_pool_make_dynamic_array(sizeof(mc_queue_head) *
                                                        num_cores);

  for (i = 0; i < num_cores; ++i)
    mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_heaps[i]);
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    for (j = 0; j < num_cores; ++j)
      mc_queue_init(&GLOBAL_POOL.meta_pool.reusable_sbs[i][j]);
}

static inline void global_data_pool_init(void) {
  GLOBAL_POOL.data_pool.pool_start =
      request_vm(STARTA_ADDR_DATA_POOL, LENGTH_DATA_POOL);
  GLOBAL_POOL.data_pool.pool_clean = GLOBAL_POOL.data_pool.pool_start;
  GLOBAL_POOL.data_pool.pool_end =
      GLOBAL_POOL.data_pool.pool_start + LENGTH_DATA_POOL;
}

int global_pool_check_data_addr(void *addr) {
  // check if the given address is valid,
  // i.e. either within the global meta pool or the global data pool
  if (addr >= GLOBAL_POOL.data_pool.pool_start &&
      addr <= GLOBAL_POOL.data_pool.pool_end)
    return 1;
  else
    return 0;
}

static inline super_meta_block *global_pool_make_raw_smb(int size_class) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap
  size_t smb_size = super_meta_block_size_class_to_size(size_class);

  // atomicly fetch and increase the start address of the global meta pool
  return (super_meta_block *)__sync_fetch_and_add(
      &GLOBAL_POOL.meta_pool.pool_clean, smb_size);
}

static inline void **global_pool_make_dynamic_array(size_t array_size) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap

  // atomicly fetch and increase the start address of the global meta pool
  return (void **)__sync_fetch_and_add(&GLOBAL_POOL.meta_pool.pool_clean,
                                       array_size);
}

static inline thread_local_heap *global_pool_make_raw_heap(void) {
  // get the address of the global meta pool's start address,
  // and compute the size of a thread local heap
  size_t heap_size = sizeof(thread_local_heap);

  // atomicly fetch and increase the start address of the global meta pool
  return (thread_local_heap *)__sync_fetch_and_add(
      &GLOBAL_POOL.meta_pool.pool_clean, heap_size);
}

static inline super_data_block *global_pool_make_raw_sdb(void) {
  // get the address of the global data pool's start address

  // atomicly fetch and increase the start address of the global data pool
  return (super_data_block *)__sync_fetch_and_add(
      &GLOBAL_POOL.data_pool.pool_clean, SIZE_SDB);
}

static inline super_meta_block **global_pool_make_raw_rah(void) {
  // get the address of the global meta pool's start address

  // atomicly fetch and increase the start address of the global meta pool
  return (super_meta_block **)__sync_fetch_and_add(
      &GLOBAL_POOL.meta_pool.pool_clean, LENGTH_REV_ADDR_HASHSET);
}

static inline thread_local_heap *global_pool_make_new_heap(void) {
  thread_local_heap *tlh = NULL;
  // try to make a raw heap
  tlh = global_pool_make_raw_heap();

  // init the heap
  if (unlikely(tlh == NULL))
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "metadata's vm space has ran out.");

  thread_local_heap_init_excpt_owner_thread(tlh);

  return tlh;
}

static inline super_meta_block *global_pool_make_new_sb(thread_local_heap *tlh,
                                                        int size_class) {
  super_meta_block *new_sb = NULL;

  // make a raw super meta block on the global meta pool
  super_meta_block *raw_smb = NULL;
  raw_smb = global_pool_make_raw_smb(size_class);

  if (unlikely(raw_smb == NULL))
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "metadata's vm space has ran out.");

  // make a raw super data block on the global data pool
  super_data_block *raw_sdb = NULL;
  raw_sdb = global_pool_make_raw_sdb();

  if (unlikely(raw_sdb == NULL))
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "data's vm space has ran out.");

  // assemble the raw super meta block and the raw super data block
  // into a new superblock
  new_sb = super_block_assemble(tlh, raw_smb, raw_sdb, size_class);

  return new_sb;
}

static inline super_meta_block *
global_pool_fetch_cached_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *cached_sb = NULL;

  // fetch a sb cached on the global pool(same core)
  cached_sb = (super_meta_block *)mc_dequeue(
      &GLOBAL_POOL.meta_pool.reusable_sbs[size_class][get_core_id()], 0);

  // fetch a sb(all cores)
  /* int i;
  if (cached_sb == NULL)
    for (i = 0; i < get_num_cores(); ++i)
      if ((cached_sb = (super_meta_block *)mc_dequeue(
               &GLOBAL_POOL.meta_pool.reusable_sbs[size_class][i], 0)) != NULL)
        break; */

  // init the cached sb
  if (cached_sb != NULL)
    super_block_init_cached(tlh, cached_sb, size_class);

  return cached_sb;
}

static inline super_meta_block *
global_pool_fetch_cached_or_make_new_sb(thread_local_heap *tlh,
                                        int size_class) {
  super_meta_block *sb = NULL;

  // try to fetch a superblock cached on the global pool.
  sb = global_pool_fetch_cached_sb(tlh, size_class);

  if (sb == NULL)
    sb = global_pool_make_new_sb(tlh, size_class);

  return sb;
}

static inline thread_local_heap *global_pool_fetch_cached_heap(void) {
  thread_local_heap *cached_heap;
  mc_queue_head *cached_heaps_head;
  int core_id;

  // clues to cached heaps
  core_id = get_core_id();
  cached_heaps_head = &GLOBAL_POOL.meta_pool.reusable_heaps[core_id];

  // try to fetch a cached heap
  cached_heap = mc_dequeue(cached_heaps_head, 0);

  return cached_heap;
}

thread_local_heap *global_pool_allocate_heap(void) {
  thread_local_heap *tlh = NULL;

  // try to fetch
  tlh = global_pool_fetch_cached_heap();

  if (tlh == NULL)
    tlh = global_pool_make_new_heap();

  tlh->holder_thread = pthread_self();

  return tlh;
}

void global_pool_deallocate_heap(thread_local_heap *tlh) {
  tlh->holder_thread = 0;
  mc_enqueue(&GLOBAL_POOL.meta_pool.reusable_heaps[get_core_id()],
             &tlh->freed_list, 0);
}

/*******************************************
 * @ Definition
 * @ Object :thread local heap(tlh)
 *******************************************/

static inline void
thread_local_heap_init_excpt_owner_thread(thread_local_heap *tlh) {
  memset(tlh, 0, sizeof(thread_local_heap));
}

void *thread_local_heap_allocate(thread_local_heap *tlh, int size_class) {
  super_meta_block *sb = NULL;

  // get a available superblock from the thread local heap.
  sb = thread_local_heap_get_sb(tlh, size_class);

  // get a available data block form the superblock.
  void *ret = super_block_allocate_data_block(tlh, sb);
  return ret;
}

static inline super_meta_block *thread_local_heap_get_sb(thread_local_heap *tlh,
                                                         int size_class) {
  super_meta_block *sb = NULL;

  // try to fetch a hot superblock within the tlh.
  sb = thread_local_heap_load_cached_hot_sb(tlh, size_class);

  // try to fetch a cold superblock within the tlh.
  if (unlikely(sb == NULL)) {
    sb = thread_local_heap_load_cached_cold_sb(tlh, size_class);

    // try to fetch a cool superblock within the tlh,
    // and flush all the remote free memory into the local free list
    if (sb == NULL) {
      sb = thread_local_heap_fetch_and_flush_cool_sb(tlh, size_class);

      // try to fetch a frozen superblock within the tlh,
      // If failed, request raw memory from the global pool.
      if (likely(sb == NULL))
        sb = thread_local_heap_load_cached_or_request_new_frozen_sb(tlh,
                                                                    size_class);
    }
  }
  return sb;
}

static inline super_meta_block *
thread_local_heap_load_cached_hot_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *hot_sb = NULL;
  hot_sb = (super_meta_block *)tlh->hot_sbs[size_class].head;
  return hot_sb;
}

static inline super_meta_block *
thread_local_heap_load_cached_cold_sb(thread_local_heap *tlh, int size_class) {
  super_meta_block *cold_sb = NULL;
  cold_sb = tlh->cold_sbs[size_class];
  return cold_sb;
}

static inline super_meta_block *
thread_local_heap_fetch_and_flush_cool_sb(thread_local_heap *tlh,
                                          int size_class) {
  // Try to fetch a cool superblock
  super_meta_block *cool_sb =
      sc_dequeue(&tlh->cool_sbs[size_class],
                 (size_t) & (((super_meta_block *)0x0)->prev_cool_sb));

  if (unlikely(cool_sb != NULL)) {
    // Get the head pointer of the remote free list
    // and the number of remote freed datablocks
    uint32_t num_remote_freed_blocks = 0;
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
thread_local_heap_load_cached_or_request_new_frozen_sb(thread_local_heap *tlh,
                                                       int size_class) {
  super_meta_block *frozen_sb = NULL;
  frozen_sb = tlh->frozen_sbs[size_class];

  if (frozen_sb == NULL)
    frozen_sb = global_pool_fetch_cached_or_make_new_sb(tlh, size_class);

  return frozen_sb;
}

void thread_local_heap_deallocate(thread_local_heap *tlh, void *data_block) {
  super_meta_block *smb;
  void *correlated_shadow_block;

  // target the correlated shadow block and the smb managing it
  smb = rev_addr_hashset_db_to_smb(data_block);
  correlated_shadow_block = super_block_data_to_shadow(smb, data_block);

  // recycle the correlated shadow block
  if (likely(tlh && tlh->holder_thread == pthread_self())) {
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
      sc_enqueue(&smb->owner_tlh->cool_sbs[smb->size_class], (void *)smb,
                 (size_t) & (((super_meta_block *)0x0)->prev_cool_sb));
  }
}

void thread_local_heap_trace(thread_local_heap **pTlh) {
  int num_frozens = 0, num_colds = 0, num_hot = 0, num_warms = 0;
  thread_local_heap *tlh = *pTlh;
  printf("Address of the thread local heap: %p\n", tlh);
  int i;
  printf("\tFrozen superblocks:\n");
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    num_frozens += seq_visit(tlh->frozen_sbs[i], &super_block_trace);
  printf("\t\t->Sum up to: %d\n\n", num_frozens);

  printf("\tcold superblocks:\n");
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    num_colds += seq_visit(tlh->cold_sbs[i], &super_block_trace);
  printf("\t\t->Sum up to: %d\n\n", num_colds);

  printf("\tHot superblocks:\n");
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    num_hot += double_list_visit(&tlh->hot_sbs[i], &super_block_trace);
  printf("\t\t->Sum up to: %d\n\n", num_hot);
#ifdef TRACE_WARM_BLOCKS
  printf("\tWarm superblocks:\n");
  for (i = 0; i < NUM_SIZE_CLASSES; ++i)
    num_warms += double_list_visit(&tlh->warm_sbs[i], &super_block_trace);
  printf("\t\t-> Sum up to: %d\n\n", num_warms);
#endif
  printf("\t-> Sum up to: %d\n\n",
         num_frozens + num_colds + num_hot + num_warms);
}

/*******************************************
* @ Definition
* @ About the life cycle
*******************************************/

size_t large_block_aligned_size(size_t ori_size) {
  return PAGE_ROUNDUP(ori_size + sizeof(large_block_header));
}

large_block_header *large_block_init(void *raw_ptr, size_t size) {
  large_block_header *head = (large_block_header *)(raw_ptr);

  head->size = size;
  head->mem = head + sizeof(large_block_header);

  return head;
}

large_block_header *large_block_get_header(void *usr_ptr) {
  large_block_header *header;
  header = (large_block_header *)(usr_ptr - sizeof(large_block_header));
  return header;
}

void *large_block_allocate(size_t size) {
  size_t alloc_size = large_block_aligned_size(size);

  void *raw_vm = request_vm(NULL, alloc_size);

  return large_block_init(raw_vm, alloc_size)->mem;
}

void large_block_deallocate(void *usr_ptr) {
  size_t size = large_block_get_header(usr_ptr)->size;
  release_vm(usr_ptr, size);
}

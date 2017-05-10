#ifndef CMALLOC_SIZE_CLASS_INL_H
#define CMALLOC_SIZE_CLASS_INL_H

#include "globals.h"
#include "math.inl.h"

extern int BLOCK_SIZE[NUM_SIZE_CLASSES];
extern int NUM_BLOCKS[NUM_SIZE_CLASSES];
extern int RATIO_DATA_SHADOW[NUM_SIZE_CLASSES];

#define MASK_TINY_STEP (STEP_TINY_CLASSES - 1)

/* base */

static inline size_t size_class_num_tiny(void) {
  return MAX_TINY_CLASSES / STEP_TINY_CLASSES;
}

/* size */

static inline int size_in_tiny(size_t size) { return size <= MAX_TINY_CLASSES; }

static inline int size_in_medium(size_t size) {
  return size <= MAX_MEDIUM_CLASSES && size > MAX_TINY_CLASSES;
}

static inline int size_to_size_class(size_t size) {
  if (size_in_tiny(size))
    return (size + MASK_TINY_STEP) / STEP_TINY_CLASSES - 1;

  return log2_64(size - 1) - log2_64(MAX_TINY_CLASSES) + size_class_num_tiny();
}

static inline size_t size_to_block_size(size_t size) {
  if (size_in_tiny(size))
    return (size + MASK_TINY_STEP) & ~MASK_TINY_STEP;

  return 1UL << (log2_64(size - 1) + 1);
}

#undef MASK_TINY_STEP

/* size class */

static inline int size_class_in_tiny_or_medium(int size_class) {
  return size_class < NUM_SIZE_CLASSES;
}

static inline size_t size_class_num_blocks(int size_class) {
  return NUM_BLOCKS[size_class];
}

static inline size_t size_class_block_size(int size_class) {
  return BLOCK_SIZE[size_class];
}

static inline int size_class_ratio_data_shadow(int size_class) {
  return RATIO_DATA_SHADOW[size_class];
}

void size_class_init(void);

#endif // end of CMALLOC_SIZE_CLASS_INL_H
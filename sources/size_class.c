#include "includes/globals.h"
#include "includes/size_class.inl.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "includes/heaps.h"

int BLOCK_SIZE[NUM_SIZE_CLASSES];
int NUM_BLOCKS[NUM_SIZE_CLASSES];
int RATIO_DATA_SHADOW[NUM_SIZE_CLASSES];

void size_class_init(void) {
  int i;
  for (i = 0; i < NUM_SIZE_CLASSES; ++i) {
    // size class to block size
    BLOCK_SIZE[i] =
        i < size_class_num_tiny()
            ? (i + 1) * 16
            : MAX_TINY_CLASSES * power2(i + 1 - size_class_num_tiny());

    // size class to num blocks
    NUM_BLOCKS[i] = SIZE_SDB / BLOCK_SIZE[i];

    // the size of data block is N times bigger than that of shadow's
    RATIO_DATA_SHADOW[i] = BLOCK_SIZE[i] / sizeof(shadow_block);
  }
}

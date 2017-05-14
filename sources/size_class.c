#include "includes/globals.h"
#include "includes/size_class.inl.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "includes/heaps.h"

int BLOCK_SIZE[NUM_SIZE_CLASSES] = {
    16,  32,  48,  64,  80,  96,   112,  128,  144,  160,   176,   192,
    208, 224, 240, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
};

int NUM_BLOCKS[NUM_SIZE_CLASSES] = {
    4096, 2048, 1365, 1024, 819, 682, 585, 512, 455, 409, 372, 341,
    315,  292,  273,  256,  128, 64,  32,  16,  8,   4,   2,   1,
};

int RATIO_DATA_SHADOW[NUM_SIZE_CLASSES] = {
    8,   16,  24,  32,  40,  48,  56,   64,   72,   80,   88,    96,
    104, 112, 120, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
};

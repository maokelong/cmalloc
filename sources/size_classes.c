#include "includes/size_classes.h"
#include "includes/globals.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/*******************************************
 * 全局变量定义
 *******************************************/
// 定义 Tiny Class 的最大值及递增步长
const size_t MAX_TINY_CLASSES = 256;
const size_t STEP_TINY_CLASSES = 16;

// 定义 Medium Class 的最大值及递增步长
const size_t MAX_MEDIUM_CLASSES = 65536;

static int BLOCK_SIZE[NUM_SIZE_CLASSES];
static int NUM_BLOCKS[NUM_SIZE_CLASSES];

// 是否处于 tiny class
static inline bool InTinyClasses(size_t size) {
  return size <= MAX_TINY_CLASSES;
}

// 是否处于 medium class
static inline bool InMediumClasses(size_t size) {
  return size <= MAX_MEDIUM_CLASSES && size > MAX_TINY_CLASSES;
}

// tiny class 的数量
static inline size_t NumTinyClasses(void) {
  return MAX_TINY_CLASSES / STEP_TINY_CLASSES;
}

// tiny class 步长的掩码
static inline size_t MaskTinyStep(void) { return STEP_TINY_CLASSES - 1; }

static inline size_t Power(size_t x, size_t y) {
  size_t sum = x;
  if (y == 0)
    return 1;

  while (--y != 0)
    sum *= x;
  return sum;
}

// 计算以 2 为底的对数(64位)
// code from:
// http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers）

// 定义用于计算log2结果的矩阵
static const int tab64[64] = {
    63, 0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,
    61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,
    62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5};

static int log2_64(uint64_t value) {
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  return tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

void SizeClassInit(void) {
  int i;
  for (i = 0; i < NUM_SIZE_CLASSES; ++i) {
    // size class to block size
    if (i < NumTinyClasses())
      BLOCK_SIZE[i] = (i + 1) * 16;
    else
      BLOCK_SIZE[i] = MAX_TINY_CLASSES * Power(2, i + 1 - NumTinyClasses());

    // size class to num blocks
    NUM_BLOCKS[i] = SIZE_SDB / BLOCK_SIZE[i];
  }
}

bool InSmallSize(int size_class) { return size_class < NUM_SIZE_CLASSES; }

// 将 size 转换为 size class
int SizeToSizeClass(size_t size) {
  // 处理小型尺寸(1-256)
  if (InTinyClasses(size))
    return (size + MaskTinyStep()) / STEP_TINY_CLASSES - 1;

  // 处理中型尺寸(257-)
  return log2_64(size - 1) - log2_64(MAX_TINY_CLASSES) + NumTinyClasses();
}

size_t SizeClassToBlockSize(int size_class) { return BLOCK_SIZE[size_class]; }

// 将 size 转换为 block size
size_t SizeToBLockSize(size_t size) {
  // 处理小型尺寸(1-256)
  if (InTinyClasses(size))
    return (size + MaskTinyStep()) & ~MaskTinyStep();

  // 处理中型尺寸(257-65536)
  if (InMediumClasses(size))
    return 1UL << (log2_64(size - 1) + 1);

  // ASSERT: 不可能执行的路径
  return 0;
}

size_t SizeClassToNumDataBlocks(int size_class) {
  return NUM_BLOCKS[size_class];
}
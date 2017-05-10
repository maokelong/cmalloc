#ifndef CMALLOC_MATH_H
#define CMALLOC_MATH_H

#include "globals.h"
#include <stdint.h>
#include <unistd.h>

extern const int tab64[64];

// code from:
// http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers��

static inline int log2_64(uint64_t value) {
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  return tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

static inline size_t power2(size_t power) { return (size_t)1 << power; }

#endif // end of CMALLOC_MATH_H
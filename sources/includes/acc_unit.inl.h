#ifndef CMALLOC_ACC_UNIT_H
#define CMALLOC_ACC_UNIT_H

#include "globals.h"

extern int HELPER_DATA_SHADOW_HIGH[NUM_SIZE_CLASSES];
extern short HELPER_DATA_SHADOW_LOW[16][65536 >> 4];

static inline int acc_unit_offdata_to_offshadow(int offset_data,
                                                int size_class) {
  // to ensure that only one bit or none is set to 0
  if (size_class != 0 && size_class != 1 && size_class != 3 &&
      size_class != 7 && size_class < 15)
    return (int)HELPER_DATA_SHADOW_LOW[size_class][offset_data >> 4];
  else
    return offset_data >> HELPER_DATA_SHADOW_HIGH[size_class];
}

#endif // end of CMALLOC_ACC_UNIT_H
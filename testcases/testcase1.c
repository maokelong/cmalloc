/*
    分配一个完整的超级数据块
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
  int i, *ptr;
  for (i = 0; i < 4096; ++i) {
    ptr = (int *)cmalloc_malloc(1);
    printf("%d\t%p\n", i, ptr);
  }
  cmalloc_free(ptr);
  cmalloc_trace();
  return 0;
}
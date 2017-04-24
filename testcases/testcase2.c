/*
        分配一个完整的超级数据块
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
  int i;
  for (i = 0; i < 4096; ++i) {
    int *ptr = (int *)malloc(1);
    printf("%d\t%p\n", i, ptr);
  }
  return 0;
}
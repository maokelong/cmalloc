/*
    测试解除虚拟地址-物理地址映射，从而获得更多物理内存
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>

int *ptrs_recorder[40960];
int num_ptrs;

int main(int argc, char *argv[]) {
  // 申请并覆写 10 个 superblock
  int i, j;
  for (i = 1; i < 11; ++i) {
    int total_mem, request_mem = i * 16;
    for (total_mem = 65536; total_mem > 0; total_mem -= request_mem) {
      int *ptr = (int *)malloc(request_mem);
      ptrs_recorder[num_ptrs++] = ptr;
      for (j = 0; j < request_mem / sizeof(int); ++j)
        *(ptr + j) = 1;
    }
  }

  // 暂停程序
  // getchar();

  // 释放内存
  while (num_ptrs--)
    free(ptrs_recorder[num_ptrs]);

  // 暂停程序
  // getchar();

  return 0;
}
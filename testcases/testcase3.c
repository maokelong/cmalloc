/*
    测试解除虚拟地址-物理地址映射，从而获得更多物理内存
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/size_class.inl.h"
#include <stdio.h>

int *ptrs_recorder[409600];
int num_ptrs;

int main(int argc, char *argv[]) {
  // 申请并覆写 10 * 10 个 superblock
  int i, j, k;
  for (k = 0; k < 10; ++k)
    for (i = 0; i < 10; ++i) {
      int total_mem, request_mem = size_class_block_size(i);
      for (total_mem = 0; total_mem + request_mem <= 65536;
           total_mem += request_mem) {
        int *ptr = (int *)cmalloc_malloc(request_mem);
        ptrs_recorder[num_ptrs++] = ptr;
        for (j = 0; j < request_mem / sizeof(int); ++j)
          *(ptr + j) = 1;
      }
    }

  // 暂停程序，手动观察 RSS
  cmalloc_trace();
  getchar();

  // 释放内存
  while (num_ptrs--)
    cmalloc_free(ptrs_recorder[num_ptrs]);

  // 暂停程序，手动观察 RSS
  cmalloc_trace();
  getchar();

  // 重新申请并覆写 10 * 10 个 superblock
  for (k = 0; k < 10; ++k)
    for (i = 0; i < 10; ++i) {
      int total_mem, request_mem = size_class_block_size(i);
      for (total_mem = 0; total_mem + request_mem < 65536;
           total_mem += request_mem) {
        int *ptr = (int *)cmalloc_malloc(request_mem);
        for (j = 0; j < request_mem / sizeof(int); ++j)
          *(ptr + j) = 1;
      }
    }

  // 打印堆内内存，观察内存重用情况
  cmalloc_trace();

  return 0;
}
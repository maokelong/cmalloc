/*
        测试将 frozen superblock 释放到 global pool
        并从 global pool 中取出这些 superblock
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/globals.h"
#include "../sources/includes/size_classes.h"
#include <stdio.h>

#define NUM_SBS 100
#define REQUEST_SIZE_CLASS 0
int main(int argc, char *argv[]) {
  int i, j;
  SizeClassInit();
  int num_ptrs = 0;
  int *ptrs_recorder[NUM_SBS * SIZE_SDB /
                     SizeClassToBlockSize(REQUEST_SIZE_CLASS)];

  // 申请并覆写 100 个 superblock
  for (i = 0; i < NUM_SBS; ++i) {
    int total_mem, request_mem = SizeClassToBlockSize(REQUEST_SIZE_CLASS);
    for (total_mem = 0; total_mem + request_mem <= 65536;
         total_mem += request_mem) {
      int *ptr = (int *)malloc(request_mem);
      ptrs_recorder[num_ptrs++] = ptr;
      for (j = 0; j < request_mem / sizeof(int); ++j)
        *(ptr + j) = 1;
    }
  }

  malloc_trace();

  // 内存释放
  while (num_ptrs--)
    free(ptrs_recorder[num_ptrs]);

  malloc_trace();

  // 再次申请并覆写 100 个 superblock
  for (i = 0; i < NUM_SBS; ++i) {
    int total_mem, request_mem = SizeClassToBlockSize(REQUEST_SIZE_CLASS);
    for (total_mem = 0; total_mem + request_mem <= 65536;
         total_mem += request_mem) {
      int *ptr = (int *)malloc(request_mem);
      for (j = 0; j < request_mem / sizeof(int); ++j)
        *(ptr + j) = 1;
    }
  }

  malloc_trace();

  return 0;
}
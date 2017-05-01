/*
        测试远程释放内存
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/size_classes.h"
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock;
pthread_cond_t cond;
int *ptrs_recorder[40960];
int num_ptrs;

void thread_job(void) {
  // 远程释放
  while (num_ptrs--)
    free(ptrs_recorder[num_ptrs]);
}

int main(int argc, char *argv[]) {
  // 申请并覆写 10 个 superblock
  int i, j;
  for (i = 0; i < 10; ++i) {
    int total_mem, request_mem = SizeClassToBlockSize(i);
    for (total_mem = 0; total_mem + request_mem <= 65536;
         total_mem += request_mem) {
      int *ptr = (int *)malloc(request_mem);
      ptrs_recorder[num_ptrs++] = ptr;
      for (j = 0; j < request_mem / sizeof(int); ++j)
        *(ptr + j) = 1;
    }
  }

  malloc_trace();

  // 新建线程
  pthread_t thread;
  pthread_create(&thread, NULL, (void *)thread_job, NULL);
  pthread_join(thread, NULL);

  malloc_trace();

  // 再次申请并覆写 10 个 superblock
  for (i = 0; i < 10; ++i) {
    int total_mem, request_mem = SizeClassToBlockSize(i);
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
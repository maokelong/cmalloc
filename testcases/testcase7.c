/*
  测试堆的分配/回收
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/size_class.inl.h"
#include <pthread.h>
#include <stdio.h>

void *ptr;

void thread_job(void) {
  // 远程释放
  if (ptr == NULL)
    cmalloc_free(ptr);
  ptr = cmalloc_malloc(1);
  cmalloc_trace();
}

int main(int argc, char *argv[]) {
  // 主线程开始
  cmalloc_malloc(1);
  cmalloc_trace();

  // 新建线程
  pthread_t thread;
  pthread_create(&thread, NULL, (void *)thread_job, NULL);
  pthread_join(thread, NULL);

  // 新建线程
  pthread_create(&thread, NULL, (void *)thread_job, NULL);
  pthread_join(thread, NULL);

  return 0;
}
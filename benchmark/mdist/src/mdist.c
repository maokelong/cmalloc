#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITER 1000

void *ptr_recorder[MAX_ITER]; // 动态内存记录器

main(int argc, char const *argv[]) {
  int request_size; // 请求内存的大小
  int request_iter; // 请求内存的次数

  if (argc > 2) {
    request_iter = atoi(argv[1]);
    if (request_iter > MAX_ITER)
      fprintf(stderr,
              "Error: request iter shall be less than or equal 1000\n.");

    request_size = atoi(argv[2]);
  }

  // 打印数据段尾地址
  printf("%p\n", ptr_recorder + sizeof(ptr_recorder));

  int i, j;
  for (i = 0; i < request_iter; ++i) {
    // 迭代申请内存
    void *ptr = ptr_recorder[i] = malloc(request_size);
    for (j = 0; j < request_size; ++j) {
      // 迭代填充内存并打印其地址
      *(char *)ptr = 1;
      printf("%p\n", ptr);
    }
  }

  while (i-- >= 0)
    // 迭代释放内存
    free(ptr_recorder[i]);

  return 0;
}

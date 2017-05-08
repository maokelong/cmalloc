#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char const *argv[]) {
  int request_size; // 请求内存的大小
  int request_iter; // 请求内存的次数

  if (argc > 2) {
    request_iter = atoi(argv[1]);
    request_size = atoi(argv[2]);
  }

  int i;
  for (i = 0; i < request_iter; ++i) {
    // 迭代申请内存
    void *ptr = malloc(request_size);
    int j;
    for (j = 0; j < request_size; ++j) {
      // 迭代填充内存并打印其地址
      *(char *)ptr = 1;
      printf("main: %p\n", ptr);
    }
  }

  return 0;
}

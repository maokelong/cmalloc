/*
    检查编译链接能否顺利完成
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
  void *ptr = malloc(0);
  printf("%p\n", ptr);
  return 0;
}

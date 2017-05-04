/*
  测试 large_malloc
  测试 realloc
  测试 memalign
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/globals.h"
#include "../sources/includes/size_classes.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  void *large_mem = malloc(1 MB);
  printf("large mem: %p\n", large_mem);
  free(large_mem);

  void *small_ptr = malloc(17);
  printf("small mem: %p", small_ptr);
  printf(" -> %p\n", realloc(small_ptr, 1));
  free(small_ptr);

  void *aligned_mem = memalign(256, 78);
  printf("aligned mem: %p", aligned_mem);
  free(aligned_mem);

  return 0;
}
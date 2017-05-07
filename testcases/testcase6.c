/*
  测试 large_malloc
  测试 realloc
  测试 memalign
*/

#include "../sources/includes/cmalloc.h"
#include "../sources/includes/globals.h"
#include "../sources/includes/size_class.inl.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  void *large_mem = cmalloc_malloc(1 MB);
  printf("large mem: %p\n", large_mem);
  cmalloc_free(large_mem);

  void *small_ptr = cmalloc_malloc(17);
  printf("small mem: %p", small_ptr);
  printf(" -> %p\n", cmalloc_realloc(small_ptr, 1));
  cmalloc_free(small_ptr);

  void *aligned_mem = cmalloc_memalign(256, 78);
  printf("aligned mem: %p", aligned_mem);
  cmalloc_free(aligned_mem);

  return 0;
}

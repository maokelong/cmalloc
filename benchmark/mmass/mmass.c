#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define KB *(uint64_t)1024
#define MB *(uint64_t)1024 KB
#define GB *(uint64_t)1024 MB

#define PAGE *4 KB

int main(int argc, const char *argv[]) {
  uint64_t size_pa = 1 GB;

  // input size of phycial memory
  if (argc > 1)
    size_pa = atoi(argv[1]) GB;

  // calculate num pages to allocate
  int total_pages = size_pa / (1 PAGE);

  // loop: allocate total_pages pages and recored each addr
  // then touch it
  char **ptr_table = (char **)malloc(sizeof(char *) * total_pages);
  int cur_pages;
  for (cur_pages = 0; cur_pages < total_pages; ++cur_pages) {
    ptr_table[cur_pages] = (char *)malloc(sizeof(char) * 1 PAGE);
    *ptr_table[cur_pages] = 1;
  }

  // loop: free all the recored addr and calc the time span
  struct timeval start, end;
  gettimeofday(&start, NULL);
  for (cur_pages = total_pages - 1; cur_pages >= 0; --cur_pages)
    free(ptr_table[cur_pages]);
  gettimeofday(&end, NULL);
  long timeuse =
      1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
  printf("%d: GB, time koken: %f sec\n", argc == 1 ? 1 : atoi(argv[1]),
         timeuse / 1000000.0);

  return 0;
}
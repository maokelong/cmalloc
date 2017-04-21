#ifndef STORAGE_H
#define STORAGE_H
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>

static inline int get_num_cores(void) { return core_num(); }

static inline int get_core_id(void) {
  int result;
  __asm__ __volatile__("mov $1, %%eax\n"
                       "cpuid\n"
                       : "=b"(result)
                       :
                       : "eax", "ecx", "edx");
  return (result >> 24) % 8;
}

static inline void *require_vm(void *base_addr, size_t size) {
  const int flags = PROT_READ | PROT_WRITE;
  const int prot = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;
  void *ret = mmap(base_addr, size, flags, prot, -1, 0);
  if (base_addr != NULL && ret != base_addr) {
    munmap(base_addr, size);
    return NULL;
  }
  return ret;
}

static inline void *require_nvm(void *file_name, void *base_addr, size_t size) {
  // TODO: nvm allocation
  return NULL;
}
#endif // ! STORAGE_H

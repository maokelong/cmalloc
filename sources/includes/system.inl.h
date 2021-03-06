#ifndef CMALLOC_SYSTEM_INL_H
#define CMALLOC_SYSTEM_INL_H
#include <linux/mman.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>

static inline int get_num_cores(void) { return get_nprocs(); }

static inline int get_core_id(void) {
  int result;
  __asm__ __volatile__("mov $1, %%eax\n"
                       "cpuid\n"
                       : "=b"(result)
                       :
                       : "eax", "ecx", "edx");
  return (result >> 24) % 8;
}

static inline void freeze_vm(void *start_addr, size_t length) {
  madvise(start_addr, length, MADV_DONTNEED);
}

static inline void *request_vm(void *base_addr, size_t size) {
  const int flags = PROT_READ | PROT_WRITE;
  const int prot = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;
  void *ret = mmap(base_addr, size, flags, prot, -1, 0);
  if (base_addr != NULL && ret != base_addr) {
    munmap(base_addr, size);
    return NULL;
  }
  if (ret == MAP_FAILED)
    error_and_exit("CMalloc: Error at %s:%d %s.\n", __FILE__, __LINE__,
                   "failed in requesting memory from OS.");
  return ret;
}

static inline int release_vm(void *start_addr, size_t length) {
  return munmap(start_addr, length);
}

static inline void *require_nvm(void *file_name, void *base_addr, size_t size) {
  // TODO: allocate nvm address space
  return NULL;
}
#endif // end of CMALLOC_SYSTEM_INL_H

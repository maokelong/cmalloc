#ifndef STORAGE_H
#define STORAGE_H
#include <sys/mman.h>
#include <unistd.h>

int get_core_id(void);
void *require_vm(void *base_addr, size_t size);
void *require_nvm(void *file_name, void *base_addr, size_t size);
#endif // ! STORAGE_H

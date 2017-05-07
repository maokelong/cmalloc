#ifndef CMALLOC_GLOBALS_H
#define CMALLOC_GLOBALS_H
/*
 * @文件名：globals.h
 *
 * @说明：定义宏、类型、全局量
 *
 */
#include <stdint.h>
#include <unistd.h>

/*******************************************
 * 宏（机器相关）
 *******************************************/
#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4 * 1024
#define PAGES *PAGE_SIZE
#define KB *(size_t)1024
#define MB *(size_t)1024 KB
#define GB *(size_t)1024 MB
#define TB *(size_t)1024 GB

/*******************************************
 * 宏（分配器参数）
 *******************************************/
#define STEP_TINY_CLASSES 16
#define MAX_TINY_CLASSES 256
#define MAX_MEDIUM_CLASSES 65536
#define NUM_SDB_PAGES 16
#define NUM_SIZE_CLASSES 24
#define SIZE_SDB (size_t)(NUM_SDB_PAGES * PAGE_SIZE)
#define LENGTH_DATA_POOL (size_t)(8 TB)
#define LENGTH_META_POOL (size_t)(1 TB)
#define CMALLOC_LENGTH_REV_ADDR_HASHSET                                        \
  (size_t)256 TB / SIZE_SDB * sizeof(void *)
#define STARTA_ADDR_META_POOL (void *)0x600000000000
#define STARTA_ADDR_DATA_POOL (void *)0x700000000000
#define MACRO_MAX_RATIO_COLD_LIQUID 80
#define MACRO_MAX_RATIO_FROZEN_LIQUID MACRO_MAX_RATIO_COLD_LIQUID * 4

/*******************************************
 * 宏（函数编程）
 *******************************************/
#define ACTIVE ((void *)1)
typedef unsigned long long ptr_t;
#define ROUNDUP(x, n) ((x + n - 1) & (~(n - 1)))
#define ROUNDDOWN(x, n) (((x - n) & (~(n - 1))) + 1)
#define PAGE_ROUNDUP(x) (ROUNDUP((uintptr_t)x, PAGE_SIZE))
#define PAGE_ROUNDDOWN(x) (ROUNDDOWN((uintptr_t)x, PAGE_SIZE))
#define cache_aligned __attribute__((aligned(CACHE_LINE_SIZE)))
#define LENGTH_REV_ADDR_HASHSET (size_t)256 TB / SIZE_SDB * sizeof(void *)
#define thread_local __attribute__((tls_model("initial-exec"))) __thread
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define OffsetOf(struct_name, field_name)                                      \
  (int)(&(((struct_name *)(0x0))->field_name))

/*******************************************
 * 宏（ABA问题）
 * 64位地址中实际只有48位用作地址(ADDR)，
 * 复用其前16位用作ABA计数器(COUNT)
 *******************************************/
#define ABA_ADDR_BIT (48)
#define ABA_ADDR_MASK ((1L << ABA_ADDR_BIT) - 1)
#define ABA_COUNT_MASK (~ABA_ADDR_MASK)
#define ABA_COUNT_ONE (1L << ABA_ADDR_BIT)
#define ABA_ADDR(e) ((void *)((ptr_t)(e)&ABA_ADDR_MASK))
#define ABA_COUNT(e) ((ptr_t)(e)&ABA_COUNT_MASK)

int getRatioColdLiquid(void);
int getRatioFrozenLiquid(void);

void setRatioColdLiquid(int ratio);
void setRatioFrozenLiquid(int ratio);

#endif // end of CMALLOC_GLOBALS_H
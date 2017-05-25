#ifndef CMALLOC_CDS_H
#define CMALLOC_CDS_H

/*
* @文件名：csd.h
*
* @说明：cmaloc 并发数据结构(concurrent data structure)的声明与操作定义
*
*/
#include "globals.h"
#include <stdint.h>
#include <unistd.h>

/*******************************************
 * 结构定义
 *******************************************/
typedef struct treiber_stack_top_ treiber_stack_top;

typedef treiber_stack_top mc_treiber_stack_top;
typedef treiber_stack_top sc_treiber_stack_top;
typedef treiber_stack_top counted_treiber_stack_top;
typedef treiber_stack_top cmprsed_counted_stack_treiber_stack_top;

struct treiber_stack_top_ {
  cache_aligned volatile ptr_t head;
};
#endif // end of CMALLOC_CDS_H

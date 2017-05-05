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
typedef struct queue_head_ queue_head;

typedef queue_head mc_queue_head;
typedef queue_head sc_queue_head;
typedef queue_head counted_queue_head;

struct queue_head_ {
  cache_aligned volatile ptr_t head;
};
#endif // end of CMALLOC_CDS_H

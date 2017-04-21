#ifndef CDS_INL_H
#define CDS_INL_H

#include "atomic.inl.h"

#define NEXT_NODE(ptr, offset) (*(ptr_t *)((char *)(ptr) + offset))

/*******************************************
 * 多消费者(Multi-Consumer) LIFO 队列
 *******************************************/
inline void mc_queue_init(queue_head *queue) { queue->head = 0; }

inline void mc_enqueue(queue_head *queue, void *element, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = queue->head;
    NEXT_NODE(element, next_off) = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)element;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;
    if (compare_and_swap64(&queue->head, old_head, new_head)) {
      return;
    }
  }
}

static inline void *mc_dequeue(queue_head *queue, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;
  void *old_addr;

  while (1) {
    old_head = queue->head;
    old_addr = ABA_ADDR(old_head);
    if (old_addr == NULL) {
      return NULL;
    }
    new_head = NEXT_NODE(old_addr, next_off);
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;
    if (compare_and_swap64(&queue->head, old_head, new_head)) {
      return old_addr;
    }
  }
}

/*******************************************
 * 单消费者(Single-Consumer) LIFO 队列
 *******************************************/

static inline void sc_queue_init(queue_head *queue) { queue->head = 0; }

static inline void sc_enqueue(queue_head *queue, void *element, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = queue->head;
    NEXT_NODE(element, next_off) = old_head;
    new_head = (ptr_t)element;
    if (compare_and_swap64(&queue->head, old_head, new_head)) {
      return;
    }
  }
}

static inline void *sc_dequeue(queue_head *queue, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = queue->head;
    if (old_head == 0) {
      return NULL;
    }
    new_head = NEXT_NODE(old_head, next_off);
    if (compare_and_swap64(&queue->head, old_head, new_head)) {
      return (void *)old_head;
    }
  }
}

static inline void *sc_chain_dequeue(queue_head *queue) {
  unsigned long long old_head;
  while (1) {
    old_head = queue->head;
    if (old_head == 0) {
      return NULL;
    }
    if (compare_and_swap64(&queue->head, old_head, 0)) {
      return (void *)old_head;
    }
  }
}

/*******************************************
 * 计数的(count) 队列
 *******************************************/

static inline void *counted_enqueue(queue_head *queue, void *elem) {
  unsigned long long old_head, new_head, prev;
  do {
    old_head = queue->head;
    *(ptr_t *)elem = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)elem;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;

  } while ((prev = compare_and_swap64_value(&queue->head, old_head,
                                            new_head)) != old_head);

  return (void *)prev;
}

static inline void *counted_chain_enqueue(queue_head *queue, void *elems,
                                          void *tail, int cnt) {
  unsigned long long old_head, new_head, prev;
  do {
    old_head = queue->head;
    *(ptr_t *)tail = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)elems;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE * cnt;

  } while ((prev = compare_and_swap64_value(&queue->head, old_head,
                                            new_head)) != old_head);

  return (void *)prev;
}

static inline void *counted_chain_dequeue(queue_head *queue, uint32_t *count) {
  unsigned long long old_head;
  while (1) {
    old_head = *(ptr_t *)queue;
    if (old_head == 0)
      return (NULL);
    if (compare_and_swap64(&queue->head, old_head, 0)) {
      *count = ABA_COUNT(old_head) >> ABA_ADDR_BIT;
      return (ABA_ADDR(old_head));
    }
  }
}

#endif // end of CDS_INL_H
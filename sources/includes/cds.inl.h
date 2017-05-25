#ifndef CMALLOC_CDS_INL_H
#define CMALLOC_CDS_INL_H

#include "atomic.inl.h"

#define NEXT_NODE(ptr, offset) (*(ptr_t *)((char *)(ptr) + offset))

/*******************************************
 * 多消费者(Multi-Consumer) LIFO 队列
 *******************************************/
static inline void mc_stack_init(treiber_stack_top *stack) { stack->head = 0; }

static inline void mc_push(treiber_stack_top *stack, void *element, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = stack->head;
    NEXT_NODE(element, next_off) = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)element;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;
    if (compare_and_swap64(&stack->head, old_head, new_head)) {
      return;
    }
  }
}

static inline void *mc_pop(treiber_stack_top *stack, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;
  void *old_addr;

  while (1) {
    old_head = stack->head;
    old_addr = ABA_ADDR(old_head);
    if (old_addr == NULL) {
      return NULL;
    }
    new_head = NEXT_NODE(old_addr, next_off);
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;
    if (compare_and_swap64(&stack->head, old_head, new_head)) {
      return old_addr;
    }
  }
}

/*******************************************
 * 单消费者(Single-Consumer) LIFO 队列
 *******************************************/

static inline void sc_stack_init(treiber_stack_top *stack) { stack->head = 0; }

static inline void sc_push(treiber_stack_top *stack, void *element, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = stack->head;
    NEXT_NODE(element, next_off) = old_head;
    new_head = (ptr_t)element;
    if (compare_and_swap64(&stack->head, old_head, new_head)) {
      return;
    }
  }
}

static inline void *sc_pop(treiber_stack_top *stack, int next_off) {
  unsigned long long old_head;
  unsigned long long new_head;

  while (1) {
    old_head = stack->head;
    if (old_head == 0) {
      return NULL;
    }
    new_head = NEXT_NODE(old_head, next_off);
    if (compare_and_swap64(&stack->head, old_head, new_head)) {
      return (void *)old_head;
    }
  }
}

static inline void *sc_chain_pop(treiber_stack_top *stack) {
  unsigned long long old_head;
  while (1) {
    old_head = stack->head;
    if (old_head == 0) {
      return NULL;
    }
    if (compare_and_swap64(&stack->head, old_head, 0)) {
      return (void *)old_head;
    }
  }
}

/*******************************************
 * 计数的(count) 队列
 *******************************************/

static inline uint32_t counted_num_elems(treiber_stack_top *stack) {
  unsigned long long head;
  head = stack->head;
  return ABA_COUNT(head) >> ABA_ADDR_BIT;
}

static inline void *counted_push(treiber_stack_top *stack, void *elem) {
  unsigned long long old_head, new_head, prev;
  do {
    old_head = stack->head;
    *(ptr_t *)elem = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)elem;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;
  } while ((prev = compare_and_swap64_value(&stack->head, old_head,
                                            new_head)) != old_head);

  return (void *)prev;
}

static inline void *counted_chain_push(treiber_stack_top *stack, void *elems,
                                          void *tail, int cnt) {
  unsigned long long old_head, new_head, prev;
  do {
    old_head = stack->head;
    *(ptr_t *)tail = (ptr_t)ABA_ADDR(old_head);
    new_head = (ptr_t)elems;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE * cnt;

  } while ((prev = compare_and_swap64_value(&stack->head, old_head,
                                            new_head)) != old_head);

  return (void *)prev;
}

static inline void *counted_chain_pop(treiber_stack_top *stack, uint32_t *count) {
  unsigned long long old_head;
  while (1) {
    old_head = *(ptr_t *)stack;
    if (old_head == 0)
      return (NULL);
    if (compare_and_swap64(&stack->head, old_head, 0)) {
      *count = ABA_COUNT(old_head) >> ABA_ADDR_BIT;
      return (ABA_ADDR(old_head));
    }
  }
}

/*******************************************
* 计数的(count) 队列 < 压缩后 >
*******************************************/

static inline uint32_t cmprsed_counted_stack_num_elems(treiber_stack_top *stack) {
  unsigned long long head;
  head = stack->head;
  return ABA_COUNT(head) >> ABA_ADDR_BIT;
}

static inline void *cmprsed_counted_stack_push(treiber_stack_top *stack, void *elem,
                                            void *smbend) {
  unsigned long long old_head, new_head, prev;
  do {
    old_head = stack->head;
    *(ptr_t *)elem = ABA_ADDR(old_head) == 0
                         ? (uint16_t)-1
                         : (ABA_ADDR(old_head) - smbend) / sizeof(uint16_t);
    new_head = (ptr_t)elem;
    new_head |= ABA_COUNT(old_head) + ABA_COUNT_ONE;

  } while ((prev = compare_and_swap64_value(&stack->head, old_head,
                                            new_head)) != old_head);

  return (void *)prev;
}
static inline void *cmprsed_counted_stack_chain_pop(treiber_stack_top *stack,
                                                  uint32_t *count) {
  unsigned long long old_head;
  while (1) {
    old_head = *(ptr_t *)stack;
    if (old_head == 0)
      return (NULL);
    if (compare_and_swap64(&stack->head, old_head, 0)) {
      *count = ABA_COUNT(old_head) >> ABA_ADDR_BIT;
      return (ABA_ADDR(old_head));
    }
  }
}

#endif // end of CMALLOC_CDS_INL_H
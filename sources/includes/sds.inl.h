#ifndef CMALLOC_SDS_INL_H
#define CMALLOC_SDS_INL_H
static inline void double_list_init(void *node) {
  double_list_elem *elem_node = (double_list_elem *)node;
  elem_node->next = NULL;
  elem_node->prev = NULL;
}

/* Places new_node at the front of the list. */
static inline void double_list_insert_front(void *new_node, double_list *list) {
  double_list_elem *elem_new = (double_list_elem *)new_node;
  double_list_elem *old_head = list->head;

  if (old_head == NULL) {
    list->tail = elem_new;
  } else {
    old_head->prev = elem_new;
  }

  elem_new->next = old_head;
  elem_new->prev = NULL;
  list->head = elem_new;
}

/* Removes node from the list. */
static inline void double_list_remove(void *node, double_list *list) {
  double_list_elem *elem_node = (double_list_elem *)node;

  if (elem_node->prev != NULL) {
    elem_node->prev->next = elem_node->next;
  } else {
    list->head = elem_node->next;
  }

  if (elem_node->next != NULL) {
    elem_node->next->prev = elem_node->prev;
  } else {
    list->tail = elem_node->prev;
  }

  if (list->head != NULL && list->head->next == NULL) {
    list->tail = list->head;
  } else if (list->tail != NULL && list->tail->prev == NULL) {
    list->head = list->tail;
  }
}

static inline int double_list_visit(double_list *list,
                                    void (*trace)(void *elem)) {
  int num_elems = 0;

  double_list_elem *elem_head = list->head;
  for (; elem_head != NULL; elem_head = elem_head->next, num_elems++)
    if (trace)
      trace((void *)elem_head);
  return num_elems;
}

static inline void stack_init(stack_top *stack) { *stack = NULL; }

static inline void stack_push(stack_top *stack, void *element) {
  *(void **)element = *stack;
  *stack = element;
}

static inline void *stack_pop(stack_top *stack) {
  void *old_head = *stack;
  if (old_head == NULL) {
    return NULL;
  }
  *stack = *(void **)old_head;
  return old_head;
}

static inline int stack_visit(stack_top stack, void (*trace)(void *elem)) {
  int num_elems;
  for (num_elems = 0; stack != NULL; stack = *(void **)stack, num_elems++)
    if (trace)
      trace((void *)stack);
  return num_elems;
}

static inline void cmprsed_stack_init(cmprsed_stack_top *stack) {
  *stack = NULL;
}

static inline void cmprsed_stack_push(cmprsed_stack_top *stack, void *elem,
                                       void *smbend) {
  ((cmpresed_stack_elem *)elem)->sn = unlikely(*stack == NULL)
                                        ? (uint16_t)-1
                                        : (*stack - smbend) / sizeof(uint16_t);
  *stack = elem;
}

static inline void *cmprsed_stack_pop(cmprsed_stack_top *stack, void *smbend) {
  void *old_head = *stack;

  if (old_head == NULL)
    return NULL;

  uint16_t sn = ((cmpresed_stack_elem *)old_head)->sn;
  *stack = unlikely(sn == (uint16_t)-1) ? NULL : sn * sizeof(uint16_t) + smbend;

  return old_head;
}

#endif // end of CMALLOC_SDS_INL_H
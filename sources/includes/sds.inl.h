#ifndef SDS_INL_H
#define SDS_INL_H
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

static inline void seq_queue_init(seq_queue_head *queue) { *queue = NULL; }

static inline void seq_enqueue(seq_queue_head *queue, void *element) {
  *(void **)element = *queue;
  *queue = element;
}

static inline void *seq_dequeue(seq_queue_head *queue) {
  void *old_head = *queue;
  if (old_head == NULL) {
    return NULL;
  }
  *queue = *(void **)old_head;
  return old_head;
}
#endif // end of SDS_INL_H
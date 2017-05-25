#ifndef CMALLOC_SDS_H
#define CMALLOC_SDS_H

/*
* @ File Name : sds.h
*
* @ Intro: Stack identifiers and algorithms
*
*/

/*******************************************
 * double list
 *******************************************/
typedef struct double_list_elem_ double_list_elem;
typedef struct double_list_ double_list;

struct double_list_elem_ {
  void *__padding;
  struct double_list_elem_ *next;
  struct double_list_elem_ *prev;
};

struct double_list_ {
  struct double_list_elem_ *head;
  struct double_list_elem_ *tail;
};

/*******************************************
 * Stack (LIFO)
 *******************************************/
typedef void *stack_top;

/*******************************************
* Compressed Stack (LIFO)
*******************************************/
typedef void *cmprsed_stack_top;

typedef struct { uint16_t sn; } cmpresed_stack_elem;

#endif // end of CMALLOC_SDS_H

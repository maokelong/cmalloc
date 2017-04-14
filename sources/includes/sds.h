#ifndef SDS_H
#define SDS_H

/*
* @ File Name : sds.h
*
* @ Intro: Sequential List identifiers and algorithms
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
 * Sequential List (LIFO)
 *******************************************/
typedef void *seq_queue_head;

#endif // end of SDS_H

#ifndef INODE_LIST_H
#define INODE_LIST_H

#include <stdint.h>
/**********   定义链表结点成员结构   ***********
*结点中不需要数据成元,只要求前驱和后继结点指针*/
typedef struct list_elem {
   struct list_elem* prev; // 前躯结点
   struct list_elem* next; // 后继结点
}list_elem;

/* 链表结构,用来实现队列 */
typedef struct list {
/* head是队首,是固定不变的，不是第1个元素,第1个元素为head.next */
   list_elem head;
/* tail是队尾,同样是固定不变的 */
   list_elem tail;
}list;

void list_init (list* list);
void list_insert_before(list_elem* before, list_elem* elem);
void list_push(list* plist, list_elem* elem);
void list_append(list* plist, list_elem* elem);
void list_remove(list_elem* pelem);
list_elem* list_pop(list* plist);
bool elem_find(list* plist, list_elem* obj_elem);
uint32_t list_len(list* plist);
bool list_empty(list* plist);

#endif
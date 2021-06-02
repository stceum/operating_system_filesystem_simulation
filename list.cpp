#include "list.h"

/* 初始化双向链表list */
void list_init (list* list) {
   list->head.prev = (list_elem*)(0);
   list->head.next = &list->tail;
   list->tail.prev = &list->head;
   list->tail.next = (list_elem*)(0);
}

/* 把链表元素elem插入在元素before之前 */
void list_insert_before(list_elem* before, list_elem* elem) { 
/* 将before前驱元素的后继元素更新为elem, 暂时使before脱离链表*/ 
   before->prev->next = elem; 

/* 更新elem自己的前驱结点为before的前驱,
 * 更新elem自己的后继结点为before, 于是before又回到链表 */
   elem->prev = before->prev;
   elem->next = before;

/* 更新before的前驱结点为elem */
   before->prev = elem;
}

/* 添加元素到列表队首,类似栈push操作 */
void list_push(list* plist, list_elem* elem) {
   list_insert_before(plist->head.next, elem); // 在队头插入elem
}

/* 追加元素到链表队尾,类似队列的先进先出操作 */
void list_append(list* plist, list_elem* elem) {
   list_insert_before(&plist->tail, elem);     // 在队尾的前面插入
}

/* 使元素pelem脱离链表 */
void list_remove(list_elem* pelem) { 
   pelem->prev->next = pelem->next;
   pelem->next->prev = pelem->prev;
}

/* 将链表第一个元素弹出并返回,类似栈的pop操作 */
list_elem* list_pop(list* plist) {
list_elem* elem = plist->head.next;
   list_remove(elem);
   return elem;
} 

/* 从链表中查找元素obj_elem,成功时返回true,失败时返回false */
bool elem_find(list* plist, list_elem* obj_elem) {
   list_elem* elem = plist->head.next;
   while (elem != &plist->tail) {
      if (elem == obj_elem) {
	 return true;
      }
      elem = elem->next;
   }
   return false;
}

/* 返回链表长度 */
uint32_t list_len(list* plist) {
   list_elem* elem = plist->head.next;
   uint32_t length = 0;
   while (elem != &plist->tail) {
      length++; 
      elem = elem->next;
   }
   return length;
}

/* 判断链表是否为空,空时返回true,否则返回false */
bool list_empty(list* plist) {		// 判断队列是否为空
   return (plist->head.next == &plist->tail ? true : false);
}


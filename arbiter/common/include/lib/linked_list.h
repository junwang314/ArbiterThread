#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

#include <stdbool.h>

// a very simple doubly linked-list
struct list_node {
	void *data;
	struct list_node *prev;
	struct list_node *next;
};

struct linked_list {
	struct list_node *head;
	struct list_node *tail;
	//number of items 
	int num;
};

static inline void init_linked_list(struct linked_list *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->num = 0;
}

void list_insert_head(struct linked_list *list, void *data);
void list_insert_tail(struct linked_list *list, void *data);

void *list_del_entry(struct linked_list *list, struct list_node *entry);

static inline void *list_remove_head(struct linked_list *list)
{
	if (list->head)
		return list_del_entry(list, list->head);
	return NULL;
}

static inline void *list_remove_tail(struct linked_list *list)
{
	if (list->tail)
		return list_del_entry(list, list->tail);
	return NULL;
}

//locate the node by given datum
struct list_node *linked_list_locate(struct linked_list *list, void *data);

//lookup the datum by value, return the first match
void *linked_list_lookup(struct linked_list *list, void *key, 
			 bool (*cmpf)(const void *key, const void *data));


#endif //_LINKED_LIST_H

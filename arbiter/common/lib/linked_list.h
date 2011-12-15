#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

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

//search function and list_for_each not implemented


#endif //_LINKED_LIST_H

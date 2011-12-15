//a simple linked list implementation 
//depends on glibc for allocation

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "linked_list.h"

void list_insert_head(struct linked_list *list, void *data)
{
	struct list_node *new = (struct list_node *)malloc(sizeof(struct list_node));
	assert(new);
	new->prev = NULL;
	new->next = list->head;
	new->data = data;
	//empty list
	if (list->head == NULL)	
		list->tail = new;	
	else
		list->head->prev = new;
	list->head = new;
	list->num += 1;
}


void list_insert_tail(struct linked_list *list, void *data)
{
	struct list_node *new = (struct list_node *)malloc(sizeof(struct list_node));
	assert(new);
	new->prev = list->tail;
	new->next = NULL;
	new->data = data;

	if (list->tail == NULL)
		list->head = new;
	else
		list->tail->next = new;

	list->tail = new;
	list->num += 1;
}

//must guarantee that entry belongs to the list
void *list_del_entry(struct linked_list *list, struct list_node *entry)
{
	void *data;
	if (entry == list->head)
		list->head = list->head->next;
	if (entry == list->tail)
		list->tail = list->tail->prev;
	if (entry->prev) 
		entry->prev->next = entry->next;
	if (entry->next)
		entry->next->prev = entry->prev;

	data = entry->data;
	//free the entry
	free(entry);
	list->num -= 1;
	return data;
}


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

list_t* list_create()
{
	list_t *l = (list_t*)malloc(sizeof(list_t));
	if (l == NULL) {
		fprintf(stderr, "list_create(): Could not allocate memory for list\n");
		return NULL;
	}

	l->count = 0;
	l->head = NULL;
	l->tail = NULL;
	pthread_mutex_init(&(l->mutex), NULL);
	return l;
}

void list_free(list_t *l)
{
	list_item_t *li, *tmp;

	pthread_mutex_lock(&(l->mutex));

	if (l != NULL) {
		li = l->head;
		while (li != NULL) {
			tmp = li->next;
			free(li);
			li = tmp;
		}
	}

	pthread_mutex_unlock(&(l->mutex));
	pthread_mutex_destroy(&(l->mutex));
	free(l);
}

list_item_t* list_add_element(list_t *l, void *ptr)
{
	list_item_t *li;

	pthread_mutex_lock(&(l->mutex));

	li = (list_item_t*)malloc(sizeof(list_item_t));
	li->value = ptr;
	li->next = NULL;
	li->prev = l->tail;

	if (l->tail == NULL) {
		l->head = l->tail = li;
	}
	else {
		l->tail = li;
	}
	l->count++;

	pthread_mutex_unlock(&(l->mutex));

	return li;
}

int list_remove_element(list_t *l, void *ptr)
{
	int result = 0;
	list_item_t *li = l->head;

	pthread_mutex_lock(&(l->mutex));

	while (li != NULL) {
		if (li->value == ptr) {
			if (li->prev == NULL) {
				l->head = li->next;
			}
			else {
				li->prev->next = li->next;
			}

			if (li->next == NULL) {
				l->tail = li->prev;
			}
			else {
				li->next->prev = li->prev;
			}
			l->count--;
			free(li);
			result = 1;
			break;
		}
		li = li->next;
	}

	pthread_mutex_unlock(&(l->mutex));

	return result;
}

int list_contains_element(list_t *l, void *ptr)
{
	int result = 0;
	list_item_t *li = l->head;

	pthread_mutex_lock(&(l->mutex));

	while (li != NULL) {
		//if (li->value == ptr) {
		if (strcmp(li->value, ptr) == 0) {
			result = 1;
			break;
		}
		li = li->next;
	}

	pthread_mutex_unlock(&(l->mutex));

	return result;
}
#include <pthread.h>

typedef struct list_item_t {
	void *value;
	struct list_item_t *prev;
	struct list_item_t *next;
} list_item_t;

typedef struct list_t {
	int count;
	list_item_t *head;
	list_item_t *tail;
	pthread_mutex_t mutex;
} list_t;

list_t *list_create();
void list_free(list_t *l);

list_item_t *list_add_element(list_t *l, void *ptr);
int list_remove_element(list_t *l, void *ptr);
int list_contains_element(list_t *l, void *ptr);
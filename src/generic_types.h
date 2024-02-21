#ifndef generic_types_h
#define generic_types_h

typedef int (*equality_fn)(void*, void*);
typedef void (*data_free_fn)(void*);

typedef struct node {
    void* data;
    struct node* next;
} node;

typedef struct linked_list {
    node* current;
    data_free_fn dt_free_fn;
    equality_fn eq_fn;
} linked_list;

linked_list* linked_list_create(data_free_fn dt_free_fn, equality_fn eq_fn);
void linked_list_free(linked_list* l);
int linked_list_add(linked_list* l, void* data);
void* linked_list_get(linked_list* l, void* data);
int linked_list_remove(linked_list* l, void* data);

#endif
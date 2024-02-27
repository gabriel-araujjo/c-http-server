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

typedef struct stack {
    struct node* head;
    data_free_fn dt_free_fn;
} stack;

stack* stack_create(data_free_fn dt_free_fn);
void stack_free(stack* s);
int stack_push(stack* s, void* data);
void* stack_pop(stack* s);

typedef struct doubly_linked_node {
    void* data;
    struct doubly_linked_node* next;
    struct doubly_linked_node* prev;
} doubly_linked_node;

typedef struct doubly_linked_list {
    doubly_linked_node* start;
    data_free_fn dt_free_fn;
    equality_fn eq_fn;
} doubly_linked_list;

doubly_linked_list* doubly_linked_list_create(data_free_fn dt_free_fn, equality_fn eq_fn);
void doubly_linked_list_free(doubly_linked_list* d);
int doubly_linked_list_add(doubly_linked_list* d, void* data);
void* doubly_linked_list_remove(doubly_linked_list* d, void* data);

#endif
#ifndef queue_h
#define queue_h

#include "generic_types.h"

typedef struct queue {
    node* head;
    node* tail;
    int* size;
    data_free_fn dt_free_fn;
} queue;

// Creates a new empty queue
queue* queue_create(data_free_fn dt_free);

// Frees queue, releasing memory-allocated nodes along the way.
void queue_free(queue* q);

int queue_enqueue(queue* q, void* data);
void* queue_dequeue(queue* q);
int queue_is_empty(queue* q);

#endif
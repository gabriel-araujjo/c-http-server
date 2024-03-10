#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "generic_types.h"

queue* queue_create(data_free_fn dt_free_fn) {
    size_t s = sizeof(queue);
    queue* q = (queue*)malloc(s);
    q->head = q->tail = NULL;
    q->size = malloc(sizeof(int));
    *q->size = 0;
    q->dt_free_fn = dt_free_fn;
    return q;
}

void queue_free(queue *q) {
    node* n = q->head;
    for (; n != NULL; ) {
        node* next = n->next;
        if (n->data) q->dt_free_fn(n->data);
        free(n);
        n = next;
    }

    free(q);
}

int queue_enqueue(queue* q, void* data) {
    if (q == NULL) {
        printf("queue is not initialized, please malloc\n");
        return 0;
    }

    size_t s = sizeof(node);
    node* n = (node*)malloc(s);
    n->data = data;
    n->next = NULL;

    if (queue_is_empty(q)) {
        q->tail = q->head = n;
    } else if (q->tail != q->head) {
        q->tail->next = n;
        q->tail = n;
    } else {
        q->head->next = n;
        q->tail = n;
    }

    *q->size++;

    return 1;
}

void* queue_dequeue(queue *q) {
    if (q == NULL) {
        printf("queue is not initialized, please malloc and add some item\n");
        return NULL;
    }

    void *data = NULL;
    if (queue_is_empty(q)) {
        printf("queue is empty, buddy\n");
    } else if (q->head == q->tail) {
        data = q->head->data;
        free(q->head);
        q->head = q->tail = NULL;
        *q->size--;
    } else {
        data = q->head->data;
        node* n = q->head;
        q->head = q->head->next;
        free(n);
        *q->size--;
    }

    return data;
}

int queue_is_empty(queue* q) {
    return (q->head == q->tail) && (q->head == NULL);
}

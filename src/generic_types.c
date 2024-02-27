#include <stdlib.h>
#include "generic_types.h"

linked_list* linked_list_create(data_free_fn dt_free_fn, equality_fn eq_fn) {
    size_t s = sizeof(linked_list);
    linked_list* l = malloc(s);
    l->current = NULL;
    l->dt_free_fn = dt_free_fn;
    l->eq_fn = eq_fn;
    return l;
}

void linked_list_free(linked_list* l) {
    if (!l) return;
    if (!l->current) return;
    node* n = NULL;
    while (l->current) {
        n = l->current;
        l = l->current->next;
        l->dt_free_fn(n->data);
        free(n);
    }
    free(l);
}

int linked_list_add(linked_list* l, void* data) {
    if (!l) return 0;

    size_t s = sizeof(node);
    node* n = malloc(s);
    n->data = data;
    n->next = NULL;
    if (!l->current) {
        l->current = n;
        return 1;
    }
    node* last = l->current;
    for (; last->next != NULL; last = last->next) {}
    last->next = n;
    return 1;
}

void* linked_list_get(linked_list* l, void* data) {
    if (!l || !l->current) return NULL;

    for (node *curr = l->current; curr != NULL; curr = l->current) {
        if (!l->eq_fn(data, curr->data)) continue;
        return curr->data;
    }
    
    return NULL;
}

int linked_list_remove(linked_list* l, void* data) {
    if (!l || !l->current) return 0;

    node* prev = NULL;
    node* curr = l->current;
    while (curr != NULL) {
        if (!l->eq_fn(data, curr->data)) {
            prev = curr;
            curr = curr->next;
            continue;
        };
        if (prev) prev->next = curr->next;
        l->dt_free_fn(curr->data);
        free(curr);
        return 1;
    }

    return 0;
}

stack* stack_create(data_free_fn dt_free_fn) {
    stack* s = malloc(sizeof(stack));
    s->head = NULL;
    s->dt_free_fn = dt_free_fn;
    return s;
}

void stack_free(stack* s) {
    if (!s || !s->head) return;
    node* n = s->head;
    while (n) {
        node* m = n;
        n = n->next;
        s->dt_free_fn(m->data);
        free(m);
        m = NULL;
    }
    free(s);
    s = NULL;
}

int stack_push(stack* s, void* data) {
    if (!s) return 0;
    node* n = malloc(sizeof(node));
    n->data = data;
    n->next = s->head;
    s->head = n;
    return 1;
}

void* stack_pop(stack* s) {
    if (!s || !s->head) return NULL;
    node* n = s->head;
    s->head = n->next;
    void* data = n->data;
    // printf("dt: %ld\n", data);
    free(n);
    n = NULL;
    return data;
}

doubly_linked_list* doubly_linked_list_create(data_free_fn dt_free_fn, equality_fn eq_fn) {
    doubly_linked_list* d = malloc(sizeof(doubly_linked_list));
    d->dt_free_fn = dt_free_fn;
    d->eq_fn = eq_fn;
    d->start = NULL;
    return d;
}

void doubly_linked_list_free(doubly_linked_list* d) {
    if (!d || !d->start) return;
    doubly_linked_node* n = d->start;
    while (n) {
        doubly_linked_node* m = n;
        n = n->next;
        // TODO: seems to free a dangling pointer, needs further investigation
        d->dt_free_fn(m->data);
        free(m);
        m = NULL;
    }
    free(d);
    d = NULL;
    return;
}

int doubly_linked_list_add(doubly_linked_list* d, void* data) {
    if (!d) return 0;
    doubly_linked_node* n = malloc(sizeof(doubly_linked_node));
    n->data = data;
    n->next = NULL;
    doubly_linked_node* m = d->start;
    if (!m) {
        n->prev = NULL;
        d->start = n;
        return 1;
    }
    while (m->next) m = m->next;
    m->next = n;
    n->prev = m;
    return 1;
}

void* doubly_linked_list_remove(doubly_linked_list* d, void* data) {
    if (!d || !d->start) return NULL;
    doubly_linked_node* n = NULL;
    for (doubly_linked_node* m = d->start; m != NULL; m = m->next) {
        if (!d->eq_fn(m->data, data)) continue;
        n = m;
        break;
    }
    if (!n) return NULL;
    void* found = n->data;
    if (n->prev && n->next) {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }
    else if (n->prev) n->prev->next = NULL;
    else if (n->next) n->next->prev = NULL;
    free(n);
    n = NULL;
    return found;
}

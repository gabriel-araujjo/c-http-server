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
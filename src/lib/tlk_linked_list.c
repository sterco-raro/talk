#include "../../include/tlk_linked_list.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct _linked_list_node{
    void *value;
    struct _linked_list_node *next;
} linked_list_node;

typedef struct {
    linked_list_node *head;
    linked_list_node *tail;
    int size;
} linked_list_struct;

int linked_list_get(linked_list * ll, int index, void **value) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    if (index < 0 || index >= ptr->size)
        return LINKED_LIST_NOK;
    int i;
    linked_list_node *current = ptr->head;
    for (i = 0; i < index && current != NULL; i++) {
        current = current->next;
    }
    if (current == NULL)
        return LINKED_LIST_NOK;
    *value = current->value;
    return LINKED_LIST_OK;
}

int linked_list_add(linked_list * ll, void *value) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    linked_list_node *added = (linked_list_node *) malloc(sizeof(linked_list_node));
    added->next = NULL;
    added->value = value;
    if (ptr->tail != NULL)
        ptr->tail->next = added;
    if (ptr->head == NULL)
        ptr->head = added;
    ptr->tail = added;
    ptr->size++;
}

int linked_list_remove(linked_list *ll, int index) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    if (index < 0 || index >= ptr->size)
        return LINKED_LIST_NOK;
    int i;
    linked_list_node *current = ptr->head;
    linked_list_node *previous = NULL;
    for (i = 0; i < index && current != NULL; i++) {
        previous = current;
        current = current->next;
    }
    if (ptr->head == ptr->tail)
        ptr->tail = NULL;
    if (previous == NULL)
        ptr->head = current->next;
    else
        previous->next = current->next;
    free(current);
    ptr->size--;
    return LINKED_LIST_OK;
}

int linked_list_tostring(linked_list *ll, char *buffer) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    buffer[0] = '\0';
    sprintf(buffer, "%s[", buffer);
    int i;
    linked_list_node *current = ptr->head;
    for (i = 0; i < ptr->size || current != NULL; i++) {
        sprintf(buffer, "%s%s%p", buffer, (i != 0 ? ", " : ""), current->value);
        current = current->next;
    }
    sprintf(buffer, "%s]", buffer);
    return LINKED_LIST_OK;
}

linked_list * linked_list_new() {
    linked_list_struct *ptr = (linked_list_struct *) malloc(sizeof(linked_list_struct));
    ptr->head = NULL;
    ptr->tail = NULL;
    ptr->size = 0;
    return (linked_list *) ptr;
}

int linked_list_delete(linked_list *ll) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    int i;
    for (i = 0; ptr->size > 0; i++) {
        linked_list_remove(ll, 0);
    }
    free(ll);
}

linked_list_iterator * linked_list_iterator_new(linked_list *ll) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    return (linked_list_iterator *) ptr->head;
}

linked_list_iterator * linked_list_iterator_next(linked_list_iterator * iter) {
    linked_list_node *ptr = (linked_list_node *) iter;
    return ptr->next;
}

void * linked_list_iterator_getvalue(linked_list_iterator *iter) {
    linked_list_node *ptr = (linked_list_node *) iter;
    return ptr->value;
}

int linked_list_is_empty(linked_list *ll) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    if (ptr->size == 0)
        return LINKED_LIST_OK;
    return LINKED_LIST_NOK;
}

int linked_list_size(linked_list *ll) {
    linked_list_struct *ptr = (linked_list_struct *) ll;
    return ptr->size;
}

int linked_list_contains(linked_list *ll, void *n) {
    linked_list_struct * lls = (linked_list_struct *) ll;
    linked_list_node * current = lls->head;
    while (current != NULL) {
        if (current->value == n)
            return LINKED_LIST_OK;
        current = current->next;
    }
    return LINKED_LIST_NOK;
}

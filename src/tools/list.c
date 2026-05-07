#include "list.h"

void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list->prev = list;
}

void list_add(struct list_head *new, struct list_head *head) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

void list_add_tail(struct list_head *new, struct list_head *head) {
    struct list_head *prev = head->prev;
    new->next = head;
    new->prev = prev;
    prev->next = new;
    head->prev = new;
}

void list_del(struct list_head *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

int list_empty(const struct list_head *head)
{
    return head->next == head;
}
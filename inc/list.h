#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>

struct list_head
{
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)                   \
    for (pos = container_of((head)->next, typeof(*pos), member); \
         &pos->member != (head);                                 \
         pos = container_of(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)            \
    for (pos = container_of((head)->next, typeof(*pos), member),  \
        n = container_of(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                  \
         pos = n,                                                 \
        n = container_of(n->member.next, typeof(*n), member))

void list_del(struct list_head *entry);
void INIT_LIST_HEAD(struct list_head *list);
int list_empty(const struct list_head *head);
void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);

#endif // LIST_H
#ifndef QUEUE_H
#define QUEUE_H

#include <list.h>
#include <spinlock.h>

typedef struct {
	struct list_head head; // 哨兵节点
	spinlock_t lock;
} queue_t;

#define QUEUE_INIT(q)                                                          \
	do {                                                                   \
		INIT_LIST_HEAD(&(q)->head);                                    \
		spinlock_init(&(q)->lock);                                     \
	} while (0)

// 入队（尾插）
void queue_enqueue(queue_t *q, struct list_head *node);

// 出队（头取）
struct list_head *queue_dequeue(queue_t *q);

int queue_empty(queue_t *q);

#endif // QUEUE_H
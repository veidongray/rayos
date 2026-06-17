#include <queue.h>

// 入队（尾插）
void queue_enqueue(queue_t *q, struct list_head *node)
{
    spinlock_lock(&q->lock);
    list_add_tail(node, &q->head); // 加到 tail
    spinlock_unlock(&q->lock);
}

// 出队（头取）
struct list_head *queue_dequeue(queue_t *q)
{
    struct list_head *node = NULL;
    spinlock_lock(&q->lock);
    if (!list_empty(&q->head))
    {
        node = q->head.next;
        list_del(node);
    }
    spinlock_unlock(&q->lock);
    return node;
}

int queue_empty(queue_t *q)
{
    int is_empty;

    spinlock_lock(&q->lock);
    is_empty = list_empty(&q->head);
    spinlock_unlock(&q->lock);

    return is_empty;
}
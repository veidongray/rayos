#include "semaphore.h"
#include "atomic.h"
#include "task.h"

void semaphore_init(struct semaphore *sem, int32_t initial_count)
{
    atomic_store(&sem->count, initial_count);
    spinlock_init(&sem->spin);
    sem->wait_queue_head.next = &sem->wait_queue_head;
    sem->wait_queue_head.prev = &sem->wait_queue_head;
}

void semaphore_p(struct semaphore *sem)
{
    spinlock_lock(&sem->spin);
    sem->count--;
    if (sem->count < 0)
    {
        current->task_status = TASK_BLOCKED;
        list_del(&current->list);
        list_add_tail(&current->list, &sem->wait_queue_head);
        spinlock_unlock(&sem->spin);
        scheduler();
    }
    spinlock_unlock(&sem->spin);
}

void semaphore_v(struct semaphore *sem)
{
    struct task_struct *task;
    spinlock_lock(&sem->spin);
    sem->count++;
    if (!list_empty(&sem->wait_queue_head))
    {
        task = container_of(sem->wait_queue_head.next, struct task_struct, list);
        task->task_status = TASK_READY;
        list_del(&task->list);
        list_add(&task->list, &current->list);
    }
    spinlock_unlock(&sem->spin);
}
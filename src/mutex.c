#include <int.h>
#include <task.h>
#include <mutex.h>
#include <atomic.h>

void mutex_init(mutex_t *mutex)
{
    atomic_store(&mutex->locked, MUTEX_UNLOCK);
    spinlock_init(&mutex->spinlock);
    INIT_LIST_HEAD(&mutex->wait_queue);
}

int mutex_trylock(mutex_t *mutex)
{
    int64_t expected = MUTEX_UNLOCK;
    return atomic_compare_exchange(&mutex->locked, &expected, MUTEX_LOCKED);
}

int mutex_lock(mutex_t *mutex)
{
    struct task_struct *current;

    while (!mutex_trylock(mutex))
    {
        spinlock_lock(&mutex->spinlock);
        current = get_current();
        list_del(&current->list);
        list_add_tail(&current->list, &mutex->wait_queue);
        spinlock_unlock(&mutex->spinlock);

        current->status = TASK_BLOCKED;
        disable_irq();
        scheduler();
    }
    return 1;
}

int mutex_unlock(mutex_t *mutex)
{
    struct list_head *task_list;
    struct task_struct *task;

    spinlock_lock(&mutex->spinlock);
    atomic_store(&mutex->locked, MUTEX_UNLOCK);
    if (!list_empty(&mutex->wait_queue))
    {
        task_list = get_tasklist();
        task = container_of(mutex->wait_queue.next, struct task_struct, list);
        list_del(&task->list);
        list_add(&task->list, task_list);
        task->status = TASK_READY;
        spinlock_unlock(&mutex->spinlock);
    }
    else
    {
        spinlock_unlock(&mutex->spinlock);
    }
    return 0;
}
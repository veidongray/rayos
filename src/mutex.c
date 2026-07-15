#include <atomic.h>
#include <int.h>
#include <mutex.h>
#include <task.h>

void mutex_init(mutex_t *mt)
{
	QUEUE_INIT(&mt->wait_queue);
	atomic_store(&mt->locked, MUTEX_UNLOCK);
}

int mutex_trylock(mutex_t *mt)
{
	int64_t expected = MUTEX_UNLOCK;
	return atomic_compare_exchange(&mt->locked, &expected, MUTEX_LOCKED);
}

int mutex_lock(mutex_t *mt)
{
	struct task_struct *current;

	while (!mutex_trylock(mt)) {
		current = get_current();
		current->status = TASK_BLOCKED;
		queue_enqueue(&mt->wait_queue, &current->list);
		scheduler();
	}
	return 1;
}

int mutex_unlock(mutex_t *mt)
{
	struct task_struct *task;

	atomic_store(&mt->locked, MUTEX_UNLOCK);
	if (!queue_empty(&mt->wait_queue)) {
		task = container_of(queue_dequeue(&mt->wait_queue),
		                    struct task_struct, list);
		task->status = TASK_READY;
		queue_enqueue(get_task_readyqueue(), &task->list);
	}
	return 0;
}
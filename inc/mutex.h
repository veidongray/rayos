#ifndef MUTEX_H
#define MUTEX_H

#include <list.h>
#include <queue.h>
#include <atomic.h>
#include <spinlock.h>

enum mutex_status
{
    MUTEX_LOCKED,
    MUTEX_UNLOCK
};

struct mutex
{
    queue_t wait_queue;
    atomic_int_t locked;
};

typedef struct mutex mutex_t;

int mutex_lock(mutex_t *mt);
void mutex_init(mutex_t *mt);
int mutex_unlock(mutex_t *mt);
int mutex_trylock(mutex_t *mt);
#endif // MUTEX_H
#ifndef MUTEX_H
#define MUTEX_H

#include <list.h>
#include <atomic.h>
#include <spinlock.h>

enum mutex_status
{
    MUTEX_LOCKED,
    MUTEX_UNLOCK
};

struct mutex
{
    atomic_int_t locked;
    spinlock_t spinlock;
    struct list_head wait_queue;
};

typedef struct mutex mutex_t;

void mutex_init(mutex_t *mt);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
int mutex_trylock(mutex_t *mutex);
#endif // MUTEX_H
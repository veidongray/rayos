#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>
#include "spinlock.h"
#include "list.h"

struct semaphore
{
    int32_t count;
    spinlock_t spin;
    struct list_head wait_queue_head;
};

void semaphore_init(struct semaphore *sem, int32_t initial_count);
void semaphore_p(struct semaphore *sem);
void semaphore_v(struct semaphore *sem);

#endif // SEMAPHORE_H
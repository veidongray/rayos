#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef volatile int32_t spinlock_t;

#define SPINLOCK_INIT(name) spinlock_t name

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);
int spinlock_try_lock(spinlock_t *lock);

#endif // SPINLOCK_H
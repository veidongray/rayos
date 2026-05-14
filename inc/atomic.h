#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

typedef volatile int32_t atomic_int_t;

#define ATOMIC_INT_INIT 0

int32_t atomic_load(const atomic_int_t *ptr);
void atomic_store(atomic_int_t *ptr, int32_t val);
int atomic_compare_exchange(atomic_int_t *ptr, int32_t *expected, int32_t desired);
int32_t atomic_fetch_add(atomic_int_t *ptr, int32_t val);
int32_t atomic_fetch_sub(atomic_int_t *ptr, int32_t val);
int32_t atomic_fetch_or(atomic_int_t *ptr, int32_t val);
int32_t atomic_fetch_and(atomic_int_t *ptr, int32_t val);
int32_t atomic_exchange(atomic_int_t *ptr, int32_t val);

#endif // ATOMIC_H
#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

// 确保 8 字节对齐
typedef volatile int64_t atomic_int_t __attribute__((aligned(8)));

#define ATOMIC_INT_INIT 0LL

int64_t atomic_load(const atomic_int_t *ptr);
void atomic_store(atomic_int_t *ptr, int64_t val);
int atomic_compare_exchange(atomic_int_t *ptr, int64_t *expected,
                            int64_t desired);
int64_t atomic_fetch_add(atomic_int_t *ptr, int64_t val);
int64_t atomic_fetch_sub(atomic_int_t *ptr, int64_t val);
int64_t atomic_fetch_or(atomic_int_t *ptr, int64_t val);
int64_t atomic_fetch_and(atomic_int_t *ptr, int64_t val);
int64_t atomic_exchange(atomic_int_t *ptr, int64_t val);

#endif // ATOMIC_H
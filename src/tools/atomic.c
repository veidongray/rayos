#include "atomic.h"

int32_t atomic_load(const atomic_int_t *ptr)
{
    int32_t result;
    asm volatile(
        "movl %1, %0"
        : "=r"(result)
        : "m"(*ptr)
        : "memory");
    return result;
}

void atomic_store(atomic_int_t *ptr, int32_t val)
{
    asm volatile(
        "movl %1, %0"
        : "=m"(*ptr)
        : "r"(val)
        : "memory");
}

int atomic_compare_exchange(atomic_int_t *ptr, int32_t *expected, int32_t desired)
{
    int success;
    asm volatile(
        "lock cmpxchgl %3, %1\n\t"
        "setz %b0"
        : "=q"(success), "+m"(*ptr), "+a"(*expected)
        : "r"(desired)
        : "memory");
    return success;
}

int32_t atomic_fetch_add(atomic_int_t *ptr, int32_t val)
{
    int32_t result;
    asm volatile(
        "lock xaddl %0, %1"
        : "=r"(result), "+m"(*ptr)
        : "0"(val)
        : "memory");
    return result;
}

int32_t atomic_fetch_sub(atomic_int_t *ptr, int32_t val)
{
    // Subtraction is just addition of the negative value.
    // atomic_fetch_add already handles the atomicity.
    return atomic_fetch_add(ptr, -val);
}

int32_t atomic_fetch_or(atomic_int_t *ptr, int32_t val)
{
    int32_t old_val, new_val;
    do
    {
        old_val = atomic_load(ptr);
        new_val = old_val | val;
    } while (!atomic_compare_exchange(ptr, &old_val, new_val));
    return old_val;
}

int32_t atomic_fetch_and(atomic_int_t *ptr, int32_t val)
{
    int32_t old_val, new_val;
    do
    {
        old_val = atomic_load(ptr);
        new_val = old_val & val;
    } while (!atomic_compare_exchange(ptr, &old_val, new_val));
    return old_val;
}

int32_t atomic_exchange(atomic_int_t *ptr, int32_t val)
{
    int32_t result;
    asm volatile(
        "lock xchgl %0, %1"
        : "=r"(result), "+m"(*ptr)
        : "0"(val)
        : "memory");
    return result;
}
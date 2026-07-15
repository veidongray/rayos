#include <atomic.h>

int64_t atomic_load(const atomic_int_t *ptr)
{
	int64_t result;
	asm volatile("movq %1, %0" : "=r"(result) : "m"(*ptr) : "memory");
	return result;
}

void atomic_store(atomic_int_t *ptr, int64_t val)
{
	asm volatile("movq %1, %0" : "=m"(*ptr) : "r"(val) : "memory");
}

int atomic_compare_exchange(atomic_int_t *ptr, int64_t *expected,
                            int64_t desired)
{
	int success;
	asm volatile("lock cmpxchgq %3, %1\n\t"
	             "setz %b0"
	             : "=r"(success), "+m"(*ptr), "+a"(*expected)
	             : "r"(desired)
	             : "memory");
	return success;
}

int64_t atomic_fetch_add(atomic_int_t *ptr, int64_t val)
{
	int64_t result;
	asm volatile("lock xaddq %0, %1"
	             : "=r"(result), "+m"(*ptr)
	             : "0"(val)
	             : "memory");
	return result;
}

int64_t atomic_fetch_sub(atomic_int_t *ptr, int64_t val)
{
	return atomic_fetch_add(ptr, -val);
}

int64_t atomic_fetch_or(atomic_int_t *ptr, int64_t val)
{
	int64_t old_val, new_val;
	do {
		old_val = atomic_load(ptr);
		new_val = old_val | val;
	} while (!atomic_compare_exchange(ptr, &old_val, new_val));
	return old_val;
}

int64_t atomic_fetch_and(atomic_int_t *ptr, int64_t val)
{
	int64_t old_val, new_val;
	do {
		old_val = atomic_load(ptr);
		new_val = old_val & val;
	} while (!atomic_compare_exchange(ptr, &old_val, new_val));
	return old_val;
}

int64_t atomic_exchange(atomic_int_t *ptr, int64_t val)
{
	int64_t result;
	asm volatile("lock xchgq %0, %1"
	             : "=r"(result), "+m"(*ptr)
	             : "0"(val)
	             : "memory");
	return result;
}
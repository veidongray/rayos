#include <atomic.h>
#include <spinlock.h>

void spinlock_init(spinlock_t *lock)
{
    atomic_store(lock, 0); // Unlocked state is 0
}

void spinlock_lock(spinlock_t *lock)
{
    // Continuously try to acquire the lock using compare-and-swap
    while (1)
    {                         // Loop until success
        int64_t expected = 0; // Expected unlocked state
        // Attempt to swap 1 (locked) into the lock if it's currently 0 (unlocked)
        if (atomic_compare_exchange(lock, &expected, 1))
        {
            // Success! The lock was 0, we changed it to 1, and now own the lock.
            break;
        }
        // If the CAS failed, expected was updated to the current value (1),
        // and the lock is still held by someone else. Loop again.
    }
}

void spinlock_unlock(spinlock_t *lock)
{
    // To release the lock, simply store the unlocked state (0).
    // This is safe because only the thread holding the lock should call this.
    atomic_store(lock, 0);

    // Optional: An explicit memory barrier could be added here if needed by your
    // system's memory model, but typically the atomic_store itself provides sufficient ordering.
    asm volatile("mfence" ::: "memory");
}

int spinlock_try_lock(spinlock_t *lock)
{
    int64_t expected = 0; // Expected unlocked state
    // Try once to swap 1 (locked) into the lock if it's currently 0 (unlocked)
    if (atomic_compare_exchange(lock, &expected, 1))
    {
        // Success! We acquired the lock.
        return 1;
    }
    // Failure! The lock was already held.
    return 0;
}
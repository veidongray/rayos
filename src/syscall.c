#include <vfs.h>
#include <stdint.h>
#include <stddef.h>

#define syscall(args, retval) \
    do                        \
    {                         \
        asm volatile(         \
            "int $0x80"       \
            : "=a"(retval)    \
            : "D"(args)       \
            : "memory");      \
    } while (0)

int open(const char *pathname, int flags, ...)
{
    uint64_t ret;
    uint64_t args[7];

    args[0] = SYS_OPEN;
    args[1] = pathname;

    syscall(args, ret);
}
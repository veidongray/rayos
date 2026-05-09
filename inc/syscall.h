#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#define SYSCALL_WRITE 0x1

#define syscall(alist, retv) \
    do                       \
    {                        \
        asm volatile(        \
            "int $0x80"      \
            : "=a"(retv)     \
            : "a"(alist));   \
    } while (0)

size_t sys_write(int fd, const char *buf, size_t len);

#endif // SYSCALL_H
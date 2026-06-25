#include <libc.h>
#include <stdint.h>
#include <stddef.h>

static inline int syscall(uint64_t nr,
                          uint64_t a1,
                          uint64_t a2,
                          uint64_t a3,
                          uint64_t a4)
{
    int ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(nr),
          "D"(a1),
          "S"(a2),
          "d"(a3),
          "c"(a4)
        : "memory", "cc");

    return ret;
}

int open(const char *pathname, int flags, ...)
{
    int ret;

    ret = syscall(SYS_OPEN, (uint64_t)pathname, flags, 0, 0);

    return ret;
}

int close(int fd)
{
    int ret;

    ret = syscall(SYS_CLOSE, fd, 0, 0, 0);

    return ret;
}

ssize_t read(int fd, void *buf, size_t count)
{
    int ret;

    ret = syscall(SYS_READ, fd, (uint64_t)buf, count, 0);

    return ret;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    int ret;

    ret = syscall(SYS_WRITE, fd, (uint64_t)buf, count, 0);

    return ret;
}

int creat(const char *pathname, mode_t mode)
{
    int ret;

    ret = syscall(SYS_CREATE, (uint64_t)pathname, mode, 0, 0);

    return ret;
}
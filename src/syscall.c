#include "syscall.h"
#include "panic.h"

size_t sys_write(int fd, const char *buf, size_t len)
{
    fd = fd;
    printk(buf);
    return len;
}

uint32_t syscall_handler(void *args)
{
    uint32_t *arg = (uint32_t *)args;

    switch (arg[0])
    {
    case SYSCALL_WRITE:
        return sys_write(arg[1], (char *)arg[2], arg[3]);
    default:
        PANIC("SYSCALL\n");
    }
    return 0;
}
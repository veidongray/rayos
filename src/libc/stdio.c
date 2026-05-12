#include "libc/stdio.h"
#include "syscall.h"
#include "libc/string.h"

int printf(const char *fmt, ...)
{
    uint32_t retval;
    uint32_t arg_list[6];

    arg_list[0] = SYSCALL_WRITE;
    arg_list[1] = STDOUT;
    arg_list[2] = (uint32_t)fmt;
    arg_list[3] = strlen(fmt);
    arg_list[4] = 0x0;
    arg_list[5] = 0x0;

    syscall(arg_list, retval);
    return retval;
}
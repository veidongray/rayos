#ifndef SYSCALL_H
#define SYSCALL_H

#include <sys/types.h>

int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int creat(const char *pathname, mode_t mode);

#endif // SYSCCALL_H
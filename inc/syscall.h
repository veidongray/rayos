#ifndef SYSCALL_H
#define SYSCALL_H

#include <sys/types.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int creat(const char *pathname, mode_t mode);

#endif // SYSCCALL_H
#ifndef LIBC_H
#define LIBC_H

#include <ff.h>
#include <sys/types.h>
#include <syscalls.h>
#include <vfs.h>

int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
void sync(void);

#endif // LIBC_H
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>
#include <sys/types.h>

int sys_open(const char *path, mode_t mode);
int sys_close(int fd);
int sys_read(int fd, char *buf, size_t size);
int sys_write(int fd, const char *buf, size_t size);
int sys_create(const char *pathname);
int sys_stat(const char *pathname, struct stat *_sb);

#endif // SYSCALLS_H
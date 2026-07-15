#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>

enum num_stdfd { STDIN, STDOUT, STDERR };

enum num_syscall {
	SYS_OPEN,
	SYS_CLOSE,
	SYS_READ,
	SYS_WRITE,
	SYS_STAT,
	SYS_SYNC
};

int sys_open(const char *path, mode_t mode);
int sys_close(int fd);
int sys_read(int fd, char *buf, size_t size);
int sys_write(int fd, const char *buf, size_t size);
int sys_stat(const char *pathname, struct stat *st);
void sys_sync(void);

#endif // SYSCALLS_H
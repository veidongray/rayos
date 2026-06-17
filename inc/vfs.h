#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>

enum num_syscall
{
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_CREATE,
    SYS_STAT
};

int vfs_init(void);
void vfs_task(void *arg);
int sys_open(const char *path);
int sys_close(int fd);
int sys_read(int fd, char *buf, size_t size);
int sys_write(int fd, const char *buf, size_t size);
int sys_create(const char *pathname);
int sys_stat(const char *pathname, struct stat *_sb);

#endif // VFS_H
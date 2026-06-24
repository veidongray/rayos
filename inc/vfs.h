#ifndef VFS_H
#define VFS_H

#include <list.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>

enum num_stdfd
{
    STDIN,
    STDOUT,
    STDERR
};

enum num_syscall
{
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_CREATE,
    SYS_STAT
};

struct vfs_file
{
    int fd;
    void *priv;
    struct list_head list;
};

int vfs_init(void);
int sys_open(const char *path, __mode_t mode);
int sys_close(int fd);
int sys_read(int fd, char *buf, size_t size);
int sys_write(int fd, const char *buf, size_t size);
int sys_create(const char *pathname);
int sys_stat(const char *pathname, struct stat *_sb);

#endif // VFS_H
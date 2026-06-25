#ifndef VFS_H
#define VFS_H

#include <fs.h>
#include <list.h>
#include <types.h>
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
int vfs_mount(char *dev_name, char *dir_name, char *fstype,
             unsigned long flags, void *data);
#endif // VFS_H
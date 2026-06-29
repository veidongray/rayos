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

struct vfsmount
{
    struct superblock *sb;
    struct dentry *root_dentry;
};

int vfs_init(void);

#endif // VFS_H
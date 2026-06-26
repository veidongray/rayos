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
    struct super_block *sb;
    struct dentry *root_dentry;
};

struct mount
{
    struct vfsmount mnt;
    struct list_head mnt_list;
};

int vfs_init(void);
int vfs_mount(char *dev_name, char *dir_name, char *fstype,
              unsigned long flags, void *data);
int vfs_open(const struct path *path, struct file *file);
struct file *dentry_open(struct path *path, int flags);
struct super_block *find_sb_mount_by_name(const char *name);
#endif // VFS_H
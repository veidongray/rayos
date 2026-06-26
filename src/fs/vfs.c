#include <ff.h>
#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <task.h>
#include <page.h>
#include <fat32.h>
#include <types.h>
#include <printk.h>
#include <string.h>
#include <atomic.h>
#include <mempool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <block_device.h>

#define NUM_FD_MAX 1024

static FATFS fs;
struct fd
{
    struct file *filp;
};
static struct fd *__fd;

static LIST_HEAD(mount_list);

static inline int find_unused_fd(void)
{
    for (int i = 0; i < NUM_FD_MAX; i++)
    {
        if (__fd[i].filp == NULL)
        {
            return i;
        }
    }
    return -1;
}

static inline int fd_install(int __fd_index, struct file *filp)
{
    if (__fd_index >= NUM_FD_MAX)
        return -1;

    __fd[__fd_index].filp = filp;
    return 0;
}

struct file *dentry_open(struct path *path, int flags)
{
    struct file *filp;
    struct dentry *dentry = path->dentry;

    filp = (struct file *)kzalloc(sizeof(struct file));

    filp->f_dentry = dentry;
    filp->f_path = path;

    return filp;
}

struct super_block *find_sb_mount_by_name(const char *name)
{
    struct mount *mnt;

    list_for_each_entry(mnt, &mount_list, mnt_list)
    {
        if (!strncmp(mnt->mnt.root_dentry->d_name, name, strlen(name)))
            return mnt->mnt.sb;
    }
    return NULL;
}

int vfs_init(void)
{
    int ret;

    __fd = (struct fd *)kzalloc(sizeof(struct fd) * NUM_FD_MAX);

    ret = vfs_mount("tmpfs", "/", "tmpfs", 0, NULL);
    if (!ret)
        pr_info("Mount %s to %s", "tmpfs", "/");

    f_mount(&fs, "", 1);
    return 0;
}

int vfs_mount(char *dev_name, char *dir_name, char *fstype,
              unsigned long flags, void *data)
{
    struct dentry *d;
    struct file_system_type *fs_type;

    if (!dev_name || !dir_name || !fstype)
        return -1;

    fs_type = fs_get_by_name(fstype);
    if (!fs_type)
        return -1;

    d = fs_type->mount(fs_type, fs_type->fs_flags, dev_name, NULL);
    if (!d)
        return -1;

    struct mount *mnt = (struct mount *)kzalloc(sizeof(struct mount));
    mnt->mnt.sb = d->d_inode->i_sb;
    mnt->mnt.root_dentry = d;
    list_add_tail(&mnt->mnt_list, &mount_list);
    return 0;
}

int vfs_open(const struct path *path, struct file *file)
{
    file->f_path = path;
    return file->f_dentry->d_inode->f_ops->open(file->f_dentry->d_inode, file);
}

ssize_t vfs_read(struct file *file, char *buf, size_t count, __loff_t *pos)
{
    ssize_t ret;

    ret = file->f_dentry->d_inode->f_ops->read(file, buf, count, pos);

    return ret;
}
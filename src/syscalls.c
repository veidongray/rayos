#include <ff.h>
#include <mm.h>
#include <vfs.h>
#include <syscalls.h>
#include <sys/types.h>

static int __g_fd_count = 0;
LIST_HEAD(__list_vfs_file);

int sys_open(const char *path, mode_t mode)
{
    FIL *fp = (FIL *)kzalloc(sizeof(FIL));
    struct vfs_file *file = (struct vfs_file *)kzalloc(sizeof(struct vfs_file));

    f_open(fp, path, mode);
    file->fd = __g_fd_count++;
    file->priv = (void *)fp;

    list_add_tail(&file->list, &__list_vfs_file);
    return file->fd;
}

int sys_close(int fd)
{
    struct list_head *pos;
    struct vfs_file *file;

    list_for_each(pos, &__list_vfs_file)
    {
        file = container_of(pos, struct vfs_file, list);
        if (file->fd == fd)
        {
            list_del(&file->list);
            f_close((FIL *)file->priv);
            kfree(file->priv);
            kfree(file);
            return 0;
        }
    }
    return -1;
}

int sys_read(int fd, char *buf, size_t size)
{
    int ret;
    struct list_head *pos;
    struct vfs_file *file;

    list_for_each(pos, &__list_vfs_file)
    {
        file = container_of(pos, struct vfs_file, list);
        if (file->fd == fd)
        {
            f_read((FIL *)file->priv, buf, (UINT)size, (UINT *)&ret);
            return ret;
        }
    }

    return -1;
}

int sys_write(int fd, const char *buf, size_t size)
{
    int ret;
    struct list_head *pos;
    struct vfs_file *file;

    list_for_each(pos, &__list_vfs_file)
    {
        file = container_of(pos, struct vfs_file, list);
        if (file->fd == fd)
        {
            f_write((FIL *)file->priv, buf, (UINT)size, (UINT *)&ret);
            return ret;
        }
    }

    return -1;
}

int sys_create(const char *pathname)
{
    FIL fp;
    f_open(&fp, pathname, FA_CREATE_NEW);
    f_sync(&fp);
    f_close(&fp);
    return 0;
}

int sys_stat(const char *pathname, struct stat *_sb)
{
    FILINFO info;

    f_stat(pathname, &info);
    _sb->st_size = info.fsize;

    return 0;
}
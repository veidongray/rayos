#include <vfs.h>
#include <fat32.h>
#include <printk.h>

int vfs_init(void)
{
}

void vfs_task(void *arg)
{
    printk("VFS running...\n");
    while (1)
        ;
}

int sys_open(const char *path)
{
    return fat32_open(path);
}

int sys_close(int fd)
{
    fat32_close(fd);
    return 0;
}

int sys_read(int fd, char *buf, size_t size)
{
    fat32_read(fd, buf, size);
    return 0;
}

int sys_write(int fd, const char *buf, size_t size)
{
    fat32_write(fd, buf, size);
    return 0;
}

int sys_create(const char *pathname)
{
    fat32_create(pathname);
    return 0;
}
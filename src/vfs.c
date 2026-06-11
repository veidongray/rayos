#include <fat32.h>
#include <printk.h>
#include <stdint.h>
#include <stddef.h>

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
    fat32_open(path);
    return 0;
}

int sys_read(int fd, char *buf, size_t size)
{
    fat32_read(fd, buf, size);
    return 0;
}
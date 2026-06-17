#include <mm.h>
#include <vfs.h>
#include <task.h>
#include <fat32.h>
#include <printk.h>
#include <sys/stat.h>
#include <string.h>

int vfs_init(void)
{
    return 0;
}

void vfs_task(void *arg)
{
    arg = arg;
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
    return fat32_close(fd);
}

int sys_read(int fd, char *buf, size_t size)
{
    int ret;

    ret = fat32_read(fd, buf, size);

    return ret;
}

int sys_write(int fd, const char *buf, size_t size)
{
    int ret;

    ret = fat32_write(fd, buf, size);

    return ret;
}

int sys_create(const char *pathname)
{
    fat32_create(pathname);
    return 0;
}

int sys_stat(const char *pathname, struct stat *_sb)
{
    struct stat sb;
    struct fat32_dir_entry entry;

    fat32_lookup(pathname, &entry);

    sb.st_size = entry.sfn_entry.file_size;

    memcpy(_sb, &sb, sizeof(struct stat));

    return 0;
}
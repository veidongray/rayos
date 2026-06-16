#include <mm.h>
#include <vfs.h>
#include <task.h>
#include <fat32.h>
#include <printk.h>
#include <sys/stat.h>
#include <lib/string/string.h>

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
    fat32_close(fd);
    return 0;
}

int sys_read(int fd, char *buf, size_t size)
{
    int ret;
    char *_buf = kzalloc(size);

    ret = fat32_read(fd, _buf, size);
    memcpy(buf, _buf, size);

    kfree(_buf);
    return ret;
}

int sys_write(int fd, const char *buf, size_t size)
{
    int ret;
    char *_buf = kzalloc(size);

    memcpy(_buf, buf, size);
    ret = fat32_write(fd, _buf, size);

    kfree(_buf);
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
#include <ff.h>
#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <task.h>
#include <fat32.h>
#include <types.h>
#include <printk.h>
#include <string.h>
#include <atomic.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <block_device.h>

static FATFS fs;

int vfs_init(void)
{
    int ret;

    ret = vfs_mount("tmpfs", "/", "tmpfs", 0, NULL);
    if (!ret)
        printk("Mount %s to %s\n", "tmpfs", "/");

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

    return 0;
}
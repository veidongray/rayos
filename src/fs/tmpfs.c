#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <init.h>
#include <string.h>
#include <printk.h>

struct file_system_type tmpfs_type = {
    .name = "tmpfs",
};

static int tmpfs_init(void)
{
    printk("tmpfs_init");
    return register_filesystem(&tmpfs_type);
}
module_init(tmpfs_init);

static void tmpfs_exit(void)
{
    printk("tmpfs_exit");
    unregister_filesystem(&tmpfs_type);
}
module_exit(tmpfs_exit);
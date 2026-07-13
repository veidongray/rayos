#include <fs.h>
#include <init.h>
#include <mm.h>
#include <printk.h>
#include <string.h>
#include <vfs.h>

/**
 * @brief 调用对应的 VFS 层的 mount 函数
 *
 * @param fstype
 * @param flags
 * @param dev_name
 * @param data
 * @return struct dentry*
 */
static int tmpfs_mount(struct file_system_type *fstype, const char *dev_name)
{
	return 0;
}

static struct file_system_type tmpfs_type = {
        .name = "tmpfs",
        .mount = tmpfs_mount,
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
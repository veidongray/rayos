#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <init.h>
#include <string.h>
#include <printk.h>

struct tmpfs_inode_info
{
    int ino;
    int name_len;
    char name[FS_NAME_MAX];
};

/**
 * @brief 在 dir 指定的路径下查找 dentry
 *
 * @param dir - 在指定的路径下查找
 * @param dentry - 待查找的 dentry
 * @return int - 成功返回 0
 */
int tmpfs_lookup(struct inode *dir, struct dentry *dentry)
{
    return 0;
}

struct inode_operations iops = {
    .lookup = tmpfs_lookup,
};

/**
 * @brief Alloc a new inode
 * tmpfs 的每一次 inode 分配都走这个调用
 *
 * @param sb - 该文件系统对应的 super_block
 * @return struct inode*
 */
struct inode *tmpfs_alloc_inode(struct super_block *sb)
{
    struct inode *inode;

    inode = (struct inode *)kzalloc(sizeof(struct inode));
    if (!inode)
    {
        return NULL;
    }

    inode->i_ino = 0;
    inode->i_ops = &iops;
    inode->i_sb = sb;

    return inode;
}

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
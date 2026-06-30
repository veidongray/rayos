#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <init.h>
#include <string.h>
#include <printk.h>

struct tmpfs_sb_info
{
    unsigned long next_ino;
    struct list_head inode_list; /* 全局 inode 索引链表 */
};

struct tmpfs_inode_info
{
    struct inode vfs_inode;
    struct list_head inode_link;   /* 挂到 sb_info->inode_list */
    struct list_head list_subdirs; /* 目录子项链表头 */
    struct tmpfs_inode_info *parent;
};

struct tmpfs_dirent
{
    unsigned long ino;
    unsigned int name_len;
    char name[FS_NAME_MAX + 1];
    struct list_head sibling;
};

static struct tmpfs_sb_info ts_info;

static struct inode *tmpfs_iget(struct super_block *sb, unsigned long ino)
{
    struct tmpfs_sb_info *ts_info = (struct tmpfs_sb_info *)sb->s_fs_info;
    struct tmpfs_inode_info *ti_info;

    list_for_each_entry(ti_info, &ts_info->inode_list, inode_link)
    {
        if (ti_info->vfs_inode.i_ino == ino)
        {
            return &ti_info->vfs_inode;
        }
    }

    return NULL;
}

/**
 * @brief VFS lookup 回调：根据文件名在目录中查找并绑定 inode
 *
 * 当 VFS dentry cache 未命中时调用此函数。
 * 从 dir 对应的 tmpfs 私有 dirent 链表中按 dentry->d_name 查找匹配项：
 *   - 找到：获取对应 inode，调用 d_add(dentry, inode)
 *   - 未找到：调用 d_add(dentry, NULL) 创建负 dentry（合法情况，非错误）
 *   - 异常：返回负错误码，不调用 d_add
 *
 * @param dir     父目录的 inode（通过 container_of 获取 tmpfs_inode_info）
 * @param dentry  VFS 预分配的空壳 dentry，d_name 包含待查找的文件名
 * @return struct dentry * 非空指针表示查找成功，NULL表示查找失败
 */
static struct dentry *tmpfs_lookup(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode;
    struct tmpfs_dirent *de;
    struct tmpfs_inode_info *parent;

    parent = container_of(dir, struct tmpfs_inode_info, vfs_inode);

    list_for_each_entry(de, &parent->list_subdirs, sibling)
    {
        if ((de->name_len == dentry->d_namelen) && (!memcmp(de->name, dentry->d_name, de->name_len)))
        {
            inode = tmpfs_iget(dir->i_sb, de->ino);
            if (!inode)
            {
                return NULL;
            }
            dentry->d_inode = inode;
            return dentry;
        }
    }
    return NULL;
}

static struct inode_operations inode_ops = {
    .lookup = tmpfs_lookup,
};

/**
 * @brief VFS alloc_inode 回调：为 tmpfs 分配一个新的 VFS inode
 *
 * 由 VFS 在 iget/new_inode 等路径中调用，是 tmpfs 所有 inode 创建的唯一入口。
 * 注意：返回的 inode 仅完成内存分配和基础字段设置，
 *       i_mode/i_size/i_op 等语义字段需在调用方或 init_inode 中进一步填充。
 *
 * @param sb 该文件系统对应的 super_block
 * @return struct inode* 成功返回新分配的 inode 指针，失败返回 NULL
 */
static struct inode *tmpfs_alloc_inode(struct super_block *sb)
{
    struct inode *inode;
    struct tmpfs_inode_info *ti_info;

    ti_info = (struct tmpfs_inode_info *)kzalloc(sizeof(struct tmpfs_inode_info));
    if (!ti_info)
    {
        return NULL;
    }

    ti_info->parent = NULL;

    /* 初始化子目录项列表头 */
    INIT_LIST_HEAD(&ti_info->list_subdirs);

    /* 将新的 inode 加入全局 inode 列表 */
    list_add_tail(&ti_info->inode_link, &ts_info.inode_list);

    inode = &ti_info->vfs_inode;
    inode->i_ino = ts_info.next_ino++;
    inode->i_ops = &inode_ops;
    inode->i_sb = sb;

    return inode;
}

static struct super_operations sb_ops = {
    .alloc_inode = tmpfs_alloc_inode,
};

static struct file_system_type tmpfs_type = {
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
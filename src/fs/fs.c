/*
 * ┌──────────────────────────────────────────────────────┐
 * │                  super_block                         │
 * │  (文件系统全局信息: 类型、块大小、操作函数集)           │
 * │                                                      │
 * │   s_root ──────────► dentry (根目录 "/")              │
 * │                          │                           │
 * │                     d_child / d_subdirs              │
 * │                          │                           │
 * │                          ▼                           │
 * │                      dentry ("home")                 │
 * │                     /        \                       │
 * │              d_child          d_child                │
 * │               /                   \                  │
 * │              ▼                     ▼                 │
 * │         dentry ("user")       dentry ("etc")         │
 * │              │                     │                 │
 * │              │ d_inode             │ d_inode         │
 * │              ▼                     ▼                 │
 * │           inode                 inode                │
 * │      (用户目录元数据)          (etc目录元数据)          │
 * │              ▲                     ▲                 │
 * │              │ f_inode             │                 │
 * │              │                     │                 │
 * │           file ◄──(open)───────────┘                 │
 * │    (fd, offset, f_op)                                │
 * └──────────────────────────────────────────────────────┘
 */

#include <fs.h>
#include <mm.h>
#include <list.h>
#include <string.h>
#include <printk.h>

static LIST_HEAD(fs_type_list);
static LIST_HEAD(super_block_list);

int register_filesystem(struct file_system_type *fs)
{
    fs->nm_len = strlen(fs->name);
    list_add_tail(&fs->fs_list, &fs_type_list);
    return 0;
}
void unregister_filesystem(struct file_system_type *fs)
{
    list_del(&fs->fs_list);
}

struct file_system_type *fs_get_by_name(const char *name)
{
    struct file_system_type *fs_type;

    if (list_empty(&fs_type_list))
        return NULL;

    list_for_each_entry(fs_type, &fs_type_list, fs_list)
    {
        if (!strncmp(fs_type->name, name, fs_type->nm_len))
            return fs_type;
    }

    return NULL;
}

struct dentry *mount_nodev(struct file_system_type *fs_type,
                           int flags, void *data,
                           int (*fill_super)(struct super_block *, void *, int))
{
    int ret;
    struct super_block *sb;

    sb = (struct super_block *)kzalloc(sizeof(struct super_block));
    if (!sb)
        return NULL;

    sb->s_type = fs_type;
    ret = fill_super(sb, NULL, 0);
    if (ret < 0)
    {
        kfree(sb);
        return NULL;
    }

    return sb->s_root;
}

int super_block_add(struct super_block *sb)
{
    list_add_tail(&sb->s_list, &super_block_list);
    return 0;
}
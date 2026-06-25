#ifndef FS_H
#define FS_H

#include <list.h>
#include <types.h>
#include <atomic.h>
#include <stdbool.h>
#include <sys/types.h>

#define FS_NAME_MAX 255

struct inode;
struct dentry;
struct file;
struct super_block;

struct super_operations
{
    struct inode *(*alloc_inode)(struct super_block *);
    void (*destroy_inode)(struct inode *);
};

struct inode_operations
{
    struct dentry *(*lookup)(struct inode *dir, struct dentry *dentry);
    int (*create)(struct inode *dir, struct dentry *dentry, mode_t mode);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, mode_t mode);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
};

struct file_operations
{
    ssize_t (*read)(struct file *filp, char *buf, size_t count, __loff_t *ppos);
    ssize_t (*write)(struct file *filp, const char *buf, size_t count, __loff_t *ppos);
    __loff_t (*llseek)(struct file *filp, __loff_t offset, int whence);
    int (*readdir)(struct file *filp, void *dirent,
                   int (*filldir)(void *, const char *, int, __loff_t, ino_t));
    int (*release)(struct file *filp);
};

struct dentry_operations
{
    int (*d_compare)(const struct dentry *dentry, const char *name, int len);
};

struct super_block
{
    uint32_t s_magic;     // 文件系统魔数
    uint32_t s_blocksize; // 逻辑块大小

    struct dentry *s_root; // 根目录项（挂载后设置）

    const struct super_operations *s_ops;

    void *s_fs_info; // tmpfs_sb_info / ext2_sb_info

    struct list_head s_list; // 全局 sb 链表节点
};

struct inode
{
    ino_t i_ino;  // inode 编号
    __u64 i_size; // 文件大小（字节）

    struct super_block *i_sb; // 所属超级块

    const struct inode_operations *i_ops; // 目录/符号链接操作
    const struct file_operations *f_ops;  // 读写操作（注意：存在inode上而非file上）

    void *i_private; // tmpfs_inode / ext2_inode_info
};

struct dentry
{
    char d_name[FS_NAME_MAX + 1]; // 文件名分量（不含路径分隔符）

    struct inode *d_inode;   // 关联inode（NULL = 负dentry/未解析）
    struct dentry *d_parent; // 父目录项

    struct list_head d_child;   // 在父目录 d_subdirs 链表中的节点
    struct list_head d_subdirs; // 子目录项链表头

    const struct dentry_operations *d_ops;
};

struct file
{
    struct dentry *f_dentry; // 关联的目录项
    void *private_data;      // 驱动/FS私有运行时数据
};

struct file_system_type
{
    const char *name;
    int fs_flags;
    struct list_head fs_list;
    struct dentry *(*mount)(struct file_system_type *fs_type, int flags,
                            const char *dev_name, void *data);
    void (*kill_sb)(struct super_block *);
};

int register_filesystem(struct file_system_type *fs);
void unregister_filesystem(struct file_system_type *fs);

#endif // FS_H
#ifndef FS_H
#define FS_H

#include <list.h>
#include <types.h>
#include <atomic.h>
#include <stdbool.h>
#include <sys/types.h>

#define FS_NAME_MAX 255

struct file;
struct inode;
struct dentry;
struct super_block;
struct file_system_type;

struct super_operations
{
    struct inode *(*alloc_inode)(struct super_block *sb);
};

struct inode_operations
{
    struct dentry *(*lookup)(struct inode *dir, struct dentry *dentry);
};

struct file_operations
{
    int (*open)(struct inode *inode, struct file *filp);
};

struct dentry_operations
{
    int (*d_compare)(const struct dentry *dentry, const char *cmp_name);
};

struct super_block
{
    void *s_fs_info;

    struct dentry *s_root;
    struct super_operations *s_ops;
    struct file_system_type *s_type;
};

struct inode
{
    unsigned long i_ino;
    struct super_block *i_sb;
    struct file_operations *i_fops;
    struct inode_operations *i_ops;
};

struct dentry
{
    char *d_name;
    unsigned int d_namelen;

    struct inode *d_inode;
    struct dentry *d_parent;
    struct super_block *d_sb;

    struct list_head d_child;
    struct list_head d_subdirs;

    struct dentry_operations *d_ops;
};

struct file
{
    char *f_name;
    unsigned long f_namelen;

    struct inode *f_inode;
    struct file_operations *f_ops;
};

struct file_system_type
{
    const char *name;
    int nm_len;

    struct dentry *(*mount)(struct file_system_type *fstype, int flags,
                            const char *dev_name, void *data);
    struct list_head fs_list;
};

struct mount
{
    struct dentry *root;
    struct super_block *sb;
    struct file_system_type *fs_type;

    struct list_head mnt_list;
};

int register_filesystem(struct file_system_type *fs);
void unregister_filesystem(struct file_system_type *fs);
struct file_system_type *fs_get_by_name(const char *name);

#endif /* FS_H */
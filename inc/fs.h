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
struct superblock;

struct super_operations
{
    struct inode *(*alloc_inode)(struct superblock *);
    void (*destroy_inode)(struct inode *);
};

struct inode_operations
{
};

struct file_operations
{
};

struct dentry_operations
{
};

struct superblock
{
};

struct inode
{
};

struct dentry
{
};

struct file
{
};

struct file_system_type
{
    const char *name;
    int nm_len;
    struct list_head fs_list;
};

struct mount
{
    struct dentry *root;
    struct superblock *sb;
    struct file_system_type *fs_type;

    struct list_head mnt_list;
};

int register_filesystem(struct file_system_type *fs);
void unregister_filesystem(struct file_system_type *fs);
struct file_system_type *fs_get_by_name(const char *name);

#endif // FS_H
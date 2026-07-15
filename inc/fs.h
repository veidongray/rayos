#ifndef FS_H
#define FS_H

#include <atomic.h>
#include <list.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>

#define FS_NAME_MAX 255

struct file_system_type;

struct dentry {
	int pathlen;
	char *pathname;

	struct mount *mnt; // 指向对应的 mount
	struct list_head list;

	atomic_int_t d_ref; // 引用计数

	void *private_data;
};

struct file {
	__u32 fd;

	struct dentry *dentry; // 指向对应的 dentry
	struct list_head list;

	void *private_data;
};

struct file_system_operations {
	int (*mount)(struct file_system_type *fstype, const char *path);
	struct dentry *(*lookup)(struct dentry *dentry, const char *path);
	int (*open)(struct file *filp, mode_t mode);
	int (*release)(struct file *filp);
	int (*creat)(const char *path);
	int (*mkdir)(const char *path);
	int (*unlink)(const char *path);
	int (*rename)(const char *oldpath, const char *newpath);
	ssize_t (*read)(struct dentry *dentry, void *buf, size_t len);
	ssize_t (*write)(struct dentry *dentry, const void *buf, size_t len);
	int (*readdir)(const char *path);
	int (*stat)(struct dentry *dentry, struct stat *st);
	void (*sync)(struct dentry *dentry);
};

struct file_system_type {
	const char *fs_name;
	int fs_nmlen;

	struct file_system_operations *fs_ops;

	struct list_head fs_list;

	void *private_data;
};

int register_filesystem(struct file_system_type *fs);
void unregister_filesystem(struct file_system_type *fs);
struct file_system_type *fs_get_by_name(const char *name);

#endif /* FS_H */

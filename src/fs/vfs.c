/*
 * ┌──────────────────────────────────────────────────────┐
 * │                  super_block                         │
 * │  (文件系统全局信息: 类型、块大小、操作函数集)            │
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
 * │      (用户目录元数据)          (etc目录元数据)         │
 * │              ▲                     ▲                 │
 * │              │ f_inode             │                 │
 * │              │                     │                 │
 * │           file ◄──(open)───────────┘                 │
 * │    (fd, offset, f_op)                                │
 * └──────────────────────────────────────────────────────┘
 */

#include <ff.h>
#include <fs.h>
#include <mount.h>
#include <printk.h>
#include <string.h>
#include <sys/stat.h>
#include <vfs.h>

static LIST_HEAD(dentry_list);

void vfs_init(void)
{
	/**
	 * @brief cmdline
	 *
	 * 后期加入 cmdline 支持
	 *
	 */
	const char *root_path = "";
	const char *root_fstype = "fatfs";

	struct file_system_type *fstype;
	fstype = fs_get_by_name(root_fstype);
	fstype->fs_ops->mount(fstype, root_path);
}

int vfs_open(const char *path, mode_t mode)
{
	int ret = -1;
	struct mount *mnt;
	struct dentry *dentry;
	struct file_system_type *type;

	mnt = mount_get_by_name(path);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	if (type->fs_ops->lookup) {
		dentry = type->fs_ops->lookup(path + strlen(mnt->mnt_path));
		if (!dentry) {
			return ret;
		}
	} else {
		return -ENODEV;
	}

	dentry->pathname = (char *)path;
	dentry->pathlen = strlen(path);
	list_add_tail(&dentry->list, &dentry_list);

	if (type->fs_ops->open) {
		ret = type->fs_ops->open(dentry, mode);
		return ret;
	} else {
		return -ENODEV;
	}
}

int vfs_close(const char *path)
{
	int ret = -1;
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(path);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(path, pos->pathname)) {
			if (type->fs_ops->release) {
				ret = type->fs_ops->release(pos);
				if (ret >= 0) {
					list_del(&pos->list);
				}
				return ret;
			}
			return -ENODEV;
		}
	}
	return -ENOENT;
}

int vfs_read(const char *path, void *buf, size_t len)
{
	int ret = -1;
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(path);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(path, pos->pathname)) {
			if (type->fs_ops->read) {
				ret = type->fs_ops->read(pos, buf, len);
				return ret;
			}
			return -ENODEV;
		}
	}

	return -ENOENT;
}

int vfs_write(const char *path, const void *buf, size_t len)
{
	int ret = -1;
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(path);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(path, pos->pathname)) {
			if (type->fs_ops->write) {
				ret = type->fs_ops->write(pos, buf, len);
				return ret;
			}
			return -ENODEV;
		}
	}

	return -ENOENT;
}

int vfs_stat(const char *path, struct stat *st)
{
	int ret = -1;
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(path);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(path, pos->pathname)) {
			if (type->fs_ops->stat) {
				ret = type->fs_ops->stat(pos, st);
				return ret;
			}
			return -ENODEV;
		}
	}

	return -ENOENT;
}

void vfs_sync(const char *pathname)
{
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(pathname);
	if (mnt == NULL) {
		return;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(pathname, pos->pathname)) {
			if (type->fs_ops->sync) {
				type->fs_ops->sync(pos);
			}
		}
	}
}
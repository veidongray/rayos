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
#include <mm.h>
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

int vfs_open(struct file *filp, const char *path, mode_t mode)
{
	struct mount *mount = mount_get_by_name(path);
	if (!mount) {
		return -ENODEV;
	}

	char *fs_path =
	        (char *)path + strlen(mount->mnt_path); // 文件系统内部的路径
	struct file_system_type *type = mount->mnt_fstype;

	/*
	 * 尝试查找 dentry 列表确定是否已经存在 dentry
	 * 已经存在直接设置 filp->dentry = dentry 并返回
	 */
	struct dentry *dentry;
	list_for_each_entry(dentry, &dentry_list, list)
	{
		if (!strcmp(dentry->pathname, path)) {
			filp->dentry = dentry;
			atomic_fetch_add(&dentry->d_ref, 1);
			return 0;
		}
	}

	// 没找到就创建新的 dentry
	dentry = kzalloc(sizeof(struct dentry));
	if (!dentry) {
		return -ENOMEM;
	}

	filp->dentry = dentry;
	if (type->fs_ops->lookup) {
		if (!type->fs_ops->lookup(filp->dentry, fs_path)) {
			kfree(dentry);
			return -ENOENT;
		}
	} else {
		kfree(dentry);
		return -ENODEV;
	}

	type->fs_ops->open(filp, mode);

	// 填充 dentry 剩余部分
	dentry->pathname = (char *)path;
	dentry->pathlen = strlen(dentry->pathname) + 1; // 加上末尾 '\0' 的长度
	dentry->mnt = mount;
	atomic_store(&dentry->d_ref, 1);
	list_add_tail(&dentry->list, &dentry_list);

	return 0;
}

int vfs_close(struct file *filp)
{
	struct dentry *pos;
	struct dentry *dentry = filp->dentry;
	struct mount *mount = mount_get_by_name(dentry->pathname);
	if (!mount) {
		return -ENODEV;
	}
	struct file_system_type *type = mount->mnt_fstype;

	// 尝试在 dentry 列表中查找
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(pos->pathname, dentry->pathname)) {
			atomic_fetch_sub(&dentry->d_ref, 1);
			if (!atomic_load(&dentry->d_ref)) {
				type->fs_ops->release(filp);
				list_del(&dentry->list);
				kfree(dentry->pathname);
				kfree(dentry);
			}
			return 0;
		}
	}

	return -ESTALE;
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
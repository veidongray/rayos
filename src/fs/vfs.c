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

	// 新建 buff 内存区不再操作传递进来的地址
	char *pathbuff = kzalloc(strlen(path) + 1);
	if (!pathbuff) {
		kfree(filp);
		return -ENOMEM;
	}
	strcpy(pathbuff, path);

	struct file_system_type *type = mount->mnt_fstype;

	/*
	 * 尝试查找 dentry 列表确定是否已经存在 dentry
	 * 已经存在直接设置 filp->dentry = dentry 并返回
	 */
	struct dentry *dentry;
	list_for_each_entry(dentry, &dentry_list, list)
	{
		if (!strcmp(dentry->pathname, pathbuff)) {
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
		// 查找是否存在路径并在路径存在时填充 dentry->private_data
		if (!type->fs_ops->lookup(filp->dentry, pathbuff)) {
			kfree(dentry);
			return -ENOENT;
		}
	} else {
		kfree(dentry);
		return -ENODEV;
	}

	// 填充 dentry 剩余部分
	dentry->pathname = (char *)pathbuff;
	dentry->pathlen = strlen(dentry->pathname) + 1; // 加上末尾 '\0' 的长度
	dentry->mnt = mount;

	// 此时 filp 填充基本完整
	// 进入 fsops 的 open 填充剩余部分
	// 或者根据具体实现填充或不填充需要的部分
	int ret = type->fs_ops->open(filp, mode);
	if (ret < 0) {
		kfree(pathbuff);
		kfree(dentry);
		return ret;
	}

	atomic_store(&dentry->d_ref, 1);
	list_add_tail(&dentry->list, &dentry_list);

	return 0;
}

int vfs_close(struct file *filp)
{
	struct dentry *dentry = filp->dentry;
	struct mount *mount = mount_get_by_name(dentry->pathname);
	if (!mount) {
		return -ENODEV;
	}
	struct file_system_type *type = mount->mnt_fstype;

	// 尝试在 dentry 列表中查找
	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(pos->pathname, dentry->pathname)) {
			atomic_fetch_sub(&dentry->d_ref, 1);

			// 检查 dentry 引用是否为 0
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

int vfs_read(struct file *filp, void *buf, size_t len)
{
	struct dentry *dentry = filp->dentry;
	struct mount *mount = mount_get_by_name(dentry->pathname);
	if (!mount) {
		return -ENODEV;
	}
	struct file_system_type *type = mount->mnt_fstype;

	if (type->fs_ops->read) {
		return type->fs_ops->read(filp, buf, len);
	} else {
		return -ENODEV;
	}
}

int vfs_write(struct file *filp, const void *buf, size_t len)
{
	struct dentry *dentry = filp->dentry;
	struct mount *mount = mount_get_by_name(dentry->pathname);
	if (!mount) {
		return -ENODEV;
	}
	struct file_system_type *type = mount->mnt_fstype;

	if (type->fs_ops->write) {
		return type->fs_ops->write(filp, buf, len);
	} else {
		return -ENODEV;
	}
}

int vfs_stat(const char *pathname, struct stat *st)
{
	int ret = -1;
	struct mount *mnt;
	struct file_system_type *type;

	mnt = mount_get_by_name(pathname);
	if (mnt == NULL) {
		return ret;
	}
	type = mnt->mnt_fstype;

	if (type->fs_ops->stat) {
		ret = type->fs_ops->stat(pathname, st);
		return ret;
	}

	return -ENODEV;
}

void vfs_sync(struct file *filp)
{
	struct mount *mnt;
	struct dentry *dentry;
	struct file_system_type *type;

	dentry = filp->dentry;
	mnt = mount_get_by_name(dentry->pathname);
	if (mnt == NULL) {
		return;
	}
	type = mnt->mnt_fstype;

	struct dentry *pos;
	list_for_each_entry(pos, &dentry_list, list)
	{
		if (!strcmp(dentry->pathname, pos->pathname)) {
			if (type->fs_ops->sync) {
				type->fs_ops->sync(pos);
			}
		}
	}
}

int vfs_creat(const char *pathname, mode_t mode)
{
	struct mount *mount = mount_get_by_name(pathname);
	if (!mount) {
		return -ENODEV;
	}

	struct file_system_type *fstype = mount->mnt_fstype;
	return fstype->fs_ops->creat(pathname, mode);
}
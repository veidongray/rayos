#include <ff.h>
#include <mm.h>
#include <printk.h>
#include <string.h>
#include <sys/types.h>
#include <syscalls.h>
#include <vfs.h>

static __u32 fd_count = 0;
static LIST_HEAD(file_list);

int sys_open(const char *path, mode_t mode)
{
	int ret;
	struct file *filp;

	// 创建空 file
	filp = kzalloc(sizeof(struct file));
	if (!filp) {
		pr_err("MEM");
		return -ENOMEM;
	}

	ret = vfs_open(filp, path, mode);
	if (ret < 0) {
		kfree(filp);
		return ret;
	}

	filp->fd = fd_count++;
	list_add_tail(&filp->list, &file_list);

	return filp->fd;
}

int sys_close(int fd)
{
	struct file *filp;
	list_for_each_entry(filp, &file_list, list)
	{
		// 查找是否有对应的 filp
		if (filp->fd == (__u32)fd) {
			int ret = vfs_close(filp);
			if (ret < 0) {
				return ret;
			}
			list_del(&filp->list);
			kfree(filp);
			return 0;
		}
	}

	return -EBADF;
}

int sys_read(int fd, char *buf, size_t size)
{
	int ret;
	struct file *filp;

	list_for_each_entry(filp, &file_list, list)
	{
		if (filp->fd == (__u32)fd) {
			ret = vfs_read(filp, buf, size);
			return ret;
		}
	}

	return -ENOENT;
}

int sys_write(int fd, const char *buf, size_t size)
{
	int ret;
	struct file *filp;

	list_for_each_entry(filp, &file_list, list)
	{
		if (filp->fd == (__u32)fd) {
			ret = vfs_write(filp, buf, size);
			return ret;
		}
	}

	return -ENOENT;
}

int sys_stat(const char *pathname, struct stat *st)
{
	return vfs_stat(pathname, st);
}

void sys_sync(void)
{
	struct file *filp;
	list_for_each_entry(filp, &file_list, list) { vfs_sync(filp); }
}

int sys_creat(const char *pathname, mode_t mode)
{
	int ret;
	char *path = kzalloc(strlen(pathname) + 1);
	if (!path) {
		return -ENOMEM;
	}
	strcpy(path, pathname);

	ret = vfs_creat(pathname, mode);
	kfree(path);
	return ret;
}
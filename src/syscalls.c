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

	ret = vfs_open(path, mode);
	if (ret < 0) {
		return ret;
	}

	filp = kzalloc(sizeof(struct file));
	if (!filp) {
		vfs_close(path);
		return -ENOMEM;
	}

	filp->fd = fd_count++;
	filp->pathname = (char *)path;
	filp->pathlen = strlen(path);
	list_add_tail(&filp->list, &file_list);

	return filp->fd;
}

int sys_close(int fd)
{
	int ret;
	struct file *filp;

	list_for_each_entry(filp, &file_list, list)
	{
		if (filp->fd == (__u32)fd) {
			ret = vfs_close(filp->pathname);
			return ret;
		}
	}

	return -ENOENT;
}

int sys_read(int fd, char *buf, size_t size)
{
	int ret;
	struct file *filp;

	list_for_each_entry(filp, &file_list, list)
	{
		if (filp->fd == (__u32)fd) {
			ret = vfs_read(filp->pathname, buf, size);
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
			ret = vfs_write(filp->pathname, buf, size);
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
	list_for_each_entry(filp, &file_list, list)
	{
		vfs_sync(filp->pathname);
	}
}
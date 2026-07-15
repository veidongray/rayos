#include <ff.h>
#include <fs.h>
#include <init.h>
#include <mm.h>
#include <mount.h>
#include <printk.h>
#include <string.h>

enum dentry_type {
	DENTRY_FILE,
	DENTRY_DIR,
};

struct fatfs_dentry {
	char *name;
	int nmlen;
	FIL *fp;
	void *data;
};

static FATFS fatfs_root;

static struct dentry *fatfs_lookup(struct dentry *dentry, const char *path)
{
	FILINFO fno;
	FRESULT res;
	struct fatfs_dentry *fd;

	res = f_stat(path, &fno);
	if (res == FR_OK) {
		fd = kzalloc(sizeof(struct fatfs_dentry));
		if (!fd) {
			return NULL;
		}

		fd->name = (char *)path;
		fd->nmlen = strlen(path);

		dentry->private_data = fd;

		return dentry;
	} else {
		return NULL;
	}
}

static int fatfs_creat(const char *path)
{
	FIL fp;
	FRESULT res;

	res = f_open(&fp, path, FA_CREATE_NEW);
	if (res != FR_OK) {
		pr_err("Can't open %s", path);
		return res;
	}

	do {
		res = f_sync(&fp);
	} while (res != FR_OK);

	res = f_close(&fp);
	if (res != FR_OK) {
		pr_err("Can't close %s", path);
		return res;
	}
	return 0;
}

static int fatfs_open(struct file *filp, mode_t mode)
{
	FIL *fp;
	FRESULT res;
	struct fatfs_dentry *fd;
	struct dentry *dentry = filp->dentry;
	fd = (struct fatfs_dentry *)dentry->private_data;

	fp = kzalloc(sizeof(FIL));
	if (fp == NULL) {
		return -1;
	}

	res = f_open(fp, fd->name, mode);
	if (res != FR_OK) {
		kfree(fp);
		return -1;
	}

	fd->data = fp;
	return 0;
}

static int fatfs_release(struct file *filp)
{
	struct dentry *dentry = filp->dentry;
	struct fatfs_dentry *fd = (struct fatfs_dentry *)dentry->private_data;

	FIL *fp = fd->data;
	f_close(fp);
	kfree(fp);
	return 0;
}

static ssize_t fatfs_read(struct file *filp, void *buf, size_t len)
{
	FIL *fp;
	UINT ret;
	FRESULT res;
	struct dentry *dentry = filp->dentry;
	struct fatfs_dentry *fd = (struct fatfs_dentry *)dentry->private_data;

	fp = fd->data;
	res = f_read(fp, buf, (UINT)len, &ret);
	if (res != FR_OK) {
		return res;
	}
	return ret;
}

static ssize_t fatfs_write(struct file *filp, const void *buf, size_t len)
{
	FIL *fp;
	UINT ret;
	FRESULT res;
	struct dentry *dentry = filp->dentry;
	struct fatfs_dentry *fd = (struct fatfs_dentry *)dentry->private_data;

	fp = fd->data;
	res = f_write(fp, buf, (UINT)len, &ret);
	if (res != FR_OK) {
		return res;
	}
	return ret;
}

static int fatfs_readdir(const char *path)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;

	res = f_opendir(&dir, path);
	if (res != FR_OK) {
		pr_err("Cant open dir %s", path);
		return res;
	}

	for (;;) {
		res = f_readdir(&dir, &fno);

		// 读取出错 或 已读到目录末尾
		if (res != FR_OK || fno.fname[0] == '\0')
			break;

		// 判断类型并处理
		if (fno.fattrib & AM_DIR) {
			pr_info("[DIR]  %s", fno.fname);
		} else {
			pr_info("[FILE] %-12s  %lu bytes", fno.fname,
			        fno.fsize);
		}
	}

	f_closedir(&dir);
	return res;
}

/* 删除文件或空目录 */
static int fatfs_unlink(const char *path)
{
	FRESULT res = f_unlink(path);

	switch (res) {
	case FR_OK:
		break;

	case FR_DENIED:
		break;

	case FR_NO_FILE:
	case FR_NO_PATH:
		break;

	default:
		break;
	}

	return res;
}

/* 显式创建目录（支持多级自动创建中间目录） */
static int fatfs_mkdir(const char *path)
{
	FRESULT res;
	char work_path[FF_MAX_LFN + 1];
	strncpy(work_path, path, sizeof(work_path) - 1);
	work_path[sizeof(work_path) - 1] = '\0';

	// 从根开始逐级尝试创建
	char *p = work_path;

	// 跳过盘符前缀 "0:/"
	if (p[1] == ':' && p[2] == '/')
		p += 3;
	else if (p[0] == '/')
		p += 1;

	for (;;) {
		// 找到下一级分隔符
		char *slash = strchr(p, '/');
		if (slash)
			*slash = '\0'; // 临时截断

		// 尝试创建当前层级
		res = f_mkdir(work_path);

		// FR_EXIST 是正常情况，继续往下走
		if (res != FR_OK && res != FR_EXIST)
			return res;

		// 如果没有更多层级了，说明全部创建完成
		if (!slash)
			break;

		// 恢复分隔符，移动到下一级
		*slash = '/';
		p = slash + 1;
	}

	return FR_OK;
}

/* 获取节点属性 */
static int fatfs_stat(struct dentry *dentry, struct stat *st)
{
	FILINFO fno;
	struct fatfs_dentry *fd = (struct fatfs_dentry *)dentry->private_data;
	FRESULT res = f_stat(fd->name, &fno);

	if (res == FR_OK) {
		if (fno.fattrib & AM_DIR) {
			pr_info("%s is dir", fd->name);
		} else {
			st->st_size = fno.fsize;
		}
	} else if (res == FR_NO_FILE || res == FR_NO_PATH) {
		pr_err("No file or dir");
	}

	return res;
}

/* 重命名 / 移动节点（同文件系统内） */
static int fatfs_rename(const char *oldpath, const char *newpath)
{
	FRESULT res = f_rename(oldpath, newpath);

	switch (res) {
	case FR_OK:
		break;

	case FR_EXIST:
		break;

	case FR_NO_FILE:
	case FR_NO_PATH:
		break;

	case FR_DENIED:
		break;

	default:
		break;
	}
	return res;
}

static void fatfs_sync(struct dentry *dentry)
{
	FIL *fp = (FIL *)((struct fatfs_dentry *)dentry->private_data)->data;
	if (fp) {
		while (f_sync(fp) != FR_OK)
			;
	}
}

static int fatfs_mount(struct file_system_type *fstype, const char *path)
{
	pr_info("Mount fatfs to %s", path);
	f_mount(&fatfs_root, "", 1);
	return mount_nodev(fstype, path);
}

static struct file_system_operations fs_ops = {
        .mount = fatfs_mount,
        .lookup = fatfs_lookup,
        .open = fatfs_open,
        .release = fatfs_release,
        .creat = fatfs_creat,
        .mkdir = fatfs_mkdir,
        .unlink = fatfs_unlink,
        .rename = fatfs_rename,
        .read = fatfs_read,
        .write = fatfs_write,
        .readdir = fatfs_readdir,
        .stat = fatfs_stat,
        .sync = fatfs_sync,
};

static struct file_system_type fatfs_type = {
        .fs_name = "fatfs",
        .fs_ops = &fs_ops,
        .private_data = (void *)&fatfs_root,
};

static int fatfs_init(void)
{
	printk("fatfs_init");
	return register_filesystem(&fatfs_type);
}
module_init(fatfs_init);

static void fatfs_exit(void)
{
	printk("fatfs_exit");
	unregister_filesystem(&fatfs_type);
}
module_exit(fatfs_exit);
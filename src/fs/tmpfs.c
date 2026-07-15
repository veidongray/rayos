#include <fs.h>
#include <init.h>
#include <mm.h>
#include <mount.h>
#include <printk.h>
#include <string.h>
#include <vfs_errno.h>

/* 简易错误码定义，替代标准 errno */
enum tmpfs_err {
	TMPFS_OK = 0,
	TMPFS_ERR_INVAL = -1,   /* 参数无效 */
	TMPFS_ERR_NOMEM = -2,   /* 内存分配失败 */
	TMPFS_ERR_TOOLONG = -3, /* 名称超长 */
	TMPFS_ERR_ISDIR = -4,   /* 目标是目录，不能作为文件操作 */
	TMPFS_ERR_NOTDIR = -5,  /* 目标不是目录 */
	TMPFS_ERR_NOENT = -6,   /* 节点不存在 */
	TMPFS_ERR_NOTEMPTY = -7, /* 目录非空，无法删除/覆盖 */
};

enum dentry_type {
	DENTRY_FILE,
	DENTRY_DIR,
};

struct tmpfs_dentry {
	char *name;
	int nmlen;
	enum dentry_type type;
	size_t size;
	struct tmpfs_dentry *parent;
	struct list_head children;
	struct list_head sibling;
	void *data;
};

struct tmpfs_dentry tmpfs_root = {
        .name = "/",
        .nmlen = 1,
        .type = DENTRY_DIR,
        .parent = NULL,
        .children = LIST_HEAD_INIT(tmpfs_root.children),
        .sibling = LIST_HEAD_INIT(tmpfs_root.sibling),
};

static struct tmpfs_dentry *tmpfs_alloc_dentry(struct tmpfs_dentry *parent,
                                               const char *name, int len,
                                               enum dentry_type type)
{
	struct tmpfs_dentry *td = kzalloc(sizeof(*td));
	if (!td)
		return NULL;

	td->name = kzalloc(len + 1);
	if (!td->name) {
		kfree(td);
		return NULL;
	}
	memcpy(td->name, name, len);
	td->nmlen = len;
	td->type = type;
	td->parent = parent;
	INIT_LIST_HEAD(&td->children);
	list_add_tail(&td->sibling, &parent->children);
	return td;
}

static struct tmpfs_dentry *tmpfs_lookup(const char *path)
{
	if (!path || path[0] != '/')
		return NULL;
	if (path[1] == '\0')
		return &tmpfs_root;

	struct tmpfs_dentry *cur = &tmpfs_root;
	const char *p = path + 1;

	while (*p) {
		while (*p == '/')
			p++;
		if (*p == '\0')
			break;

		const char *slash = strchr(p, '/');
		int len = slash ? (int)(slash - p) : (int)strlen(p);
		if (len > FS_NAME_MAX)
			return NULL;

		struct tmpfs_dentry *child = NULL, *pos;
		list_for_each_entry(pos, &cur->children, sibling)
		{
			if (pos->nmlen == len &&
			    memcmp(pos->name, p, len) == 0) {
				child = pos;
				break;
			}
		}
		if (!child)
			return NULL;

		cur = child;
		p += len;
	}
	return cur;
}

static int tmpfs_creat(const char *path)
{
	if (!path || path[0] != '/' || path[1] == '\0')
		return TMPFS_ERR_INVAL;

	struct tmpfs_dentry *cur = &tmpfs_root;
	const char *p = path + 1;

	while (*p) {
		while (*p == '/')
			p++;
		if (*p == '\0')
			break;

		const char *slash = strchr(p, '/');
		int len = slash ? (int)(slash - p) : (int)strlen(p);
		if (len > FS_NAME_MAX)
			return TMPFS_ERR_TOOLONG;

		struct tmpfs_dentry *child = NULL, *pos;
		list_for_each_entry(pos, &cur->children, sibling)
		{
			if (pos->nmlen == len &&
			    memcmp(pos->name, p, len) == 0) {
				child = pos;
				break;
			}
		}

		if (!child) {
			enum dentry_type type =
			        slash ? DENTRY_DIR : DENTRY_FILE;
			child = tmpfs_alloc_dentry(cur, p, len, type);
			if (!child)
				return TMPFS_ERR_NOMEM;
		} else if (!slash && child->type == DENTRY_DIR) {
			return TMPFS_ERR_ISDIR;
		}

		cur = child;
		p += len;
	}
	return TMPFS_OK;
}

static ssize_t tmpfs_read(struct dentry *dentry, void *buf, size_t len)
{
	struct tmpfs_dentry *td = (struct tmpfs_dentry *)dentry->private_data;
	if (!td || td->type != DENTRY_FILE || !td->data)
		return TMPFS_ERR_INVAL;

	if (len > td->size)
		len = td->size;

	memcpy(buf, td->data, len);
	return (ssize_t)len;
}

static ssize_t tmpfs_write(struct dentry *dentry, const void *buf, size_t len)
{
	struct tmpfs_dentry *td = (struct tmpfs_dentry *)dentry->private_data;
	if (!td || td->type != DENTRY_FILE)
		return TMPFS_ERR_INVAL;

	void *new_data;

	if (td->data == NULL) {
		new_data = kzalloc(len);
	} else {
		new_data = krealloc(td->data, len);
	}
	if (!new_data && len > 0)
		return TMPFS_ERR_NOMEM;

	td->data = new_data;
	memcpy(td->data, buf, len);
	td->size = len;
	return (ssize_t)len;
}

static int tmpfs_readdir(const char *path)
{
	struct tmpfs_dentry *dir = tmpfs_lookup(path);
	if (!dir)
		return TMPFS_ERR_NOENT;
	if (dir->type != DENTRY_DIR)
		return TMPFS_ERR_NOTDIR;

	struct tmpfs_dentry *de;
	list_for_each_entry(de, &dir->children, sibling)
	        printk("%s%c ", de->name, de->type == DENTRY_DIR ? '/' : ' ');
	return TMPFS_OK;
}

/* 内部辅助：从父节点摘除并释放单个dentry（不递归释放子树） */
static void tmpfs_free_dentry(struct tmpfs_dentry *td)
{
	if (!td)
		return;
	list_del(&td->sibling);
	kfree(td->name);
	if (td->data != NULL && td->type == DENTRY_FILE) {
		kfree(td->data);
	}
	kfree(td);
}

/* 删除文件或空目录 */
static int tmpfs_unlink(const char *path)
{
	if (!path || path[0] != '/' || path[1] == '\0')
		return TMPFS_ERR_INVAL;

	struct tmpfs_dentry *target = tmpfs_lookup(path);
	if (!target)
		return TMPFS_ERR_NOENT;
	/* 根目录不可删除 */
	if (target == &tmpfs_root)
		return TMPFS_ERR_INVAL;
	/* 非空目录不可删除 */
	if (target->type == DENTRY_DIR && !list_empty(&target->children))
		return TMPFS_ERR_NOTEMPTY;

	tmpfs_free_dentry(target);
	return TMPFS_OK;
}

/* 显式创建目录（支持多级自动创建中间目录） */
static int tmpfs_mkdir(const char *path)
{
	if (!path || path[0] != '/' || path[1] == '\0')
		return TMPFS_ERR_INVAL;

	struct tmpfs_dentry *cur = &tmpfs_root;
	const char *p = path + 1;

	while (*p) {
		while (*p == '/')
			p++;
		if (*p == '\0')
			break;

		const char *slash = strchr(p, '/');
		int len = slash ? (int)(slash - p) : (int)strlen(p);
		if (len > FS_NAME_MAX)
			return TMPFS_ERR_TOOLONG;

		struct tmpfs_dentry *child = NULL, *pos;
		list_for_each_entry(pos, &cur->children, sibling)
		{
			if (pos->nmlen == len &&
			    memcmp(pos->name, p, len) == 0) {
				child = pos;
				break;
			}
		}

		if (!child) {
			/* mkdir 所有层级都必须是目录 */
			child = tmpfs_alloc_dentry(cur, p, len, DENTRY_DIR);
			if (!child)
				return TMPFS_ERR_NOMEM;
		} else if (child->type != DENTRY_DIR) {
			/* 路径中某一级是文件，无法作为目录的父级 */
			return TMPFS_ERR_NOTDIR;
		}

		cur = child;
		p += len;
	}
	return TMPFS_OK;
}

/* 获取节点属性 */
static int tmpfs_stat(struct dentry *dentry, struct stat *st)
{
	if (!st)
		return TMPFS_ERR_INVAL;

	struct tmpfs_dentry *t = (struct tmpfs_dentry *)dentry->private_data;
	struct tmpfs_dentry *de = tmpfs_lookup(t->name);
	if (!de)
		return TMPFS_ERR_NOENT;

	st->st_size = de->size;
	return TMPFS_OK;
}

/* 重命名 / 移动节点（同文件系统内） */
static int tmpfs_rename(const char *oldpath, const char *newpath)
{
	if (!oldpath || !newpath || oldpath[0] != '/' || newpath[0] != '/')
		return TMPFS_ERR_INVAL;
	/* 禁止移动根目录 */
	if (oldpath[1] == '\0')
		return TMPFS_ERR_INVAL;

	struct tmpfs_dentry *src = tmpfs_lookup(oldpath);
	if (!src)
		return TMPFS_ERR_NOENT;

	/* 解析新路径的父目录和末级名称 */
	const char *last_slash = strrchr(newpath, '/');
	/* newpath 至少为 "/x"，last_slash 不会为 NULL */
	const char *new_name = last_slash + 1;
	int new_nmlen = (int)strlen(new_name);
	if (new_nmlen == 0 || new_nmlen > FS_NAME_MAX)
		return TMPFS_ERR_INVAL;

	/* 获取新路径的父目录：若 newpath 为 "/name" 则父目录是 root */
	struct tmpfs_dentry *new_parent;
	if (last_slash == newpath) {
		new_parent = &tmpfs_root;
	} else {
		/* 临时构造父路径字符串用于 lookup */
		int parent_len = (int)(last_slash - newpath);
		char *parent_path = kzalloc(parent_len + 1);
		if (!parent_path)
			return TMPFS_ERR_NOMEM;
		memcpy(parent_path, newpath, parent_len);
		new_parent = tmpfs_lookup(parent_path);
		kfree(parent_path);
		if (!new_parent)
			return TMPFS_ERR_NOENT;
	}

	if (new_parent->type != DENTRY_DIR)
		return TMPFS_ERR_NOTDIR;

	/* 检查新父目录下是否已有同名节点 */
	struct tmpfs_dentry *exist = NULL, *pos;
	list_for_each_entry(pos, &new_parent->children, sibling)
	{
		if (pos->nmlen == new_nmlen &&
		    memcmp(pos->name, new_name, new_nmlen) == 0) {
			exist = pos;
			break;
		}
	}
	/* 目标已存在且不是空目录，拒绝覆盖 */
	if (exist && exist->type == DENTRY_DIR && !list_empty(&exist->children))
		return TMPFS_ERR_NOTEMPTY;
	/* 若目标是文件或非空目录以外的节点，先删除 */
	if (exist)
		tmpfs_free_dentry(exist);

	/* 分配新名称并替换旧名称 */
	char *name_buf = kzalloc(new_nmlen + 1);
	if (!name_buf)
		return TMPFS_ERR_NOMEM;
	memcpy(name_buf, new_name, new_nmlen);

	/* 从旧父节点摘除，挂到新父节点 */
	list_del(&src->sibling);
	kfree(src->name);
	src->name = name_buf;
	src->nmlen = new_nmlen;
	src->parent = new_parent;
	list_add_tail(&src->sibling, &new_parent->children);

	return TMPFS_OK;
}

static int tmpfs_open(struct file *filp, mode_t mode)
{
	struct dentry *dentry = filp->dentry;
	struct tmpfs_dentry *t;
	struct tmpfs_dentry *td = (struct tmpfs_dentry *)dentry->private_data;

	mode = mode;
	t = tmpfs_lookup(td->name);
	if (t == NULL) {
		return -ENOENT;
	}
	return 0;
}

static int tmpfs_release(struct file *filp)
{
	filp = filp;
	return 0;
}

static int tmpfs_mount(struct file_system_type *fstype, const char *path)
{
	pr_info("Mount tmpfs to %s", path);
	return mount_nodev(fstype, path);
}

static struct file_system_operations fs_ops = {
        .mount = tmpfs_mount,
        .open = tmpfs_open,
        .release = tmpfs_release,
        .creat = tmpfs_creat,
        .mkdir = tmpfs_mkdir,
        .unlink = tmpfs_unlink,
        .rename = tmpfs_rename,
        .read = tmpfs_read,
        .write = tmpfs_write,
        .readdir = tmpfs_readdir,
        .stat = tmpfs_stat,
};

static struct file_system_type tmpfs_type = {
        .fs_name = "tmpfs",
        .fs_ops = &fs_ops,
        .private_data = (void *)&tmpfs_root,
};

static int tmpfs_init(void)
{
	printk("tmpfs_init");
	return register_filesystem(&tmpfs_type);
}
module_init(tmpfs_init);

static void tmpfs_exit(void)
{
	printk("tmpfs_exit");
	unregister_filesystem(&tmpfs_type);
}
module_exit(tmpfs_exit);
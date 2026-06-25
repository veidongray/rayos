#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <init.h>
#include <string.h>
#include <printk.h>

/* Super block operations */
static struct inode *tmpfs_alloc_inode(struct super_block *sb)
{
    struct inode *inode = (struct inode *)kzalloc(sizeof(struct inode));

    if (!inode)
        return NULL;

    printk("tmpfs_alloc_inode\n");
    return inode;
}

static void tmpfs_destroy_inode(struct inode *inod)
{
    printk("tmpfs_destroy_inode\n");
    kfree(inod);
}

static struct super_operations tmpfs_super_ops = {
    .alloc_inode = tmpfs_alloc_inode,
    .destroy_inode = tmpfs_destroy_inode,
};

/* Inode operations */
static struct dentry *tmpfs_lookup(struct inode *dir, struct dentry *dentry)
{
    printk("tmpfs_lookup\n");
    return NULL;
}

static int tmpfs_create(struct inode *dir, struct dentry *dentry, mode_t mode)
{
    printk("tmpfs_create\n");
    return 0;
}

static int tmpfs_mkdir(struct inode *dir, struct dentry *dentry, mode_t mode)
{
    printk("tmpfs_mkdir\n");
    return 0;
}

static int tmpfs_unlink(struct inode *dir, struct dentry *dentry)
{
    printk("tmpfs_unlink\n");
    return 0;
}

static int tmpfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    printk("tmpfs_rmdir\n");
    return 0;
}

static struct inode_operations tmpfs_inode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .rmdir = tmpfs_rmdir,
    .unlink = tmpfs_unlink,
};

/* File operations */
static ssize_t tmpfs_read(struct file *filp, char *buf, size_t count, __loff_t *ppos)
{
    printk("tmpfs_read\n");
    return 0;
}

static ssize_t tmpfs_write(struct file *filp, const char *buf, size_t count, __loff_t *ppos)
{
    printk("tmpfs_write\n");
    return 0;
}

static __loff_t tmpfs_llseek(struct file *filp, __loff_t offset, int whence)
{
    printk("tmpfs_llseek\n");
    return 0;
}

static int tmpfs_readdir(struct file *filp, void *dirent,
                         int (*filldir)(void *, const char *, int, __loff_t, ino_t))
{
    printk("tmpfs_readdir\n");
    return 0;
}

static int tmpfs_release(struct file *filp)
{
    printk("tmpfs_release\n");
    return 0;
}

static struct file_operations tmpfs_file_ops = {
    .read = tmpfs_read,
    .readdir = tmpfs_readdir,
    .llseek = tmpfs_llseek,
    .write = tmpfs_write,
    .release = tmpfs_release,
};

/* Dentry operations */
static int tmpfs_compare(const struct dentry *dentry, const char *name, int len)
{
    printk("tmpfs_compare\n");
    return 0;
}

static struct dentry_operations tmpfs_dentry_ops = {
    .d_compare = tmpfs_compare,
};

static int tmpfs_fill_super(struct super_block *sb, void *data, int flags)
{
    struct dentry *root;
    struct inode *root_inode;

    sb->s_blocksize = 512;
    sb->s_fs_info = NULL;
    sb->s_magic = 0XABCDDCBA;
    sb->s_ops = &tmpfs_super_ops;

    root_inode = sb->s_ops->alloc_inode(sb);
    if (!root_inode)
        return -1;

    root_inode->f_ops = &tmpfs_file_ops;
    root_inode->i_ino = 1;
    root_inode->i_ops = &tmpfs_inode_ops;
    root_inode->i_private = NULL;
    root_inode->i_sb = sb;
    root_inode->i_size = 0;

    root = (struct dentry *)kzalloc(sizeof(struct dentry));
    if (!root)
    {
        kfree(root_inode);
        return -1;
    }

    root->d_inode = root_inode;
    root->d_ops = &tmpfs_dentry_ops;
    root->d_parent = root;
    strcpy(root->d_name, "/");
    INIT_LIST_HEAD(&root->d_subdirs);
    list_add_tail(&root->d_child, &root->d_subdirs);

    sb->s_root = root;
    super_block_add(sb);

    return 0;
}

static struct dentry *tmpfs_mount(struct file_system_type *fs_type, int flags,
                                  const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, tmpfs_fill_super);
}

static void tmpfs_kill_sb(struct super_block *sb)
{
    // Do nothings.
}

struct file_system_type tmpfs_type = {
    .name = "tmpfs",
    .fs_flags = FS_NODEV,
    .mount = tmpfs_mount,
    .kill_sb = tmpfs_kill_sb,
};

static int tmpfs_init(void)
{
    printk("tmpfs_init\n");
    return register_filesystem(&tmpfs_type);
}
module_init(tmpfs_init);

static void tmpfs_exit(void)
{
    printk("tmpfs_exit\n");
    unregister_filesystem(&tmpfs_type);
}
module_exit(tmpfs_exit);
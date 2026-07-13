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

#include <atomic.h>
#include <block_device.h>
#include <fat32.h>
#include <ff.h>
#include <fs.h>
#include <mempool.h>
#include <mm.h>
#include <page.h>
#include <printk.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <task.h>
#include <types.h>
#include <vfs.h>

int vfs_init(void)
{
	/**
	 * @brief cmdline
	 *
	 * 后期加入 cmdline 支持
	 *
	 */
	const char *root_path = "/";
	const char *root_fstype = "fatfs";

	struct file_system_type *fstype;
	fstype = fs_get_by_name(root_fstype);
	fstype->fs_ops->mount(fstype, root_path);
	return 0;
}
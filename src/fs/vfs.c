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
#include <vfs.h>
#include <task.h>
#include <page.h>
#include <fat32.h>
#include <types.h>
#include <printk.h>
#include <string.h>
#include <atomic.h>
#include <mempool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <block_device.h>

static FATFS fs;

int vfs_init(void)
{
    f_mount(&fs, "", 1);
    return 0;
}
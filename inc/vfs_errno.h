#ifndef VFS_ERRNO_H
#define VFS_ERRNO_H

#define ENOENT 2   /* No such file or directory: 路径分量不存在 */
#define EACCES 13  /* Permission denied: 缺少读/写/执行权限 */
#define EPERM 1    /* Operation not permitted: 操作被禁止(如immutable) */
#define ENOTDIR 20 /* Not a directory: 路径中间分量不是目录 */
#define ELOOP 40   /* Too many symbolic links: 符号链接解析超限 */
#define ENAMETOOLONG 36 /* File name too long: 文件名或路径超长 */
#define ESTALE 116      /* Stale file handle: NFS inode/dentry已失效 */

#define EEXIST 17  /* File exists: O_CREAT|O_EXCL 时文件已存在 */
#define EISDIR 21  /* Is a directory: 对目录执行写操作 */
#define ENODEV 19  /* No such device: 设备驱动未注册/fs类型不支持 */
#define EROFS 30   /* Read-only file system: 只读文件系统上写操作 */
#define ETXTBSY 26 /* Text file busy: 写入正在执行的可执行文件 */
#define EMFILE 24  /* Too many open files: 进程fd数达上限 */
#define ENFILE 23  /* File table overflow: 系统全局file数量达上限 */

#define EBADF 9    /* Bad file descriptor: fd无效或模式不匹配 */
#define EINVAL 22  /* Invalid argument: 参数非法(负偏移/未对齐等) */
#define EFBIG 27   /* File too large: 写入超出文件大小限制 */
#define ENOSPC 28  /* No space left on device: 磁盘空间/inode耗尽 */
#define EDQUOT 122 /* Disk quota exceeded: 超出用户/组配额 */
#define EIO 5      /* I/O error: 底层硬件错误(由具体fs向上透传) */

#define EBUSY 16     /* Device or resource busy: 卸载时仍有打开文件 */
#define ENXIO 6      /* No such device or address: 块设备无法打开 */
#define EOVERFLOW 75 /* Value too large: stat数据超出32位表示范围 */

#define ENOMEM 12 /* Out of memory: 内核内存分配失败 */
#define EFAULT 14 /* Bad address: 用户空间指针无效 */
#define EINTR 4   /* Interrupted system call: 被信号中断 */
#define EAGAIN 11 /* Resource temporarily unavailable: 非阻塞操作需重试 */

#endif /* VFS_ERRNO_H */
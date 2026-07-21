#ifndef VFS_H
#define VFS_H

#include <fs.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <types.h>
#include <vfs_errno.h>

void vfs_init(void);
int vfs_open(struct file *filp, const char *path, mode_t mode);
int vfs_close(struct file *filp);
int vfs_read(struct file *filp, void *buf, size_t len);
int vfs_write(struct file *filp, const void *buf, size_t len);
int vfs_stat(const char *path, struct stat *st);
void vfs_sync(struct file *filp);
int vfs_creat(const char *pathname, mode_t mode);

#endif // VFS_H
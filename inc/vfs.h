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
int vfs_read(const char *path, void *buf, size_t len);
int vfs_write(const char *path, const void *buf, size_t len);
int vfs_stat(const char *path, struct stat *st);
void vfs_sync(const char *pathname);

#endif // VFS_H
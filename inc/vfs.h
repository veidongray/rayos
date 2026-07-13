#ifndef VFS_H
#define VFS_H

#include <fs.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <types.h>

struct vfs_file {
	int fd;
	void *priv;
	struct list_head list;
};

int vfs_init(void);

#endif // VFS_H
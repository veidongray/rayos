#ifndef FS_H
#define FS_H

#include <atomic.h>
#include <list.h>
#include <stdbool.h>
#include <sys/types.h>
#include <types.h>

#define FS_NAME_MAX 255

struct file_system_type {
	const char *name;
	int nm_len;

	int (*mount)(struct file_system_type *fstype, const char *dev_name);
	struct list_head fs_list;
};

struct mount {
	struct file_system_type *fs_type;

	struct list_head mnt_list;
};

int register_filesystem(struct file_system_type *fs);
void unregister_filesystem(struct file_system_type *fs);
struct file_system_type *fs_get_by_name(const char *name);

#endif /* FS_H */
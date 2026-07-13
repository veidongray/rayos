#ifndef MOUNT_H
#define MOUNT_H

#include <fs.h>

struct mount {
	char *mnt_path;

	struct file_system_type *mnt_fstype;

	struct list_head mnt_list;
};

int mount_nodev(struct file_system_type *fstype, const char *path);

#endif /* MOUNT_H */
#include <mm.h>
#include <mount.h>
#include <string.h>

static LIST_HEAD(mount_list);

int mount_nodev(struct file_system_type *fstype, const char *path)
{
	struct mount *mnt;

	mnt = kzalloc(sizeof(struct mount));
	if (!mnt) {
		return -1;
	}

	mnt->mnt_fstype = fstype;

	int len = strlen(path);
	mnt->mnt_path = kzalloc(len);

	memcpy(mnt->mnt_path, path, strlen(path));
	list_add_tail(&mnt->mnt_list, &mount_list);

	return 0;
}
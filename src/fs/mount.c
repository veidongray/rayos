#include <mm.h>
#include <mount.h>
#include <printk.h>
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
	mnt->mnt_path = kzalloc(len + 1);
	mnt->mnt_path[len] = '\0';
	mnt->mnt_pathlen = strlen(path) + 1;
	memcpy(mnt->mnt_path, path, strlen(path));

	/* 消除末尾的 '/' */
	for (char *p = &mnt->mnt_path[len - 1]; *p == '/'; p--) {
		*p = '\0';
	}

	list_add_tail(&mnt->mnt_list, &mount_list);

	return 0;
}

struct mount *mount_get_by_name(const char *path)
{
	int match_len;
	struct mount *pos;
	char *p, *path_buff, *match_path;

	path_buff = kzalloc(strlen(path));
	strcpy(path_buff, path);
	match_path = path_buff;

	while (!!(p = strrchr(path_buff, '/'))) {
		memset(p, '\0', strlen(p));

		match_len = strlen(match_path);
		list_for_each_entry(pos, &mount_list, mnt_list)
		{
			if ((!strncmp(pos->mnt_path, match_path, match_len))) {
				return pos;
			}
		}
	}

	return NULL;
}
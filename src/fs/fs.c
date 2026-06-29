#include <fs.h>
#include <mm.h>
#include <list.h>
#include <string.h>
#include <printk.h>

static LIST_HEAD(fs_type_list);

int register_filesystem(struct file_system_type *fs)
{
    fs->nm_len = strlen(fs->name);
    list_add_tail(&fs->fs_list, &fs_type_list);
    return 0;
}
void unregister_filesystem(struct file_system_type *fs)
{
    list_del(&fs->fs_list);
}

struct file_system_type *fs_get_by_name(const char *name)
{
    struct file_system_type *fs_type;

    if (list_empty(&fs_type_list))
        return NULL;

    list_for_each_entry(fs_type, &fs_type_list, fs_list)
    {
        if (!strncmp(fs_type->name, name, fs_type->nm_len))
            return fs_type;
    }

    return NULL;
}

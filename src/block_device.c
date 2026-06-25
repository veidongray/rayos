#include <mm.h>
#include <list.h>
#include <string.h>
#include <block_device.h>

LIST_HEAD(blkdev_list);

int blkdev_register(struct block_device *blkdev, struct block_device_ops *ops)
{
    blkdev->ops = ops;
    list_add_tail(&blkdev->bd_list, &blkdev_list);
    return 0;
}

void blkdev_unregister(struct block_device *blkdev)
{
    list_del(&blkdev->bd_list);
}

void blkdev_destroy(struct block_device *blkdev)
{
    list_del(&blkdev->bd_list);
    kfree(blkdev);
}

struct block_device *blkdev_get_by_name(const char *name)
{
    struct block_device *bdev;

    if (!name || list_empty(&blkdev_list))
        return NULL;

    list_for_each_entry(bdev, &blkdev_list, bd_list)
    {
        if (strcmp(name, bdev->info.bd_name) == 0)
            return bdev;
    }
    return NULL;
}
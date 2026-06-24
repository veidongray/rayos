#include <mm.h>
#include <list.h>
#include <block_device.h>

LIST_HEAD(__list_blkdev);

int blkdev_register(struct block_device *blkdev)
{
    list_add_tail(&blkdev->list, &__list_blkdev);
    return 0;
}

void blkdev_unregister(struct block_device *blkdev)
{
    list_del(&blkdev->list);
}

void blkdev_destroy(struct block_device *blkdev)
{
    list_del(&blkdev->list);
    kfree(blkdev);
}
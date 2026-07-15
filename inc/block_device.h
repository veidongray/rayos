#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <list.h>
#include <stdbool.h>
#include <stdint.h>

struct block_device_ops;
struct block_device_info;
struct block_device;

struct block_device_ops {
	int (*read)(struct block_device *dev, uint64_t lba, uint32_t count,
	            void *buf);
	int (*write)(struct block_device *dev, uint64_t lba, uint32_t count,
	             void *buf);
	int (*sync)(struct block_device *dev);
};

struct block_device_info {
	char bd_name[32];
	int blkdev_id;
};

struct block_device {
	const struct block_device_ops *ops;
	struct block_device_info info;
	void *priv;
	struct list_head bd_list;
};

int blkdev_register(struct block_device *blkdev, struct block_device_ops *ops);
void blkdev_unregister(struct block_device *blkdev);
void blkdev_destroy(struct block_device *blkdev);
struct block_device *blkdev_get_by_name(const char *name);

#endif // BLOCK_DEVICE_H
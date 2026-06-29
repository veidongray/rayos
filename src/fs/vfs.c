#include <ff.h>
#include <fs.h>
#include <mm.h>
#include <vfs.h>
#include <task.h>
#include <page.h>
#include <fat32.h>
#include <types.h>
#include <printk.h>
#include <string.h>
#include <atomic.h>
#include <mempool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <block_device.h>

static FATFS fs;

int vfs_init(void)
{
    f_mount(&fs, "", 1);
    return 0;
}
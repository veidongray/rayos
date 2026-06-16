#ifndef FAT32_H
#define FAT32_h

#include <list.h>
#include <stdint.h>
#include <stddef.h>

/* ========== 基础属性位定义 ========== */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F /* = READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID */

/* LFN 条目的 attr 必须严格等于 0x0F，不能多也不能少 */
#define ATTR_LONG_NAME_MASK 0x0F

/* 这些属性在文件搜索/列目录时通常需要隐藏 */
#define ATTR_HIDDEN_FILTER (ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

/* SFN name[0] 特殊值定义 */
#define SFN_NAME0_FREE 0x00       /* 目录结束，后续条目均无效 */
#define SFN_NAME0_DELETED 0xE5    /* 条目已删除 */
#define SFN_NAME0_E5_ESCAPE 0x05  /* 实际首字符为 0xE5 的转义 */
#define SFN_NAME0_DOT 0x2E        /* '.' 或 '..' 目录条目 */
#define SFN_NAME0_LFN_MARKER 0x7B /* 有关联的 Unicode LFN */

/* LFN order 字段常量定义 */
#define LFN_ORDER_LAST_ENTRY 0x40 /* 最高位：标记这是 LFN 链的最后一个片段(物理上第一个) */
#define LFN_ORDER_SEQ_MASK 0x1F   /* 低5位掩码：提取片段序号 (1~20) */
#define LFN_ORDER_DELETED 0xE5    /* 整个 LFN 条目已被删除 */
#define LFN_MAX_FRAGMENTS 20      /* FAT32 规范允许的最大 LFN 片段数 */

struct mbr_partition
{
    uint8_t boot;
    uint8_t start_chs[3];
    uint8_t type;
    uint8_t end_chs[3];
    uint32_t first_lba;
    uint32_t sector_count;
};

struct fat32_bpb
{
    /* 0x00 */
    uint8_t jump_boot[3]; // EB ?? 90
    /* 0x03 */
    char oem_name[8];
    /* 0x0B */
    uint16_t bytes_per_sector; // 通常 512
    /* 0x0D */
    uint8_t sectors_per_cluster;
    /* 0x0E */
    uint16_t reserved_sector_count;
    /* 0x10 */
    uint8_t num_fats; // 通常 2
    /* 0x11 */
    uint16_t root_entry_count; // FAT32 = 0
    /* 0x13 */
    uint16_t total_sectors_16; // FAT32通常为0
    /* 0x15 */
    uint8_t media;
    /* 0x16 */
    uint16_t fat_size_16; // FAT32 = 0
    /* 0x18 */
    uint16_t sectors_per_track;
    /* 0x1A */
    uint16_t num_heads;
    /* 0x1C */
    uint32_t hidden_sectors;
    /* 0x20 */
    uint32_t total_sectors_32;
    /* ===== FAT32 Extended BPB ===== */
    /* 0x24 */
    uint32_t fat_size_32;
    /* 0x28 */
    uint16_t ext_flags;
    /* 0x2A */
    uint16_t fs_version;
    /* 0x2C */
    uint32_t root_cluster; // 通常 = 2
    /* 0x30 */
    uint16_t fs_info;
    /* 0x32 */
    uint16_t backup_boot_sector;
    /* 0x34 */
    uint8_t reserved[12];
    /* 0x40 */
    uint8_t drive_number;
    /* 0x41 */
    uint8_t reserved1;
    /* 0x42 */
    uint8_t boot_signature; // 0x29
    /* 0x43 */
    uint32_t volume_id;
    /* 0x47 */
    char volume_label[11];
    /* 0x52 */
    char filesystem_type[8]; // "FAT32   "
} __attribute__((packed));

struct fat32_sfn_entry
{
    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} __attribute__((packed));

struct fat32_lfn_entry
{
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr; // 0x0F
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t first_cluster; // 总是0
    uint16_t name3[2];
} __attribute__((packed));

struct fat32_dir_entry
{
    union
    {
        struct fat32_sfn_entry sfn_entry;
        struct fat32_lfn_entry lfn_entry;
    };
} __attribute__((packed));

struct sata_device
{
    uint32_t port_no;
    struct hba_memory_registers *hba;
    struct sata_controller_port_register *port;
};

struct lookup_context
{
    char *lfn;
    char *target;
    struct fat32_dir_entry *entry;
    struct fat32_dir_entry *result;
};

typedef int (*fat32_foreach_dirent_callback_t)(struct lookup_context *ctx);

struct fat32_fs
{
    struct fat32_bpb *bpb;
    struct sata_device *sata_dev;
    struct fat32_dir_entry entry;
    struct fat32_dir_entry parent_entry;
};

struct fat32_file
{
    int fd;
    char *path;
    size_t size;
    struct fat32_fs fs;
    struct list_head list;
};

int fat32_create(const char *path);
int fat32_read_cluster(struct fat32_file *fp, char *buf, size_t size);
int fat32_write_cluster(struct fat32_file *fp, const char *buf, size_t size);
/**
 * 根据指定的 cluster 在对应目录中查找
 */
int fat32_foreach_dirent(fat32_foreach_dirent_callback_t cb, uint32_t dir_cluster, const char *target, struct fat32_dir_entry *result);
/**
 * 拆解目录并遍历查找
 */
int fat32_lookup(const char *path, struct fat32_dir_entry *entry_result);
int fat32_readdir(const char *path);
int fat32_read(int fd, char *buf, size_t size);
int fat32_write(int fd, const char *buf, size_t size);
int fat32_open(const char *path);
int fat32_close(int fd);

#endif // FAT32_H
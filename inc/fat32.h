#ifndef FAT32_H
#define FAT32_h

#include <stdint.h>

/* ========== 基础属性位定义 ========== */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F /* = READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID */

/* ========== 派生/组合属性掩码 ========== */
/* LFN 条目的 attr 必须严格等于 0x0F，不能多也不能少 */
#define ATTR_LONG_NAME_MASK 0x0F

/* 这些属性在文件搜索/列目录时通常需要隐藏 */
#define ATTR_HIDDEN_FILTER (ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

/* ========== 单属性检测宏（返回 bool） ========== */
#define FAT_IS_READ_ONLY(e) (((e)->attr & ATTR_READ_ONLY) != 0)
#define FAT_IS_HIDDEN(e) (((e)->attr & ATTR_HIDDEN) != 0)
#define FAT_IS_SYSTEM(e) (((e)->attr & ATTR_SYSTEM) != 0)
#define FAT_IS_VOLUME_ID(e) (((e)->attr & ATTR_VOLUME_ID) != 0)
#define FAT_IS_DIRECTORY(e) (((e)->attr & ATTR_DIRECTORY) != 0)
#define FAT_IS_ARCHIVE(e) (((e)->attr & ATTR_ARCHIVE) != 0)

/* ========== 条目类型判定宏（最关键） ========== */
/* 判断 LFN 必须用全等比较，不能用位与！
 * 因为 ATTR_LONG_NAME == 0x0F 恰好是低4位全部置1，
 * 若某个损坏的 SFN attr=0x1F，(0x1F & 0x0F)==0x0F 会误判为 LFN */
#define FAT_IS_LFN_ENTRY(e) ((uint8_t)(e)->attr == ATTR_LONG_NAME)
#define FAT_IS_SFN_ENTRY(e) (!FAT_IS_LFN_ENTRY(e))

/* ========== 属性合法性校验 ========== */
/* FAT32 规范禁止的属性组合：
 * - VOLUME_ID 不能与 DIRECTORY / HIDDEN / SYSTEM 同时存在
 * - LFN 条目 attr 必须严格为 0x0F（已由 FAT_IS_LFN_ENTRY 保证）
 * - DIRECTORY 不应有 READ_ONLY（虽不强制但属于异常） */
#define FAT_ATTR_VALID(e)                                                                \
    (FAT_IS_LFN_ENTRY(e)   ? 1                                                           \
     : FAT_IS_VOLUME_ID(e) ? !((e)->attr & (ATTR_DIRECTORY | ATTR_HIDDEN | ATTR_SYSTEM)) \
                           : 1)

/* ========== 用户可见性过滤 ========== */
/* 列目录时判断是否应该显示该条目 */
#define FAT_IS_VISIBLE(e) \
    (FAT_IS_SFN_ENTRY(e) && !FAT_IS_VOLUME_ID(e) && !FAT_IS_HIDDEN(e) && !FAT_IS_SYSTEM(e))

/* SFN name[0] 特殊值定义 */
#define SFN_NAME0_FREE 0x00       /* 目录结束，后续条目均无效 */
#define SFN_NAME0_DELETED 0xE5    /* 条目已删除 */
#define SFN_NAME0_E5_ESCAPE 0x05  /* 实际首字符为 0xE5 的转义 */
#define SFN_NAME0_DOT 0x2E        /* '.' 或 '..' 目录条目 */
#define SFN_NAME0_LFN_MARKER 0x7B /* 有关联的 Unicode LFN */

/* 类型判断宏（返回非零为真） */
#define SFN_IS_END_OF_DIR(e) ((uint8_t)(e)->name[0] == SFN_NAME0_FREE)
#define SFN_IS_DELETED(e) ((uint8_t)(e)->name[0] == SFN_NAME0_DELETED)
#define SFN_IS_E5_ESCAPED(e) ((uint8_t)(e)->name[0] == SFN_NAME0_E5_ESCAPE)
#define SFN_IS_DOT_ENTRY(e) ((uint8_t)(e)->name[0] == SFN_NAME0_DOT)
#define SFN_HAS_LFN(e) ((uint8_t)(e)->name[0] == SFN_NAME0_LFN_MARKER)

/* 获取真实首字符（处理 0x05 → 0xE5 转义） */
#define SFN_REAL_FIRST_CHAR(e) \
    ((uint8_t)((e)->name[0] == SFN_NAME0_E5_ESCAPE ? 0xE5 : (e)->name[0]))

/* 综合状态枚举（用于 switch/case 分发） */
typedef enum
{
    SFN_STATUS_NORMAL = 0,
    SFN_STATUS_END_OF_DIR,
    SFN_STATUS_DELETED,
    SFN_STATUS_DOT,
    SFN_STATUS_HAS_LFN
} sfn_status_t;

/* 一键分类宏 */
#define SFN_GET_STATUS(e)                                                    \
    ((uint8_t)(e)->name[0] == SFN_NAME0_FREE         ? SFN_STATUS_END_OF_DIR \
     : (uint8_t)(e)->name[0] == SFN_NAME0_DELETED    ? SFN_STATUS_DELETED    \
     : (uint8_t)(e)->name[0] == SFN_NAME0_DOT        ? SFN_STATUS_DOT        \
     : (uint8_t)(e)->name[0] == SFN_NAME0_LFN_MARKER ? SFN_STATUS_HAS_LFN    \
                                                     : SFN_STATUS_NORMAL)

/* LFN order 字段常量定义 */
#define LFN_ORDER_LAST_ENTRY 0x40 /* 最高位：标记这是 LFN 链的最后一个片段(物理上第一个) */
#define LFN_ORDER_SEQ_MASK 0x1F   /* 低5位掩码：提取片段序号 (1~20) */
#define LFN_ORDER_DELETED 0xE5    /* 整个 LFN 条目已被删除 */
#define LFN_MAX_FRAGMENTS 20      /* FAT32 规范允许的最大 LFN 片段数 */

/* 基础状态判断宏 */
#define LFN_IS_LAST(e) (((uint8_t)(e)->order & LFN_ORDER_LAST_ENTRY) != 0)
#define LFN_IS_DELETED(e) ((uint8_t)(e)->order == LFN_ORDER_DELETED)
#define LFN_GET_SEQ(e) ((uint8_t)((e)->order & LFN_ORDER_SEQ_MASK))

/* 合法性校验宏（防止解析损坏的文件系统导致越界） */
#define LFN_SEQ_VALID(e) (LFN_GET_SEQ(e) >= 1 && LFN_GET_SEQ(e) <= LFN_MAX_FRAGMENTS)

/* 综合状态枚举 */
typedef enum
{
    LFN_STATUS_VALID = 0, /* 正常有效的 LFN 片段 */
    LFN_STATUS_LAST,      /* 正常的最后一个 LFN 片段 */
    LFN_STATUS_DELETED,   /* 已删除的 LFN 片段 */
    LFN_STATUS_INVALID    /* 序号非法/文件系统损坏 */
} lfn_status_t;

/* 一键分类宏 */
#define LFN_GET_STATUS(e)                                          \
    ((uint8_t)(e)->order == LFN_ORDER_DELETED ? LFN_STATUS_DELETED \
     : !LFN_SEQ_VALID(e)                      ? LFN_STATUS_INVALID \
     : LFN_IS_LAST(e)                         ? LFN_STATUS_LAST    \
                                              : LFN_STATUS_VALID)

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
};

int fat32_readdir(const char *path);
uint32_t cluster_to_lba(struct fat32_bpb *bpb, uint32_t cluster);

#endif // FAT32_H
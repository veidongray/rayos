#include <mm.h>
#include <ahci.h>
#include <list.h>
#include <fat32.h>
#include <printk.h>
#include <lib/string/string.h>

#define FAT32_EOC(x) ((x) >= 0x0FFFFFF8)

struct fat32_fs
{
    struct fat32_bpb *bpb;
    struct sata_device *sata_dev;
    struct fat32_dir_entry entry;
};

struct fat32_file
{
    int fd;
    char *path;
    size_t size;
    struct fat32_fs fs;
    struct list_head list;
};
LIST_HEAD(g_fat32_file_list);

static int g_fd_count = 3;

size_t utf16_to_ascii(const uint16_t *utf16_str,
                      char *ascii_str,
                      size_t max_len)
{
    size_t i = 0;

    if (!utf16_str || !ascii_str || max_len == 0)
        return 0;

    while (*utf16_str && i < max_len)
    {
        uint16_t ch = *utf16_str++;

        if (ch <= 0x7F)
            ascii_str[i++] = (char)ch;
        else
            ascii_str[i++] = '?'; // 非ASCII字符替换为 '?'
    }

    return i;
}

void sfn_to_ascii(const char *sfn, char *out)
{
    int i;
    int pos = 0;

    for (i = 0; i < 8; i++)
    {
        if (sfn[i] == ' ')
            break;

        out[pos++] = sfn[i];
    }

    if (sfn[8] != ' ')
    {
        out[pos++] = '.';

        for (i = 8; i < 11; i++)
        {
            if (sfn[i] == ' ')
                break;

            out[pos++] = sfn[i];
        }
    }

    out[pos] = '\0';
}

/*
 * path : 输入路径
 * name : 输出当前路径分量
 *
 * 返回:
 *   NULL -> 结束
 *   非NULL -> 下一个位置
 */
char *path_next(char *path, char *name)
{
    size_t len = 0;

    if (!path)
        return NULL;

    /* 跳过连续 '/' */
    while (*path == '/')
        path++;

    if (*path == '\0')
        return NULL;

    while (*path && *path != '/')
    {
        name[len++] = *path++;
    }

    name[len] = '\0';

    return path;
}

static inline int fat32_is_dot(struct fat32_dir_entry *entry)
{
    return (entry->sfn_entry.attr & 0x10) && entry->sfn_entry.name[0] == '.' && entry->sfn_entry.name[1] == ' ';
}

static inline int fat32_is_dotdot(struct fat32_dir_entry *entry)
{
    return (entry->sfn_entry.attr & 0x10) && entry->sfn_entry.name[0] == '.' && entry->sfn_entry.name[1] == '.' && entry->sfn_entry.name[2] == ' ';
}

static inline struct fat32_file *fat32_find_file(int fd)
{
    struct list_head *pos;
    struct fat32_file *fp;

    list_for_each(pos, &g_fat32_file_list)
    {
        fp = container_of(pos, struct fat32_file, list);
        if (fp->fd == fd)
        {
            return fp;
        }
    }
    return NULL;
}

int fat32_print(struct fat32_dir_entry *entry, const char *str)
{
    if (fat32_is_dot(entry))
    {
        printk("<d> %u %s\n", entry->sfn_entry.file_size, ".");
        return 0;
    }
    if (fat32_is_dotdot(entry))
    {
        printk("<d> %u %s\n", entry->sfn_entry.file_size, "..");
        return 0;
    }
    if ((uint8_t)entry->sfn_entry.name[0] != 0xE5)
    {
        if (entry->sfn_entry.attr & 0x10)
        {
            printk("<d> %u %s\n", entry->sfn_entry.file_size, str);
        }
        else
        {
            printk("<-> %u %s\n", entry->sfn_entry.file_size, str);
        }
    }
    return 0;
}

int fat32_read_cluster(struct fat32_file *fp, char *buf, size_t size)
{
    uint32_t cluster;
    char *cluster_buf;
    uint32_t *fat_table;
    struct fat32_bpb *bpb;
    struct sata_controller_port_register *port;

    port = fp->fs.sata_dev->port;
    bpb = fp->fs.bpb;

    fat_table = kzalloc(bpb->fat_size_32 * bpb->bytes_per_sector);
    ahci_read(port, bpb->reserved_sector_count, bpb->fat_size_32, fat_table);

    cluster = (fp->fs.entry.sfn_entry.first_cluster_hi << 16) + fp->fs.entry.sfn_entry.first_cluster_lo;
    cluster_buf = kzalloc(bpb->bytes_per_sector * bpb->sectors_per_cluster);
    while (!FAT32_EOC(cluster))
    {
        ahci_read(port, cluster_to_lba(bpb, cluster), bpb->sectors_per_cluster, cluster_buf);
        if (size > (bpb->bytes_per_sector * bpb->sectors_per_cluster))
        {
            memcpy(buf, cluster_buf, (bpb->bytes_per_sector * bpb->sectors_per_cluster));
            size -= (bpb->bytes_per_sector * bpb->sectors_per_cluster);
        }
        else
        {
            memcpy(buf, cluster_buf, size);
            goto done;
        }
        cluster = fat_table[cluster];
    }

done:
    kfree(cluster_buf);
    kfree(fat_table);
    kfree(bpb);
    return 0;
}

int fat32_lfn_concat(struct fat32_dir_entry *entry, char *buf)
{
    // 拼接 LFN，每个 LFN entry 可存 13 个 UTF16 字符
    size_t offset = ((entry->lfn_entry.order & 0x1F) - 1) * 13;
    uint16_t n[6];

    memcpy(n, entry->lfn_entry.name1, 5 * sizeof(uint16_t));
    utf16_to_ascii(n, buf + offset, 5);

    memcpy(n, entry->lfn_entry.name2, 6 * sizeof(uint16_t));
    utf16_to_ascii(n, buf + offset + 5, 6);

    memcpy(n, entry->lfn_entry.name3, 2 * sizeof(uint16_t));
    utf16_to_ascii(n, buf + offset + 11, 2);

    return 0;
}

int fat32_readdir_cluster(uint32_t cluster)
{
    char *cluster_buf;
    char *name_buf;
    uint32_t *fat_table;
    uint32_t entries_per_cluster;
    struct fat32_bpb *bpb;
    struct fat32_dir_entry *entry;
    struct sata_device *sata_dev;
    struct sata_controller_port_register *port;
    struct hba_memory_registers *hba;

    sata_dev = get_sata_device();
    hba = get_host_bus_adapter();
    port = &hba->ports[sata_dev->port_no];

    bpb = kzalloc(512);
    ahci_read(port, 0, 1, bpb);
    fat_table = kzalloc(bpb->fat_size_32 * bpb->bytes_per_sector);
    ahci_read(port, bpb->reserved_sector_count, bpb->fat_size_32, fat_table);

    entries_per_cluster = (bpb->bytes_per_sector * bpb->sectors_per_cluster) / sizeof(struct fat32_dir_entry);
    cluster_buf = kzalloc(bpb->bytes_per_sector * bpb->sectors_per_cluster);
    name_buf = kzalloc(512);
    while (!FAT32_EOC(cluster))
    {
        ahci_read(port, cluster_to_lba(bpb, cluster), bpb->sectors_per_cluster, cluster_buf);
        entry = (struct fat32_dir_entry *)cluster_buf;
        for (uint32_t i = 0; i < entries_per_cluster; i++, entry++)
        {
            if (entry->sfn_entry.name[0] == 0x00)
            {
                goto done;
            }
            if ((uint8_t)entry->sfn_entry.name[0] == 0xE5)
            {
                continue;
            }
            /*
             * LFN
             */
            if (entry->lfn_entry.attr == 0x0F)
            {
                fat32_lfn_concat(entry, name_buf);
            }
            else
            {
                /*
                 * 没有LFN
                 */
                if (name_buf[0] != '\0')
                {
                    fat32_print(entry, name_buf);
                    memset(name_buf, 0, 512);
                }
                else
                {
                    memset(name_buf, 0, 16);
                    sfn_to_ascii(entry->sfn_entry.name, name_buf);
                    fat32_print(entry, name_buf);
                }
            }
        }
        cluster = fat_table[cluster];
    }

done:
    kfree(cluster_buf);
    kfree(name_buf);
    kfree(fat_table);
    kfree(bpb);
    return 0;
}

/**
 * 根据指定的 cluster 在对应目录中查找
 */
int fat32_lookup_in_cluster(struct fat32_bpb *bpb, uint32_t dir_cluster, const char *name, struct fat32_dir_entry *result)
{
    char *cluster_buf = NULL;
    char *name_buf = NULL;
    uint32_t *fat_table;
    uint32_t entries_per_cluster;
    struct fat32_dir_entry *entry = NULL;
    struct sata_device *sata_dev;
    struct sata_controller_port_register *port;

    if (!name || !result)
        return -1;

    // 获取 SATA 控制器端口
    sata_dev = get_sata_device();
    port = sata_dev->port;

    // 读取 FAT 表
    fat_table = kzalloc(bpb->fat_size_32 * bpb->bytes_per_sector);
    ahci_read(port, bpb->reserved_sector_count, bpb->fat_size_32, fat_table);

    // 每簇可存放的目录项数量
    entries_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster / sizeof(struct fat32_dir_entry);

    cluster_buf = kzalloc(bpb->bytes_per_sector * bpb->sectors_per_cluster);
    name_buf = kzalloc(512);

    while (!FAT32_EOC(dir_cluster))
    {
        ahci_read(port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
        entry = (struct fat32_dir_entry *)cluster_buf;

        for (uint32_t i = 0; i < entries_per_cluster; i++, entry++)
        {
            // 空目录项，目录结束
            if (entry->sfn_entry.name[0] == 0x00)
                goto not_found;

            // 已删除的目录项
            if ((uint8_t)entry->sfn_entry.name[0] == 0xE5)
                continue;

            // 长文件名
            if (entry->lfn_entry.attr == 0x0F)
            {
                fat32_lfn_concat(entry, name_buf);
            }
            else
            {
                // SFN 文件名（没有 LFN）
                if (name_buf[0] != '\0')
                {
                    if (strncmp(name, name_buf, strlen(name_buf) + 1) == 0)
                    {
                        // 复制找到的文件或者目录的目录项
                        memcpy(result, entry, sizeof(struct fat32_dir_entry));
                        goto done;
                    }
                }
                else
                {
                    sfn_to_ascii(entry->sfn_entry.name, name_buf); // 需要实现 SFN 转 ASCII 函数
                    if (strncmp(name, name_buf, strlen(name_buf) + 1) == 0)
                    {
                        // 复制找到的文件或者目录的目录项
                        memcpy(result, entry, sizeof(struct fat32_dir_entry));
                        goto done;
                    }
                }
                memset(name_buf, 0, 512);
            }
        }

        // 下一簇
        dir_cluster = fat_table[dir_cluster];
    }

not_found:
    kfree(cluster_buf);
    kfree(fat_table);
    kfree(name_buf);
    kfree(bpb);
    return -1;

done:
    kfree(cluster_buf);
    kfree(fat_table);
    kfree(name_buf);
    kfree(bpb);
    return 0;
}

/**
 * 拆解目录并遍历查找
 */
int fat32_lookup(const char *path, struct fat32_dir_entry *entry_result)
{
    int ret;
    char *dir;
    char *name;
    uint32_t cluster;
    struct fat32_bpb *bpb;
    struct fat32_dir_entry entry;
    struct sata_device *sata_dev;
    struct sata_controller_port_register *port;
    struct hba_memory_registers *hba;

    // 获取sata controller设备端口
    sata_dev = get_sata_device();
    hba = get_host_bus_adapter();
    port = &hba->ports[sata_dev->port_no];

    // 读取 BPB
    bpb = (struct fat32_bpb *)kzalloc(512);
    ahci_read(port, 0, 1, bpb);

    dir = (char *)path;
    cluster = bpb->root_cluster;
    name = kzalloc(512);
    while ((dir = path_next(dir, name)) != NULL)
    {
        ret = fat32_lookup_in_cluster(bpb, cluster, name, &entry);
        if (ret < 0)
        {
            entry_result = NULL;
            kfree(name);
            return ret;
        }
        cluster = (entry.sfn_entry.first_cluster_hi << 16) + entry.sfn_entry.first_cluster_lo;
    }

    // 成功查找复制 entry 值
    memcpy(entry_result, &entry, sizeof(struct fat32_dir_entry));
    kfree(name);
    return 0;
}

int fat32_readdir(const char *path)
{
    int ret;
    struct fat32_dir_entry entry;

    if (path[0] == '/' && path[1] == '\0')
    {
        fat32_readdir_cluster(2);
    }
    else
    {
        ret = fat32_lookup(path, &entry);
        if (ret < 0)
        {
            printk("'%s': No such file or directory\n", path);
            return ret;
        }
        if (entry.sfn_entry.attr & 0x10)
        {
            fat32_readdir_cluster((entry.sfn_entry.first_cluster_hi << 16) + entry.sfn_entry.first_cluster_lo);
        }
        else
        {
            printk("%s is file\n", path);
        }
    }
    return 0;
}

int fat32_read(int fd, char *buf, size_t size)
{
    struct fat32_file *fp;

    if ((fp = fat32_find_file(fd)) == NULL)
    {
        return -1;
    }
    fat32_read_cluster(fp, buf, (size > fp->size) ? fp->size : size);
    return 0;
}

int fat32_open(const char *path)
{
    int ret;
    struct fat32_bpb *bpb;
    struct fat32_dir_entry entry;

    ret = fat32_lookup(path, &entry);
    if (ret < 0)
    {
        printk("'%s': No such file or directory\n", path);
        return ret;
    }
    else if (entry.sfn_entry.attr & 0x10)
    {
        printk("%s is directory\n", path);
    }
    else
    {
        struct fat32_file *fp = kmalloc(sizeof(struct fat32_file));
        fp->fd = g_fd_count++;
        fp->size = entry.sfn_entry.file_size;
        fp->path = kmalloc(strlen(path) + 1);
        memcpy(fp->path, path, strlen(path) + 1);
        list_add_tail(&fp->list, &g_fat32_file_list);

        // 获取 sata 设备端口
        fp->fs.sata_dev = get_sata_device();
        memcpy(&fp->fs.entry, &entry, sizeof(struct fat32_dir_entry));
        bpb = kzalloc(512);
        ahci_read(fp->fs.sata_dev->port, 0, 1, bpb);
        fp->fs.bpb = bpb;
        return fp->fd;
    }
    return -1;
}

int fat32_close(int fd)
{
    struct list_head *pos;
    struct fat32_file *fp;

    list_for_each(pos, &g_fat32_file_list)
    {
        fp = container_of(pos, struct fat32_file, list);
        if (fp->fd == fd)
        {
            kfree(fp->path);
            return 0;
        }
    }
    return -1;
}

uint32_t cluster_to_lba(struct fat32_bpb *bpb, uint32_t cluster)
{
    uint32_t data_start_lba = bpb->reserved_sector_count + (bpb->fat_size_32 * bpb->num_fats);

    if (cluster < 2)
    {
        return 0; // 或返回错误码，簇0/1无对应LBA
    }

    // 数据区起始 + 簇偏移 × 每簇扇区数
    return data_start_lba + (cluster - 2) * bpb->sectors_per_cluster;
}
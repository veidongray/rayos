#include <mm.h>
#include <ahci.h>
#include <list.h>
#include <fat32.h>
#include <printk.h>
#include <stdbool.h>
#include <lib/string/string.h>

#define FAT32_EOC(x) ((x) >= 0x0FFFFFF8)
#define FAT32_LFN_MAX_LEN 512
LIST_HEAD(g_fat32_file_list);

static int g_fd_count = 0;

static inline int fat32_get_parent(const char *path, char *parent)
{
    char *p;
    size_t len;

    len = strlen(path);

    if (len <= 1)
    {
        strcpy(parent, "/");
        return 0;
    }

    strcpy(parent, path);

    p = strrchr(parent, '/');

    if (!p)
        return -1;

    if (p == parent)
        parent[1] = '\0';
    else
        *p = '\0';

    return 0;
}

static inline uint32_t cluster_to_lba(struct fat32_bpb *bpb, uint32_t cluster)
{
    uint32_t data_start_lba = bpb->reserved_sector_count + (bpb->fat_size_32 * bpb->num_fats);

    if (cluster < 2)
    {
        return 0; // 或返回错误码，簇0/1无对应LBA
    }

    // 数据区起始 + 簇偏移 × 每簇扇区数
    return data_start_lba + (cluster - 2) * bpb->sectors_per_cluster;
}

static inline size_t utf16_to_ascii(const uint16_t *utf16_str,
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

static inline void sfn_to_ascii(const char *sfn, char *out)
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

static inline int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - 32;

    return c;
}

static inline int ascii_to_sfn(const char *name, uint8_t sfn[11])
{
    const char *dot;
    size_t base_len;
    size_t ext_len;
    int need_lfn = 0;

    if (!name || !sfn)
        return -1;

    memset(sfn, ' ', 11);

    dot = strrchr(name, '.');

    if (dot)
    {
        base_len = dot - name;
        ext_len = strlen(dot + 1);

        if (base_len > 8 || ext_len > 3)
            need_lfn = 1;
    }
    else
    {
        base_len = strlen(name);
        ext_len = 0;

        if (base_len > 8)
            need_lfn = 1;
    }

    if (!need_lfn)
    {
        /* 原来的8.3逻辑 */

        for (size_t i = 0; i < base_len; i++)
        {
            char ch = toupper((unsigned char)name[i]);
            sfn[i] = ch;
        }

        if (dot)
        {
            for (size_t i = 0; i < ext_len; i++)
            {
                char ch = toupper((unsigned char)dot[1 + i]);
                sfn[8 + i] = ch;
            }
        }

        return 0;
    }

    /* 生成SFN别名 */

    for (size_t i = 0; i < 6 && i < base_len; i++)
    {
        char ch = toupper((unsigned char)name[i]);

        if (ch == ' ')
            ch = '_';

        sfn[i] = ch;
    }

    sfn[6] = '~';
    sfn[7] = '1';

    if (dot)
    {
        for (size_t i = 0; i < 3 && i < ext_len; i++)
        {
            char ch = toupper((unsigned char)dot[1 + i]);
            sfn[8 + i] = ch;
        }
    }

    return 1; /* 返回1表示需要LFN */
}

/*
 * path : 输入路径
 * name : 输出当前路径分量
 *
 * 返回:
 *   NULL -> 结束
 *   非NULL -> 下一个位置
 */
static inline char *path_next(char *path, char *name)
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

static inline bool fat32_is_sfn_char(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return true;

    if (ch >= '0' && ch <= '9')
        return true;

    switch (ch)
    {
    case '$':
    case '%':
    case '\'':
    case '-':
    case '_':
    case '@':
    case '~':
    case '`':
    case '!':
    case '(':
    case ')':
    case '{':
    case '}':
    case '^':
    case '#':
    case '&':
        return true;
    }

    return false;
}

static inline bool fat32_need_lfn(const char *name)
{
    const char *dot;
    size_t base_len;
    size_t ext_len;

    if (!name || !*name)
        return true;

    dot = strrchr(name, '.');

    if (dot)
    {
        base_len = dot - name;
        ext_len = strlen(dot + 1);

        if (base_len == 0)
            return true;

        if (base_len > 8)
            return true;

        if (ext_len > 3)
            return true;
    }
    else
    {
        base_len = strlen(name);

        if (base_len > 8)
            return true;
    }

    while (*name)
    {
        unsigned char ch = *name++;

        if (ch == '.')
            continue;

        /*
         * 小写字母需要LFN
         */
        if (ch >= 'a' && ch <= 'z')
            return true;

        /*
         * 非ASCII字符需要LFN
         */
        if (ch > 0x7f)
            return true;

        /*
         * 非SFN合法字符需要LFN
         */
        if (!fat32_is_sfn_char(ch))
            return true;
    }

    return false;
}

static inline int fat32_lfn_concat(struct fat32_dir_entry *entry, char *buf)
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

static inline int fat32_print_callback(struct lookup_context *ctx)
{
    if (ctx->lfn[0] == '\0')
    {
        sfn_to_ascii(ctx->entry->sfn_entry.name, ctx->lfn);
    }

    if (fat32_is_dot(ctx->entry))
    {
        printk("<d> %u %s\n", ctx->entry->sfn_entry.file_size, ".");
        return 1;
    }
    if (fat32_is_dotdot(ctx->entry))
    {
        printk("<d> %u %s\n", ctx->entry->sfn_entry.file_size, "..");
        return 2;
    }
    if ((uint8_t)ctx->entry->sfn_entry.name[0] != 0xE5)
    {
        if (ctx->entry->sfn_entry.attr & 0x10)
        {
            printk("<d> %u %s\n", ctx->entry->sfn_entry.file_size, ctx->lfn);
        }
        else
        {
            printk("<-> %u %s\n", ctx->entry->sfn_entry.file_size, ctx->lfn);
        }
    }
    return strlen(ctx->lfn);
}

static inline int fat32_compare_callback(struct lookup_context *ctx)
{
    if (ctx->lfn[0] == '\0')
    {
        sfn_to_ascii(ctx->entry->sfn_entry.name, ctx->lfn); // 需要实现 SFN 转 ASCII 函数
    }

    if (strncmp(ctx->target, ctx->lfn, strlen(ctx->lfn) + 1) == 0)
    {
        // 复制找到的文件或者目录的目录项
        memcpy(ctx->result, ctx->entry, sizeof(struct fat32_dir_entry));
        return 0;
    }
    return -1;
}

static inline uint32_t __fat32_alloc_cluster(void)
{
    int nr_fat;
    uint32_t i;
    uint32_t *fat_table;
    uint32_t entries_per_sector;
    struct fat32_bpb *bpb;
    struct sata_device *sata_dev;
    struct sata_controller_port_register *port;

    // 获取 SATA 控制器端口
    sata_dev = get_sata_device();
    port = sata_dev->port;

    // 读取 BPB
    bpb = (struct fat32_bpb *)kzalloc(512);
    ahci_read(port, 0, 1, bpb);

    fat_table = kzalloc(bpb->bytes_per_sector);
    entries_per_sector = bpb->bytes_per_sector / 32;

    for (nr_fat = 0; (uint32_t)nr_fat < bpb->fat_size_32; nr_fat++)
    {
        ahci_read(port, bpb->reserved_sector_count + nr_fat, 1, fat_table);
        for (i = 0; i < entries_per_sector; i++)
        {
            if (fat_table[i] == 0x00000000)
            {
                fat_table[i] = 0x0fffffff;

                // 写回 FAT 表
                for (int j = 0; j < bpb->num_fats; j++)
                {
                    ahci_write(port, bpb->reserved_sector_count + (bpb->fat_size_32 * j) + nr_fat, 1, fat_table);
                }
                kfree(bpb);
                kfree(fat_table);
                return (nr_fat * entries_per_sector) + i;
            }
        }
    }

    kfree(bpb);
    kfree(fat_table);
    return -1;
}

static inline size_t ascii_to_utf16(
    const char *ascii,
    uint16_t *utf16,
    size_t max_chars)
{
    size_t len = 0;

    while (*ascii && len < max_chars)
    {
        utf16[len++] = (uint8_t)*ascii++;
    }

    utf16[len] = 0;

    return len;
}

static inline uint8_t fat32_lfn_checksum(const uint8_t sfn[11])
{
    uint8_t sum = 0;

    for (int i = 0; i < 11; i++)
    {
        sum = ((sum & 1) << 7) + (sum >> 1) + sfn[i];
    }

    return sum;
}

static inline int fat32_make_sfn_entry(struct fat32_dir_entry *entry, const char *fn)
{
    // 创建新的SFN
    ascii_to_sfn(fn, (uint8_t *)entry->sfn_entry.name);
    entry->sfn_entry.attr = ATTR_ARCHIVE;
    entry->sfn_entry.file_size = 0;
    entry->sfn_entry.first_cluster_hi = 0;
    entry->sfn_entry.first_cluster_lo = 0;
    entry->sfn_entry.write_date = 2026;
    entry->sfn_entry.write_time = 19923;
    entry->sfn_entry.last_access_date = 0;
    return 0;
}

static inline int fat32_make_lfn_entry(struct fat32_dir_entry *entry, const char *fn, uint8_t order)
{
    int nr;
    uint16_t name[6];

    ascii_to_utf16(fn, name, 5);
    for (nr = 0; nr < 5; nr++)
    {
        entry->lfn_entry.name1[nr] = name[nr];
    }

    ascii_to_utf16(fn + 5, name, 6);
    for (nr = 0; nr < 6; nr++)
    {
        entry->lfn_entry.name2[nr] = name[nr];
    }

    ascii_to_utf16(fn + 11, name, 2);
    for (nr = 0; nr < 2; nr++)
    {
        entry->lfn_entry.name3[nr] = name[nr];
    }

    entry->lfn_entry.order = order;
    entry->lfn_entry.attr = ATTR_LONG_NAME;
    entry->lfn_entry.type = 0;
    entry->lfn_entry.checksum = 0;
    return 0;
}

int fat32_create(const char *path)
{
    int nr_total_entries;
    int nr_lfn;
    int count;
    char *dir;
    char *dir_step;
    char *fn;
    char *fn_step;
    uint8_t *cluster_buf;
    uint32_t new_cluster;
    uint32_t dir_cluster;
    uint32_t bytes_per_cluster;
    uint32_t *fat_table;
    uint32_t entries_per_cluster;
    struct fat32_bpb *bpb;
    struct sata_device *sata_dev;
    struct fat32_dir_entry *entry;
    struct fat32_dir_entry entry_result;

    // 先判断路径是否存在
    if (!fat32_lookup(path, NULL))
    {
        return -1;
    }
    sata_dev = get_sata_device();

    // 读取 BPB
    bpb = (struct fat32_bpb *)kzalloc(512);
    ahci_read(sata_dev->port, 0, 1, bpb);

    fn = kzalloc(FAT32_LFN_MAX_LEN);
    dir_step = dir = kzalloc(strlen(path));

    memcpy(dir, path, strlen(path) + 1);
    count = strlen(path) - 1;

    // 拿到要创建的文件名
    while ((dir_step = path_next(dir_step, fn)) == NULL)
        ;
    fn_step = fn;

    // 将文件名或者目录名从路径中剔除
    // 寻找根路径
    while (dir[count] != '\0' && dir[count] != '/' && count > 0)
    {
        dir[count] = '\0';
        count--;
    }

    if (dir[0] == '/' && dir[1] == '\0')
    {
        // 判断是否是根目录
        dir_cluster = 2;
    }
    else
    {
        // 查看根路径是否存在
        if (fat32_lookup(dir, &entry_result) < 0)
        {
            kfree(fn);
            kfree(dir);
            return -1;
        }
        // 获取根路径的簇号
        dir_cluster = (entry_result.sfn_entry.first_cluster_hi << 16) + entry_result.sfn_entry.first_cluster_lo;
    }

    bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    cluster_buf = kzalloc(bytes_per_cluster);
    fat_table = kzalloc(bpb->bytes_per_sector);
    nr_lfn = (strlen(fn) / 13) + 1;
    fn_step = fn_step + ((nr_lfn - 1) * 13);
    nr_total_entries = nr_lfn + 1;

    // 每簇可存放的目录项数量
    entries_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster / sizeof(struct fat32_dir_entry);
    while (!FAT32_EOC(dir_cluster))
    {
        ahci_read(sata_dev->port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
        entry = (struct fat32_dir_entry *)cluster_buf;

        for (uint32_t i = 0; i < entries_per_cluster; i++, entry++)
        {
            // 空目录项，目录结束
            if (((uint8_t)entry->sfn_entry.name[0] == SFN_NAME0_FREE) || ((uint8_t)entry->sfn_entry.name[0] == SFN_NAME0_DELETED))
            {
                if (fat32_need_lfn(fn) && (fn_step >= fn))
                {
                    fat32_make_lfn_entry(entry, fn_step, ((nr_total_entries - nr_lfn) == 1) ? (LFN_ORDER_LAST_ENTRY | nr_lfn) : nr_lfn);
                    ahci_write(sata_dev->port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
                }
                else
                {
                    fat32_make_sfn_entry(entry, fn);
                    ahci_write(sata_dev->port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
                    goto done;
                }
                nr_lfn--;
                fn_step = fn_step - 13;
            }
        }

        // 下一簇
        ahci_read(sata_dev->port, bpb->reserved_sector_count + (dir_cluster * sizeof(uint32_t) / bpb->bytes_per_sector), 1, fat_table);
        if (FAT32_EOC(fat_table[(dir_cluster % bpb->bytes_per_sector % (bpb->bytes_per_sector / sizeof(uint32_t)))]))
        {
            // 如果簇末尾了
            // 分配一个新的
            new_cluster = __fat32_alloc_cluster();
            // 因为alloc更新了FAT表所以需要重新读取
            ahci_read(sata_dev->port, bpb->reserved_sector_count + (dir_cluster * sizeof(uint32_t) / bpb->bytes_per_sector), 1, fat_table);
            fat_table[(dir_cluster % bpb->bytes_per_sector % (bpb->bytes_per_sector / sizeof(uint32_t)))] = new_cluster;

            // 写回FAT
            for (int i = 0; i < bpb->num_fats; i++)
            {
                ahci_write(sata_dev->port,
                           bpb->reserved_sector_count + (bpb->fat_size_32 * i) + (dir_cluster * sizeof(uint32_t) / bpb->bytes_per_sector),
                           1, fat_table);
            }
            dir_cluster = new_cluster;

            // 清空新的簇
            memset(cluster_buf, 0, bytes_per_cluster);
            ahci_write(sata_dev->port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
        }
        else
        {
            // 直接从fat读取下一个
            dir_cluster = fat_table[(dir_cluster % bpb->bytes_per_sector % (bpb->bytes_per_sector / sizeof(uint32_t)))];
        }
    }

done:
    kfree(cluster_buf);
    kfree(fn);
    kfree(fat_table);
    kfree(dir);
    return 0;
}

int fat32_read_cluster(struct fat32_file *fp, char *buf, size_t size)
{
    uint32_t cluster;
    char *cluster_buf;
    uint32_t *fat_table;
    uint32_t bytes_per_cluster;
    struct fat32_bpb *bpb;
    struct sata_controller_port_register *port;

    port = fp->fs.sata_dev->port;
    bpb = fp->fs.bpb;

    fat_table = kzalloc(bpb->bytes_per_sector);

    cluster = (fp->fs.entry.sfn_entry.first_cluster_hi << 16) + fp->fs.entry.sfn_entry.first_cluster_lo;
    bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    cluster_buf = kzalloc(bytes_per_cluster);
    while (!FAT32_EOC(cluster))
    {
        ahci_read(port, cluster_to_lba(bpb, cluster), bpb->sectors_per_cluster, cluster_buf);
        if (size > (bytes_per_cluster))
        {
            memcpy(buf, cluster_buf, (bytes_per_cluster));
            size -= (bytes_per_cluster);
            buf = buf + (bytes_per_cluster);
        }
        else
        {
            memcpy(buf, cluster_buf, size);
            goto done;
        }

        // 读取当前 cluster 索引表
        ahci_read(port, bpb->reserved_sector_count + (cluster * sizeof(uint32_t) / bpb->bytes_per_sector), 1, fat_table);
        cluster = fat_table[(cluster % bpb->bytes_per_sector % (bpb->bytes_per_sector / sizeof(uint32_t)))];
    }

done:
    kfree(cluster_buf);
    kfree(fat_table);
    return size;
}

int fat32_write_cluster(struct fat32_file *fp, const char *buf, size_t size)
{
    uint32_t cluster;
    uint8_t *clusbuff;
    uint32_t *fat_table;
    int entries_per_cluster;
    struct fat32_dir_entry *entry;

    clusbuff = kzalloc(fp->fs.bpb->bytes_per_sector * fp->fs.bpb->sectors_per_cluster);
    fat_table = kzalloc(fp->fs.bpb->fat_size_32 * fp->fs.bpb->bytes_per_sector);
    entries_per_cluster = (fp->fs.bpb->bytes_per_sector * fp->fs.bpb->sectors_per_cluster) / 32;
    if (((fp->fs.entry.sfn_entry.first_cluster_hi << 16) + fp->fs.entry.sfn_entry.first_cluster_lo) == 0)
    {
        // 第一次写入，需要先找可用簇
        cluster = __fat32_alloc_cluster();
        fp->fs.entry.sfn_entry.first_cluster_hi = (cluster & 0xffff0000) >> 16;
        fp->fs.entry.sfn_entry.first_cluster_lo = cluster & 0x0000ffff;

        // 写回目录项内容
        cluster = (fp->fs.parent_entry.sfn_entry.first_cluster_hi << 16) + fp->fs.parent_entry.sfn_entry.first_cluster_lo;
        while (!FAT32_EOC(cluster))
        {
            // 查找对应的项的簇
            ahci_read(fp->fs.sata_dev->port, cluster_to_lba(fp->fs.bpb, cluster), fp->fs.bpb->sectors_per_cluster, clusbuff);
            entry = (struct fat32_dir_entry *)clusbuff;

            for (int i = 0; i < entries_per_cluster; i++, entry++)
            {
                if (strncmp(entry->sfn_entry.name, fp->fs.entry.sfn_entry.name, 11) == 0)
                {
                    fp->fs.entry.sfn_entry.file_size = size;
                    memcpy(entry, &fp->fs.entry, sizeof(struct fat32_dir_entry));
                    ahci_write(fp->fs.sata_dev->port, cluster_to_lba(fp->fs.bpb, cluster), fp->fs.bpb->sectors_per_cluster, clusbuff);
                    goto done;
                }
            }
            // 读取当前 cluster 索引表
            ahci_read(fp->fs.sata_dev->port, fp->fs.bpb->reserved_sector_count + (cluster * sizeof(uint32_t) / fp->fs.bpb->bytes_per_sector), 1, fat_table);
            cluster = fat_table[(cluster % fp->fs.bpb->bytes_per_sector % (fp->fs.bpb->bytes_per_sector / sizeof(uint32_t)))];
        }
    }

done:
    cluster = (fp->fs.entry.sfn_entry.first_cluster_hi << 16) + fp->fs.entry.sfn_entry.first_cluster_lo;
    uint32_t bytes_per_cluster = fp->fs.bpb->bytes_per_sector * fp->fs.bpb->sectors_per_cluster;
    uint32_t new_cluster;
    for (char *b = (char *)buf; b < (buf + size); b += (fp->fs.bpb->bytes_per_sector * fp->fs.bpb->sectors_per_cluster))
    {
        // 一次写入一个簇
        ahci_write(fp->fs.sata_dev->port,
                   cluster_to_lba(fp->fs.bpb, cluster),
                   fp->fs.bpb->sectors_per_cluster, b);
        // 下一簇
        ahci_read(fp->fs.sata_dev->port, fp->fs.bpb->reserved_sector_count + (cluster * sizeof(uint32_t) / fp->fs.bpb->bytes_per_sector), 1, fat_table);
        if (FAT32_EOC(fat_table[(cluster % fp->fs.bpb->bytes_per_sector % (fp->fs.bpb->bytes_per_sector / sizeof(uint32_t)))]))
        {
            // 如果簇末尾了
            // 分配一个新的
            new_cluster = __fat32_alloc_cluster();
            // 因为alloc更新了FAT表所以需要重新读取
            ahci_read(fp->fs.sata_dev->port, fp->fs.bpb->reserved_sector_count + (cluster * sizeof(uint32_t) / fp->fs.bpb->bytes_per_sector), 1, fat_table);
            fat_table[(cluster % fp->fs.bpb->bytes_per_sector % (fp->fs.bpb->bytes_per_sector / sizeof(uint32_t)))] = new_cluster;

            // 写回FAT
            for (int i = 0; i < fp->fs.bpb->num_fats; i++)
            {
                ahci_write(fp->fs.sata_dev->port,
                           fp->fs.bpb->reserved_sector_count + (fp->fs.bpb->fat_size_32 * i) + (cluster * sizeof(uint32_t) / fp->fs.bpb->bytes_per_sector),
                           1, fat_table);
            }
            cluster = new_cluster;

            // 清空新的簇
            memset(clusbuff, 0, bytes_per_cluster);
            ahci_write(fp->fs.sata_dev->port, cluster_to_lba(fp->fs.bpb, cluster), fp->fs.bpb->sectors_per_cluster, clusbuff);
        }
        else
        {
            // 直接从fat读取下一个
            cluster = fat_table[(cluster % fp->fs.bpb->bytes_per_sector % (fp->fs.bpb->bytes_per_sector / sizeof(uint32_t)))];
        }
    }
    kfree(clusbuff);
    kfree(fat_table);
    return 0;
}

/**
 * 根据指定的 cluster 在对应目录中查找
 */
int fat32_foreach_dirent(fat32_foreach_dirent_callback_t cb, uint32_t dir_cluster, const char *target, struct fat32_dir_entry *result)
{
    int ret;
    char *cluster_buf = NULL;
    char *lfn = NULL;
    uint32_t *fat_table;
    uint32_t entries_per_cluster;
    struct fat32_dir_entry *entry = NULL;
    struct sata_device *sata_dev;
    struct fat32_bpb *bpb;
    struct lookup_context ctx;
    struct sata_controller_port_register *port;

    // 获取 SATA 控制器端口
    sata_dev = get_sata_device();
    port = sata_dev->port;

    // 读取 BPB
    bpb = (struct fat32_bpb *)kzalloc(512);
    ahci_read(port, 0, 1, bpb);

    // 读取 FAT 表
    fat_table = kzalloc(bpb->bytes_per_sector);

    // 每簇可存放的目录项数量
    entries_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster / sizeof(struct fat32_dir_entry);

    cluster_buf = kzalloc(bpb->bytes_per_sector * bpb->sectors_per_cluster);
    lfn = kzalloc(FAT32_LFN_MAX_LEN);

    while (!FAT32_EOC(dir_cluster))
    {
        ahci_read(port, cluster_to_lba(bpb, dir_cluster), bpb->sectors_per_cluster, cluster_buf);
        entry = (struct fat32_dir_entry *)cluster_buf;

        for (uint32_t i = 0; i < entries_per_cluster; i++, entry++)
        {
            // 空目录项，目录结束
            if (entry->sfn_entry.name[0] == 0x00)
            {
                ret = -1;
                goto not_found;
            }

            // 已删除的目录项
            if ((uint8_t)entry->sfn_entry.name[0] == 0xE5)
                continue;

            // 长文件名
            if (entry->lfn_entry.attr == 0x0F)
            {
                fat32_lfn_concat(entry, lfn);
            }
            else
            {
                // SFN 文件名（没有 LFN）
                ctx.entry = entry;
                ctx.lfn = lfn;
                ctx.result = result;
                ctx.target = (char *)target;
                if ((ret = cb(&ctx)) == 0)
                {
                    goto done;
                }
                memset(lfn, 0, FAT32_LFN_MAX_LEN);
            }
        }

        // 下一簇
        ahci_read(port, bpb->reserved_sector_count + (dir_cluster * sizeof(uint32_t) / bpb->bytes_per_sector), 1, fat_table);
        dir_cluster = fat_table[(dir_cluster % bpb->bytes_per_sector % (bpb->bytes_per_sector / sizeof(uint32_t)))];
    }

not_found:
done:
    kfree(cluster_buf);
    kfree(fat_table);
    kfree(lfn);
    kfree(bpb);
    return ret;
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
        ret = fat32_foreach_dirent(fat32_compare_callback, cluster, name, &entry);
        if (ret < 0)
        {
            entry_result = NULL;
            kfree(name);
            return ret;
        }
        cluster = (entry.sfn_entry.first_cluster_hi << 16) + entry.sfn_entry.first_cluster_lo;
    }

    // 成功查找复制 entry 值
    if (entry_result != NULL)
    {
        memcpy(entry_result, &entry, sizeof(struct fat32_dir_entry));
    }
    kfree(name);
    return 0;
}

int fat32_readdir(const char *path)
{
    int ret;
    struct fat32_dir_entry entry;

    if (path[0] == '/' && path[1] == '\0')
    {
        // 判断是否是读取根目录
        fat32_foreach_dirent(fat32_print_callback, 2, NULL, NULL);
    }
    else
    {
        ret = fat32_lookup(path, &entry);
        if (ret < 0)
        {
            printk("'%s': No such file or directory\n", path);
            return -1;
        }
        if (entry.sfn_entry.attr & 0x10)
        {
            fat32_foreach_dirent(fat32_print_callback,
                                 (entry.sfn_entry.first_cluster_hi << 16) + entry.sfn_entry.first_cluster_lo,
                                 NULL, NULL);
        }
        else
        {
            printk("%s is file\n", path);
            return -1;
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
    return fat32_read_cluster(fp, buf, size);
}

int fat32_write(int fd, const char *buf, size_t size)
{
    struct fat32_file *fp;

    if ((fp = fat32_find_file(fd)) == NULL)
    {
        return -1;
    }
    return fat32_write_cluster(fp, buf, size);
}

int fat32_open(const char *path)
{
    int ret;
    char *parent;
    struct fat32_bpb *bpb;
    struct fat32_dir_entry entry;
    struct fat32_dir_entry parent_entry;

    // 记录路径的父目录
    parent = kmalloc(strlen(path));
    fat32_get_parent(path, parent);

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
        struct fat32_file *fp = kzalloc(sizeof(struct fat32_file));
        fp->fd = g_fd_count++;
        fp->size = entry.sfn_entry.file_size;
        fp->path = kzalloc(strlen(path) + 1);
        memcpy(fp->path, path, strlen(path) + 1);
        list_add_tail(&fp->list, &g_fat32_file_list);

        // 获取 sata 设备端口
        fp->fs.sata_dev = get_sata_device();
        memcpy(&fp->fs.entry, &entry, sizeof(struct fat32_dir_entry));

        if ((strlen(parent) == 1) && (strncmp(parent, "/", 1) == 0))
        {
            fp->fs.parent_entry.sfn_entry.name[0] = '/';
            fp->fs.parent_entry.sfn_entry.first_cluster_lo = 2;
        }
        else
        {
            fat32_lookup(parent, &parent_entry);
            memcpy(&fp->fs.parent_entry, &parent_entry, sizeof(struct fat32_dir_entry));
        }

        bpb = kzalloc(512);
        ahci_read(fp->fs.sata_dev->port, 0, 1, bpb);
        fp->fs.bpb = bpb;
        memcpy(fp->fs.bpb, bpb, sizeof(struct fat32_bpb));
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

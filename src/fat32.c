#include <mm.h>
#include <ahci.h>
#include <fat32.h>
#include <printk.h>
#include <lib/string/string.h>

int fat32_readdir(const char *path)
{
    char *path_name;
    char *first_sector;
    uint8_t *cluster_buffer;
    uint32_t *fat;
    struct fat32_bpb *bpb;
    struct sata_device *sata_dev;
    struct fat32_dir_entry *entry;
    struct hba_memory_registers *hba;
    struct sata_controller_port_register *port;

    path_name = (char *)path;
    if (path_name[0] != '/')
        return -1;

    path_name++;

    sata_dev = get_sata_device();
    hba = get_host_bus_adapter();
    port = &hba->ports[sata_dev->port_no];

    /**
     * 先拿到bpb
     */
    first_sector = (char *)kmalloc(512);
    ahci_read(port, 0, 1, first_sector);
    bpb = (struct fat32_bpb *)first_sector;

    /**
     * 获取fat
     */
    fat = (uint32_t *)kmalloc(512);
    ahci_read(port, bpb->reserved_sector_count, 1, fat);

    cluster_buffer = kmalloc(bpb->sectors_per_cluster * bpb->bytes_per_sector);
    if (path_name[0] == '\0')
    {
        // 如果已经是读取路径的末尾
        // 则获取该目录所有文件名
        ahci_read(port, bpb->reserved_sector_count + (bpb->fat_size_32 * bpb->num_fats), bpb->sectors_per_cluster, cluster_buffer);
        entry = (struct fat32_dir_entry *)cluster_buffer;
        while (1)
        {
            if (LFN_IS_LAST(&entry->lfn_entry))
            {
                entry++;
                break;
            }
            if (FAT_IS_LFN_ENTRY(&entry->lfn_entry))
            {
                entry++;
                continue;
            }
            entry++;
        }
        printk("%s\n", entry->sfn_entry.name);
    }
    return 0;
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
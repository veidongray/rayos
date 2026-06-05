#include <mm.h>
#include <list.h>
#include <ahci.h>
#include <page.h>
#include <fat32.h>
#include <align.h>
#include <printk.h>
#include <lib/string/string.h>

__attribute__((aligned(256))) static uint8_t fis_buffer[256];
__attribute__((aligned(1024))) static struct ahci_cmd_list_entry cl_buffer[32]; // 32个Slot
__attribute__((aligned(4096))) static struct ahci_cmd_table cmd_table_buffer;   // 升级为页级命令表
static struct sata_device *sata_dev;
static struct hba_memory_registers *hba;

struct sata_device *get_sata_device(void)
{
    return sata_dev;
}

struct hba_memory_registers *get_host_bus_adapter(void)
{
    return hba;
}

int ahci_dma_transfer(int write, struct sata_controller_port_register *port, uint64_t lba, uint16_t count, void *target_buf_virt)
{
    /*
    [HBA Port 寄存器]
         │
         ▼ (PxCLB 物理指针指向...)
  ┌──────────────────────────────────────────────────────────┐
  │ 1024 字节的 Command List (命令列表，共 32 个 Slot 挂钩)   │
  ├──────────────┬──────────────┬──────────────┬─────────────┤
  │   Slot 0     │   Slot 1     │   Slot 2     │    ...      │
  └──────┬───────┴──────┬───────┴──────┬───────┴─────────────┘
         │              │              │
         │ (ctba 指针)  └─► 指向另一个独立的 Table...
         ▼
  ┌──────────────────────────────────────────────────────────┐
  │ 某个具体的 Command Table (命令表，大小开足 4KB 页)       │
  ├──────────────────────────────────────────────────────────┤
  │  1. CFIS (Command FIS) ──► 主机写给硬盘的指令条 (如读/写) │
  ├──────────────────────────────────────────────────────────┤
  │  2. ACMD               ──► ATAPI 光驱高级命令 (日常清零)  │
  ├──────────────────────────────────────────────────────────┤
  │  3. PRDT (散集表阵列)  ──► 数据的真正目的地               │
  │     ├── prdt[0] ──► [物理地址 DBA] + [操作字节数 DBC] ───┼─► 丢进内核的 sector_data
  │     └── prdt[1] ──► [物理地址 DBA] + [操作字节数 DBC] ───┼─► (若有多块内存，继续往后接...)
  └──────────────────────────────────────────────────────────┘
    */

    // 确保任何历史挂起中断已被冲刷掉
    port->PxIS = 0xFFFFFFFFU;

    // 使用结构体 Slot 0
    struct ahci_cmd_list_entry *cmd_hdr = &cl_buffer[0];

    // 配置 Slot 0 的命令头基本属性
    cmd_hdr->opts =
        5 | ((write ? 1 : 0) << 6); // CFL = 5 dwords (20 字节); W = 0 (读命令)
    cmd_hdr->prdtl = 1;             // 只有 1 个 PRDT 搬运目的地
    cmd_hdr->prdbc = 0;             // 硬件清零计数器

    // 通过结构体直接、干净地定位到 CFIS 空间，不改变其物理绑定
    struct fis_reg_h2d *cfis = (struct fis_reg_h2d *)(cmd_table_buffer.cfis);
    memset(cfis, 0, sizeof(struct fis_reg_h2d));

    cfis->fis_type = 0x27;    // Register FIS - Host to Device
    cfis->pmport_c = 1U << 7; // 指明这是一条指令
    cfis->command =
        write ? 0x35 : 0x25; // READ SECTORS EXT (LBA48 读)
    cfis->device = 1U << 6;  // 启用 LBA 寻址模式

    // 填入 48 位 LBA 地址
    cfis->lba0 = (uint8_t)(lba & 0xFF);
    cfis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cfis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cfis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cfis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cfis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    // 填入需要读取的扇区数
    cfis->count_low = (uint8_t)(count & 0xFF);
    cfis->count_high = (uint8_t)((count >> 8) & 0xFF);

    // 使用结构体 prdt[0] 写入，彻底告别裸物理地址加算带来的惊悚
    uint64_t target_phys = get_physaddr((uint64_t)target_buf_virt);
    cmd_table_buffer.prdt[0].dba = (uint32_t)(target_phys & 0xFFFFFFFFU);
    cmd_table_buffer.prdt[0].dbau = (uint32_t)((target_phys >> 32) & 0xFFFFFFFFU);
    cmd_table_buffer.prdt[0].reserved0 = 0;
    // 传输控制：(字节数 - 1) | 传输完毕中断允许 (Bit 31)
    cmd_table_buffer.prdt[0].dbc = ((count * 512 - 1) & 0x3FFFFFU) | (1U << 31);

    // 等待硬盘空闲
    while (port->PxTFD & ((1U << 7) | (1U << 3)))
        ;

    // 清空历史错误信息，防止拒绝接收新命令
    port->PxSERR = 0xFFFFFFFFU;

    // 临门一脚：向 Slot 0 扔飞镖！
    port->PxCI = (1U << 0);

    // 轮询等待硬件回收飞镖
    while (1)
    {
        if ((port->PxCI & (1U << 0)) == 0)
        {
            break; // 成功！硬件已经完成搬运
        }
        // 保险报错退出机制
        if (port->PxIS & (1U << 30))
        {
            return -1;
        }
    }
    return 0;
}

int ahci_read(struct sata_controller_port_register *port, uint64_t lba, uint16_t count, void *target_buf_virt)
{
    return ahci_dma_transfer(
        0,
        port,
        lba,
        count,
        target_buf_virt);
}

int ahci_write(struct sata_controller_port_register *port, uint64_t lba, uint16_t count, void *target_buf_virt)
{
    return ahci_dma_transfer(
        1,
        port,
        lba,
        count,
        target_buf_virt);
}

// 判定函数
sata_dev_t ahci_check_device_type(volatile uint32_t signature)
{
    // 根据签名精准断定
    switch (signature)
    {
    case 0x00000101:
        return SATA_DEV_SATA;
    case 0xEB140101:
        return SATA_DEV_SATAPI;
    case 0xC33C0101:
        return SATA_DEV_SEMB;
    case 0x96690101:
        return SATA_DEV_PM;
    default:
        return SATA_DEV_NONE;
    }
}

void ahci_init(uintptr_t ahci_base)
{
    size_t nr_ports;
    uint32_t port_implements;

    printk("AHCI base %#llx\n", ahci_base);
    hba = (struct hba_memory_registers *)ahci_base;
    map_page_range((uint64_t)ahci_base, (uint64_t)ahci_base, 0x1b, (sizeof(struct hba_memory_registers) >> PAGE_SHIFT) + 1);

    // 全局重置与激活 AHCI
    hba->ghc.ghc |= (1U << 31);
    hba->ghc.ghc |= (1U << 0);
    while (hba->ghc.ghc & 0x1U)
        ;
    hba->ghc.ghc |= (1U << 31);

    nr_ports = (hba->ghc.cap & 0x1f) + 1;
    port_implements = hba->ghc.pi;
    printk("AHCI nr_ports %u\n", nr_ports);
    printk("AHCI port_implements %lx\n", port_implements);

    for (size_t i = 0; i < nr_ports; i++)
    {
        if (!(port_implements & (1U << i)))
            continue; // 如果主板没有实现这个端口，不要动它

        hba->ports[i].PxCMD |= (1U << 4); // 临时开启 FRE 以读取签名

        // 软延时给电气握手留出微秒级缓冲
        for (volatile int d = 0; d < 2000000; d++)
            ;

        if (((hba->ports[i].PxSSTS & 0xfU) == 0x3U) && (((hba->ports[i].PxSSTS & 0xf00U) >> 8) == 0x1U))
        {
            sata_dev_t dev_type = ahci_check_device_type(hba->ports[i].PxSIG);
            if (dev_type == SATA_DEV_SATA)
            {
                printk("[Port %d] -> Found SATA Hard Disk (HDD/SSD).\n", i);

                // 停止端口的 DMA 状态机
                hba->ports[i].PxCMD &= ~(1U << 0);
                hba->ports[i].PxCMD &= ~(1U << 4);

                while (1)
                {
                    if (hba->ports[i].PxCMD & (1U << 15))
                        continue;
                    if (hba->ports[i].PxCMD & (1U << 14))
                        continue;
                    break;
                }

                // 分配并配置 Received FIS 物理基地址
                uint64_t fis_phys = get_physaddr((uint64_t)fis_buffer);
                hba->ports[i].PxFB = (uint32_t)(fis_phys & 0xFFFFFFFFU);
                hba->ports[i].PxFBU = (uint32_t)((fis_phys >> 32) & 0xFFFFFFFFU);
                memset(fis_buffer, 0, 256);

                // 分配并配置 Command List 物理基地址
                uint64_t cl_phys = get_physaddr((uint64_t)cl_buffer);
                hba->ports[i].PxCLB = (uint32_t)(cl_phys & 0xFFFFFFFFU);
                hba->ports[i].PxCLBU = (uint32_t)((cl_phys >> 32) & 0xFFFFFFFFU);
                memset(cl_buffer, 0, sizeof(cl_buffer));

                // 绑定 Command Table 物理基地址
                uint64_t ctba_phys = get_physaddr((uint64_t)&cmd_table_buffer);
                memset(&cmd_table_buffer, 0, sizeof(struct ahci_cmd_table));

                cl_buffer[0].ctba = (uint32_t)(ctba_phys & 0xFFFFFFFFU);
                cl_buffer[0].ctbau = (uint32_t)((ctba_phys >> 32) & 0xFFFFFFFFU);

                hba->ports[i].PxSERR = 0xFFFFFFFFU;

                // 按顺序唤醒端口引擎
                hba->ports[i].PxCMD |= (1U << 4); // FRE 第一步
                hba->ports[i].PxCMD |= (1U << 0); // ST 第二步

                if ((hba->ports[i].PxCMD & (1U << 15)) && (hba->ports[i].PxCMD & (1U << 14)))
                {
                    sata_dev = kmalloc(sizeof(struct sata_device));
                    sata_dev->hba = hba;
                    sata_dev->port_no = i;
                    sata_dev->port = &hba->ports[i];

                    for (int i = 0; i < 32; i++)
                    {
                        char buffer[32];
                        sprintf(buffer, "/mylonglonglonglong%d.txt", i);
                        fat32_create(buffer);
                    }
                    fat32_readdir("/");
                    char *buf = kzalloc(100);
                    int fd = fat32_open("/filenamelarge0");
                    // fat32_write(fd, "KKKKKKKKKKKKKKKKKKK", 20);
                    fat32_read(fd, buf, 16);
                    printf("%s", buf);
                    fat32_close(fd);
                    kfree(buf);
                }
                else
                {
                    printk("[Port %d] -> DMA Engine failed to start!\n", i);
                }
            }
            else if (dev_type == SATA_DEV_SATAPI)
            {
                printk("[Port %d] -> Found SATAPI CD-ROM. Ignore for now.\n", i);
            }
            else
            {
                printk("[Port %d] -> No valid SATA device or device busy (SIG: 0x%08X).\n", i, hba->ports[i].PxSIG);
            }
        }
    }
}
#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

#define PxSIG_SATA 0x00000101
#define PxSIG_CD_DVD 0xeb140101

/**
 * AHCI 全局通用主机控制寄存器组 (Generic Host Control)
 * 位于 BAR5 (ABAR) 的最开始部分 (偏移 0x00 ~ 0x2B)
 */
struct generic_host_control
{
    uint32_t cap; // 0x00: Host Capabilities (主机能力寄存器)
                  //       - Bit 0~4 : NP (Number of Ports) 该芯片支持的最大端口数-1
                  //       - Bit 8~12: NCS (Number of Command Slots) 每个端口支持的命令槽数-1
                  //       - Bit 31  : 64位寻址能力 (1=支持64位物理地址)

    uint32_t ghc; // 0x04: Global Host Control (全局主机控制寄存器)
                  //       - Bit 31  : AE (AHCI Enable) 全局AHCI使能总开关
                  //       - Bit 1   : IE (Interrupt Enable) 全局中断允许开关
                  //       - Bit 0   : HR (HBA Reset) 软件触发整个HBA芯片全局复位

    uint32_t is; // 0x08: Interrupt Status (全局中断状态寄存器)
                 //       - 32位图，哪一位亮了代表哪一个对应的 Port 触发了中断

    uint32_t pi; // 0x0C: Ports Implemented (已实现端口掩码位图)
                 //       - 32位图，某位为1代表主板上物理焊接/实现了该SATA端口
                 //       - 驱动应当且仅能初始化这些位为1的端口

    uint32_t vs; // 0x10: AHCI Version (AHCI 规范版本号寄存器)
                 //       - 例如: 0x00010301 代表 AHCI 版本为 1.3.1

    uint32_t ccc_ctl; // 0x14: Command Completion Coalescing Control (命令完成聚合控制寄存器)
                      //       - 优化中断频率用，减少高并发读写时的硬件中断次数

    uint32_t ccc_ports; // 0x18: Command Completion Coalescing Ports (命令完成聚合端口图)

    uint32_t em_loc; // 0x1C: Enclosure Management Location (机箱管理缓冲区位置)

    uint32_t em_ctl; // 0x20: Enclosure Management Control (机箱管理控制寄存器)

    uint32_t cap2; // 0x24: Host Capabilities Extended (扩展主机能力寄存器)
                   //       - 包含是否支持睡眠、BOSH、SATA 6G速率等现代特性

    uint32_t bohc; // 0x28: BIOS/OS Handoff Control and Status (BIOS/OS 软硬件交接控制状态寄存器)
                   //       - 用于在开机时，从主板BIOS手中安全夺取AHCI控制权
} __attribute__((packed));

/**
 * AHCI 单个端口独立的寄存器组 (Port Registers)
 * 每个端口固定占用 0x80 (128字节) 空间
 */
struct port_register
{
    // === DMA 基地址寄存器 ===
    uint32_t PxCLB;  // 0x00: Command List Base Address (命令列表基物理地址 - 低32位)
                     //       - 必须 1024 字节对齐！
    uint32_t PxCLBU; // 0x04: Command List Base Address Upper (命令列表基物理地址 - 高32位)

    uint32_t PxFB;  // 0x08: FIS Base Address (接收FIS缓冲区基物理地址 - 低32位)
                    //       - 必须 256 字节对齐！
    uint32_t PxFBU; // 0x0C: FIS Base Address Upper (接收FIS缓冲区基物理地址 - 高32位)

    // === 中断控制 ===
    uint32_t PxIS; // 0x10: Interrupt Status (端口中断状态寄存器)
                   //       - 往对应位写1来清除该端口的挂起中断

    uint32_t PxIE; // 0x14: Interrupt Enable (端口中断允许寄存器)
                   //       - Bit 0: DHRE (Device to Host Register FIS) 收到硬盘响应时中断
                   //       - Bit 5: DPE (Descriptor Processed) DMA数据搬运完成时中断

    // === 状态与控制 (重要) ===
    uint32_t PxCMD; // 0x18: Command and Status (端口命令与状态寄存器)
                    //       - Bit 15: CR (Command List Running) 硬件命令列表引擎正在运转
                    //       - Bit 14: FR (FIS Receive Running) 硬件FIS接收引擎正在运转
                    //       - Bit 4 : FRE (FIS Receive Enable) 允许接收来自硬盘的数据包（点名和读写前必须开启）
                    //       - Bit 0 : ST (Start) 启动当前端口的DMA状态机引擎（筑巢完成后开启）

    uint32_t reserved0; // 0x1C: 保留空间

    uint32_t PxTFD; // 0x20: Task File Data (任务文件数据寄存器)
                    //       - 映射传统ATA状态。Bit 7 (BSY) 为1代表硬盘忙，Bit 0 (ERR) 为1代表出错

    uint32_t PxSIG; // 0x24: Signature (端口特征签名寄存器)
                    //       - 握手成功后，硬件自动填写设备类型
                    //       - 0x00000101 = SATA 硬盘, 0xEB140101 = SATAPI 光驱

    uint32_t PxSSTS; // 0x28: SATA Status (SATA 物理层状态寄存器 - 核心)
                     //       - Bit 0~3 : DET (Device Detection) 3=检测到设备且物理层通信成功
                     //       - Bit 8~11: IPM (Interface Power Management) 1=设备处于活跃供电状态

    uint32_t PxSCTL; // 0x2C: SATA Control (SATA 物理层控制寄存器)
                     //       - 可以通过往此寄存器写值来强制触发当前端口线路的物理复位

    uint32_t PxSERR; // 0x30: SATA Error (SATA 错误寄存器)
                     //       - 记录物理层各种电气和协议报错。初始化时**必须写入 0xFFFFFFFF 刷清**

    uint32_t PxSACT; // 0x34: SATA Active (SATA 激活寄存器)
                     //       - 在 NCQ (原生命令队列) 模式下进行多命令并发读写时使用

    uint32_t PxCI; // 0x38: Command Issue (命令发出寄存器 - 扔飞镖的地方)
                   //       - 32位图对应32个命令槽。往 Bit 0 写 1 意味着告诉硬件：“Slot 0 的读盘命令已准备好，立刻执行！”
                   //       - 硬件搬运完毕后，硬件会自动把这一位清零

    uint32_t PxSNTF; // 0x3C: SATA Notification (SATA 通知寄存器 - 异步通知用)

    uint32_t PxFBS; // 0x40: FIS-based Switching Control (基于FIS切换控制寄存器)

    uint32_t PxDEVSLP; // 0x44: Device Sleep (设备深度睡眠省电控制)

    uint8_t reserved1[40]; // 0x48 ~ 0x6F: 供应商保留空间

    uint32_t PxVS[4]; // 0x70 ~ 0x7F: Vendor Specific (厂商自定义寄存器扩展)
} __attribute__((packed));

/**
 * 完整的 AHCI HBA 内存映射寄存器空间总表
 * 完美契合 QEMU q35 模拟出的实际物理内存布局 (总大小约为 0x1100 字节)
 */
struct hba_memory_registers
{
    struct generic_host_control ghc; // 0x000 ~ 0x02B: 全局控制区

    uint8_t reserved[52]; // 0x02C ~ 0x05F: 协议保留空间

    uint8_t reserved_for_nvmhci[64]; // 0x060 ~ 0x09F: 为NVMHCI(早期的NVMe规范草案)预留的空间

    uint8_t vendor_specific_registers[96]; // 0x0A0 ~ 0x0FF: 芯片厂商(如Intel)自定义的扩展寄存器

    struct port_register ports[32]; // 0x100 ~ 0x10FF: 32个完全平铺连续的端口寄存器阵列！
                                    //       - 第0个端口的基地址在 abar + 0x100
                                    //       - 第1个端口的基地址在 abar + 0x180
} __attribute__((packed));

// 定义设备类型枚举
typedef enum
{
    SATA_DEV_NONE = 0,
    SATA_DEV_SATA,   // 标准硬盘 (SATA HDD/SSD)
    SATA_DEV_SATAPI, // 光驱 (CD-ROM)
    SATA_DEV_SEMB,   // 桥接管理设备
    SATA_DEV_PM      // 端口多路复用器
} sata_dev_t;

// 单个命令槽描述符 (Command List Slot) - 共 32 字节
struct ahci_cmd_list_entry
{
    uint16_t opts;  // Bit 0~4: CFL (CFIS 长度), Bit 6: W (1=写, 0=读)
    uint16_t prdtl; // PRDT 散集表条目数量
    uint32_t prdbc; // 硬件自动填写的已传输完成字节计数
    uint32_t ctba;  // Command Table Base Address (低 32 位物理地址，128字节对齐)
    uint32_t ctbau; // Command Table Base Address (高 32 位物理地址)
    uint32_t reserved[4];
} __attribute__((packed));

// 标准的主机到设备寄存器包 (Host to Device Register FIS) - 共 20 字节
struct fis_reg_h2d
{
    uint8_t fis_type; // 固定为 0x27
    uint8_t pmport_c; // Bit 7 为 1 代表命令
    uint8_t command;  // ATA 命令码 (读: 0x25, 写: 0x35)
    uint8_t features_low;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device; // LBA模式下固定为 0x40 (1 << 6)
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t features_high;
    uint8_t count_low;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;
    uint8_t reserved[4];
} __attribute__((packed));

// 散集表条目 (PRDT Entry) - 共 16 字节
struct ahci_prdt_entry
{
    uint32_t dba;  // 数据块物理基地址 (低 32 位)
    uint32_t dbau; // 数据块物理基地址 (高 32 位)
    uint32_t reserved0;
    uint32_t dbc; // 传输字节数。最高位 Bit 31 为 1 代表传输完成触发中断
} __attribute__((packed));

// 完整的命令表结构体 (Command Table) - 开足 4096 字节安全空间
struct ahci_cmd_table
{
    uint8_t cfis[64];                 // 0x00 ~ 0x3F: 容纳各种类型的 CFIS 包 (包含上面的 fis_reg_h2d)
    uint8_t acmd[32];                 // 0x40 ~ 0x5F: ATAPI 命令空间
    uint8_t reserved[32];             // 0x6F ~ 0x7F: 保留
    struct ahci_prdt_entry prdt[128]; // 0x80 开始: 散集表阵列，支持多块内存片段
} __attribute__((packed));

// 判定函数
sata_dev_t ahci_check_device_type(volatile uint32_t signature);
void ahci_init(uintptr_t ahci_base);
int ahci_read(struct port_register *port, uint64_t lba, uint16_t count, void *target_buf_virt);
int ahci_write(struct port_register *port, uint64_t lba, uint16_t count, void *target_buf_virt);

#endif // AHCI_H
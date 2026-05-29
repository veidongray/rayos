#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

struct acpi_rsdp
{
    /* ================= ACPI 1.0 基础字段 (前 20 字节) ================= */
    char signature[8];     // 签名，固定为 "RSD PTR "
    uint8_t checksum;      // 基础表头的校验和
    char oem_id[6];        // 生产厂商 ID (OEM ID)
    uint8_t revision;      // ACPI 版本号
    uint32_t rsdt_address; // 32位 RSDT 表的物理地址

    /* ================= ACPI 2.0+ 扩展字段 (后 16 字节) ================= */
    uint32_t length;           // 整个 RSDP 结构体的长度（包含扩展部分）
    uint64_t xsdt_address;     // 64位 XSDT 表的物理地址
    uint8_t extended_checksum; // 包含扩展字段在内的全表校验和
    uint8_t reserved[3];       // 保留字段，填充为 0
} __attribute__((packed));

struct acpi_sdt_header
{
    char signature[4];         // 表签名，如 "XSDT", "RSDT", "MCFG"
    uint32_t length;           // 整个表（含表头）的字节长度
    uint8_t revision;          // 表版本号
    uint8_t checksum;          // 校验和：所有字节相加必须为 0
    char oem_id[6];            // OEM 标识符
    char oem_table_id[8];      // OEM 表 ID
    uint32_t oem_revision;     // OEM 修订号
    uint32_t creator_id;       // 创建者 ID
    uint32_t creator_revision; // 创建者修订号
} __attribute__((packed));     // 必须紧凑对齐，共 36 字节

struct acpi_rsdt
{
    struct acpi_sdt_header header; // signature = "RSDT"
    uint32_t entries[];            // 可变长度数组，每个元素是其他 SDT 的 32位物理地址
} __attribute__((packed));

struct acpi_xsdt
{
    struct acpi_sdt_header header; // signature = "XSDT"
    uint64_t entries[];            // 可变长度数组，每个元素是其他 SDT 的 64位物理地址
} __attribute__((packed));

// MCFG 专属条目（每个条目 16 字节）
struct mcfg_entry
{
    uint64_t base_address;  // ECAM 物理基地址（必须 4KB 对齐）
    uint16_t segment_group; // PCI Segment Group 编号
    uint8_t bus_start;      // 该条目覆盖的起始 Bus 号
    uint8_t bus_end;        // 该条目覆盖的结束 Bus 号（含）
    uint32_t reserved;      // 保留字段，必须为 0
} __attribute__((packed));

// 完整的 MCFG 表
struct acpi_mcfg
{
    struct acpi_sdt_header header; // 签名 = "MCFG"
    uint64_t reserved;             // MCFG 特有：8 字节保留字段
    struct mcfg_entry entries[];   // 可变长度数组，可有多个条目
} __attribute__((packed));

struct acpi_madt
{
    struct acpi_sdt_header header; // 36 字节的 ACPI 标准表头
    uint32_t local_apic_address;   // 局部 APIC (Local APIC) 的物理基地址 (用于 APIC 模式)
    uint32_t flags;                // 属性标志：Bit 0 置 1 表示系统支持双 8259 中断控制器兼容模式

    // 紧跟其后的是可变长度的 Local/IO APIC 结构体数组 (Interrupt Controller Structures)
    // 在 C 语言中可以用柔性数组（Flexible Array Member）来表示，方便指针偏移
    uint8_t entries[];
} __attribute__((packed));

struct acpi_madt_entry_header
{
    uint8_t type;   // 子表类型 (0 ~ 16+)
    uint8_t length; // 当前子表的总长度（含 header）
} __attribute__((packed));

struct acpi_madt_local_apic
{
    struct acpi_madt_entry_header header;
    uint8_t processor_id; // ACPI 处理器 ID
    uint8_t apic_id;      // 硬件 Local APIC ID
    uint32_t flags;       // Bit 0: Enabled, Bit 1: Online Capable
} __attribute__((packed));

struct acpi_madt_io_apic
{
    struct acpi_madt_entry_header header;
    uint8_t io_apic_id;                    // I/O APIC ID
    uint8_t reserved;                      // 保留
    uint32_t io_apic_addr;                 // 物理基地址
    uint32_t global_system_interrupt_base; // 该 GSI 的起始中断号
} __attribute__((packed));

struct acpi_madt_int_source_override
{
    struct acpi_madt_entry_header header;
    uint8_t bus;    // 总线源 (通常为 0，代表 ISA)
    uint8_t source; // 变动前的 IRQ 号
    uint32_t gsi;   // 变动后的 GSI 号
    uint16_t flags; // 触发和极性标志（如电平/边沿触发）
} __attribute__((packed));

struct acpi_madt_nmi_source
{
    struct acpi_madt_entry_header header;
    uint16_t flags; // 触发和极性标志
    uint32_t gsi;   // 对应的 GSI 号
} __attribute__((packed));

struct acpi_madt_local_apic_nmi
{
    struct acpi_madt_entry_header header;
    uint8_t processor_id; // 处理器 ID (0xFF 表示所有处理器)
    uint16_t flags;       // 触发和极性标志
    uint8_t lint;         // LINT 引脚号 (通常为 1)
} __attribute__((packed));

struct acpi_madt_local_apic_address_override
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;           // 保留
    uint64_t local_apic_address; // 64位 Local APIC 物理地址
} __attribute__((packed));

struct acpi_madt_io_sapic
{
    struct acpi_madt_entry_header header;
    uint8_t io_sapic_id;
    uint8_t reserved;
    uint32_t global_system_interrupt_base;
    uint64_t io_sapic_address;
} __attribute__((packed));

struct acpi_madt_local_sapic
{
    struct acpi_madt_entry_header header;
    uint8_t processor_id;
    uint8_t local_sapic_id;
    uint8_t local_sapic_eid;
    uint8_t reserved[3];
    uint32_t flags;
    uint32_t processor_uid;
    char processor_uid_string[]; // 柔性数组，若 uid 很大时使用
} __attribute__((packed));

struct acpi_madt_platform_int_source
{
    struct acpi_madt_entry_header header;
    uint16_t flags;
    uint8_t int_type; // 中断类型
    uint8_t processor_id;
    uint8_t processor_eid;
    uint8_t io_sapic_vector;
    uint32_t gsi;
    uint32_t platform_int_flags;
} __attribute__((packed));

struct acpi_madt_local_x2apic
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint32_t x2apic_id;     // 32位的 x2APIC ID
    uint32_t flags;         // 同 Type 0
    uint32_t processor_uid; // 对应 ACPI Processor Device 的 UID
} __attribute__((packed));

struct acpi_madt_local_x2apic_nmi
{
    struct acpi_madt_entry_header header;
    uint16_t flags;
    uint32_t processor_uid; // 0xFFFFFFFF 表示所有处理器
    uint8_t lint;           // LINT 引脚号
    uint8_t reserved[3];
} __attribute__((packed));

struct acpi_madt_gicc
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint32_t cpu_interface_number;
    uint32_t acpi_processor_uid;
    uint32_t flags;
    uint32_t parking_protocol_version;
    uint32_t performance_interrupt_gsiv;
    uint64_t parked_address;
    uint64_t physical_base_address;
    uint64_t gicv;
    uint64_t gich;
    uint32_t vgic_maintenance_interrupt;
    uint64_t gicr_base_address;
    uint64_t mpidr;
    uint8_t processor_power_efficiency_class;
    uint8_t reserved2;
    uint16_t spe_overflow_interrupt; // 较新规范增加
} __attribute__((packed));

struct acpi_madt_gicd
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint32_t gic_id;
    uint64_t physical_base_address;
    uint32_t system_vector_base;
    uint8_t gic_version; // GIC 版本 (如 2, 3, 4)
    uint8_t reserved2[3];
} __attribute__((packed));

struct acpi_madt_gic_msi_frame
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint32_t gic_msi_frame_id;
    uint64_t physical_base_address;
    uint32_t flags;
    uint16_t spi_count;
    uint16_t spi_base;
} __attribute__((packed));

struct acpi_madt_gicr
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint64_t discovery_range_base_address;
    uint32_t discovery_range_length;
} __attribute__((packed));

struct acpi_madt_gits
{
    struct acpi_madt_entry_header header;
    uint16_t reserved;
    uint32_t gic_its_id;
    uint64_t physical_base_address;
    uint32_t reserved2;
} __attribute__((packed));

struct acpi_madt_mp_wakeup
{
    struct acpi_madt_entry_header header;
    uint16_t mail_box_version;
    uint32_t reserved;
    uint64_t mail_box_address;
} __attribute__((packed));

uint64_t acpi_find_mcfg_pci_mmio_base(uint64_t offset);
uint64_t acpi_find_madt_lapic_base(void);
struct acpi_rsdp *acpi_find_rsdp(void);
void acpi_init(void);

#endif // ACPI_H
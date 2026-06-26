#include <x86.h>
#include <int.h>
#include <acpi.h>
#include <page.h>
#include <cpuid.h>
#include <types.h>
#include <printk.h>
#include <ioapic.h>

static volatile __u32 *ioapic_base; /* MMIO 映射后的虚拟地址 */

static inline __u32 __get_current_apic_id(void)
{
    unsigned int eax, ebx, ecx, edx;

    /* 尝试 Leaf 0xB (Extended Topology Enumeration) */
    if (__get_cpuid_max(0, NULL) >= 0xB)
    {
        __cpuid_count(0xB, 0, eax, ebx, ecx, edx);
        if (ebx != 0)   /* SMT/Core 层级有效时 EBX != 0 */
            return edx; /* EDX = x2APIC ID (32-bit) */
    }

    /* 回退: Leaf 1 EBX[31:24] = 8-bit Initial APIC ID */
    __cpuid(1, eax, ebx, ecx, edx);
    return (ebx >> 24) & 0xFF;
}

static inline void ioapic_write(__u8 reg, __u32 val)
{
    ioapic_base[0] = reg; /* IOREGSEL: 选择寄存器 */
    ioapic_base[4] = val; /* IOWIN:    写入数据 */
}

static inline __u32 ioapic_read(__u8 reg)
{
    ioapic_base[0] = reg;
    return ioapic_base[4];
}

/* 读取/写入 64 位重定向表条目 (由两个 32 位寄存器组成) */
static inline __u64 ioapic_rte_read(__u8 entry)
{
    __u32 lo = ioapic_read(IOAPIC_REG_REDTBL + entry * 2);
    __u32 hi = ioapic_read(IOAPIC_REG_REDTBL + entry * 2 + 1);
    return ((__u64)hi << 32) | lo;
}

static inline void ioapic_rte_write(__u8 entry, __u64 rte)
{
    /* 先写高位(含目标APIC ID)，再写低位(含向量号和标志) */
    ioapic_write(IOAPIC_REG_REDTBL + entry * 2 + 1, (__u32)(rte >> 32));
    ioapic_write(IOAPIC_REG_REDTBL + entry * 2, (__u32)(rte));
}

void ioapic_enable_irq(__u8 irq, __u8 vector,
                       __u8 dest_apic_id, __u64 flags)
{
    __u64 rte = (__u64)vector | RTE_DELIVERY_FIXED | RTE_DEST_PHYSICAL | flags | ((__u64)dest_apic_id << RTE_DEST_SHIFT);
    /* 注意: 未设置 RTE_MASKED → 中断已启用 */
    ioapic_rte_write(irq, rte);
}

void ioapic_disable_irq(__u8 irq)
{
    __u64 rte = ioapic_rte_read(irq);
    rte |= RTE_MASKED;
    ioapic_rte_write(irq, rte);
}

void ioapic_init(void)
{
    __u32 curr_lapic_id;

    curr_lapic_id = __get_current_apic_id();
    printk("CPUID Current APIC ID %d", curr_lapic_id);

    ioapic_base = __acpi_find_madt_ioapic_base();
    printk("IOAPIC base %#llx map to %#llx", ioapic_base, (__u8 *)ioapic_base + KERNEL_BASE);
    map_page((__u64)ioapic_base, (__u64)ioapic_base + KERNEL_BASE, MAP_KERN_MMIO);
    ioapic_base = (__u32 *)((__u8 *)ioapic_base + KERNEL_BASE);

    __u32 ver = ioapic_read(IOAPIC_REG_VER);
    __u8 max_entry = (ver >> 16) & 0xff;
    printk("IOAPIC ver %u, max entries %u", ver & 0xff, max_entry);

    /**
     * Mask All Entry
     */
    for (int i = 0; i < max_entry; i++)
    {
        ioapic_rte_write(i, RTE_MASKED);
    }

    /**
     * 映射 COM1 中断
     */
    __u32 com1_gsi = __acpi_find_gsi_for_irq(4);
    printk("COM1 real GSI = %u", com1_gsi);
    ioapic_enable_irq(com1_gsi, X86_IRQ_COM1, curr_lapic_id, RTE_EDGE_TRIGGERED | RTE_ACTIVE_HIGH);
}
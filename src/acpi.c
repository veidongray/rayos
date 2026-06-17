#include <acpi.h>
#include <page.h>
#include <stdint.h>
#include <printk.h>
#include <string.h>

/* ACPI MCFG 表指针（PCIe ECAM/MMIO 配置空间） */
static struct acpi_mcfg *mcfg;

/* ACPI MADT 表指针（APIC/LAPIC 信息） */
static struct acpi_madt *madt;

/*
 * 根据 offset 获取 MCFG 中第 offset 个 PCIe ECAM 基地址
 */
uint64_t acpi_find_mcfg_pci_mmio_base(uint64_t offset)
{
    uint64_t entries;

    /* 计算 MCFG 中有多少个 ECAM 条目 */
    entries = (mcfg->header.length - sizeof(struct acpi_sdt_header) - 8) / 16;

    /* 越界检查 */
    if (offset >= entries)
    {
        return UINT64_MAX;
    }

    /* 返回对应 PCIe ECAM base address */
    return mcfg->entries[offset].base_address;
}

/*
 * 获取 LAPIC（Local APIC）物理地址
 */
uint64_t acpi_find_madt_lapic_base(void)
{
    return madt->local_apic_address;
}

/*
 * 在 BIOS 预留区域搜索 RSDP（Root System Description Pointer）
 */
struct acpi_rsdp *acpi_find_rsdp(void)
{
    for (uintptr_t ptr = 0xe0000; ptr < 0xfffff; ptr += 8)
    {
        /* 匹配 "RSD PTR " 签名 */
        if (!strncmp("RSD PTR ", (char *)ptr, 8))
        {
            return (struct acpi_rsdp *)ptr;
        }
    }
    return NULL;
}

/*
 * ACPI 初始化流程：
 * 1. 查找 RSDP
 * 2. 根据 ACPI 版本选择 RSDT / XSDT
 * 3. 遍历系统描述表
 * 4. 找到 MADT / MCFG 表
 */
void acpi_init(void)
{
    struct acpi_rsdp *rsdp;
    struct acpi_rsdt *rsdt;
    struct acpi_xsdt *xsdt;

    mcfg = NULL;
    madt = NULL;

    /* 查找 RSDP */
    rsdp = acpi_find_rsdp();

    /* ACPI 2.0+ 使用 XSDT */
    if (rsdp->revision >= 2)
    {
        printk("rsdp->xsdt_address %#llx\n", rsdp->xsdt_address);

        xsdt = (struct acpi_xsdt *)((uint64_t)rsdp->xsdt_address + KERNEL_BASE);

        /* 映射 XSDT 到虚拟地址空间 */
        map_page((uint64_t)rsdp->xsdt_address, (uint64_t)xsdt, 0x1b);
    }
    else
    {
        /* 映射 RSDT */
        rsdt = (struct acpi_rsdt *)((uintptr_t)rsdp->rsdt_address + KERNEL_BASE);
        map_page((uint64_t)rsdp->rsdt_address, (uint64_t)rsdt, 0x1b);
        
        printk("map rsdt_address %#llx -> %#llx\n", rsdp->rsdt_address, rsdt);

        /* 计算 RSDT 中表项数量 */
        uint32_t nr_rsdt_entry = (rsdt->header.length - sizeof(struct acpi_sdt_header)) >> 2;

        /* 遍历所有 ACPI 表 */
        for (uint32_t count = 0; count < nr_rsdt_entry; count++)
        {
            struct acpi_sdt_header *header =
                (struct acpi_sdt_header *)((uintptr_t)rsdt->entries[count] + KERNEL_BASE);

            /* 映射每一个 ACPI 表 */
            map_page((uint64_t)rsdt->entries[count], (uint64_t)header, 0x1b);

            /* 查找 MADT（APIC表） */
            if (!strncmp(header->signature, "APIC", 4))
            {
                madt = (struct acpi_madt *)header;
                printk("ACPI Found MADT %#llx\n", header);
                printk("MADT LAPIC address %#llx\n", madt->local_apic_address);
            }

            /* 查找 MCFG（PCIe ECAM表） */
            if (!strncmp(header->signature, "MCFG", 4))
            {
                mcfg = (struct acpi_mcfg *)header;
                printk("ACPI Found MCFG %#llx\n", mcfg);
                printk("MCFG base address %#llx\n", mcfg->entries[0].base_address);
                printk("MCFG lenght %u\n", mcfg->header.length);
                printk("MCFG number of tables %u\n",
                       (mcfg->header.length - sizeof(struct acpi_sdt_header) - 8) / 16);
            }
        }
    }
}
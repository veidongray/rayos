#include <mm.h>
#include <x86.h>
#include <pci.h>
#include <page.h>
#include <lib/printf/printf.h>

#define USE_MMIO 1
#define PCI_MMIO_BASE 0xB0000000

void pci_read_config_pio(uint32_t bus, uint32_t slot, uint32_t func, struct pci_config *pci_config)
{
    int count;
    uint32_t *data;
    uint32_t address;

    data = (uint32_t *)pci_config;
    for (count = 0; count < 64; count += 4)
    {
        address = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | (count & 0xFC);
        outl(PCI_CONFIG_ADDRESS_PORT, address);
        *(data++) = inl(PCI_CONFIG_DATA_PORT);
    }
}

void pci_read_config_mmio(uint32_t bus, uint32_t slot, uint32_t func, struct pci_config *pci_config)
{
    int count;
    uint32_t *data;
    volatile uint32_t *mmio_addr;

    data = (uint32_t *)pci_config;

    // 遍历传统 PCI 寄存器的前 64 字节（如果需要 PCIe 的 4KB 空间，可以把 64 改为 1024）
    for (count = 0; count < 64; count += 4)
    {
        /*
         * MMIO (ECAM) 地址计算公式:
         * Base Address + (Bus << 20) + (Device << 15) + (Function << 12) + Register
         */
        uintptr_t offset = ((bus & 0xFF) << 20) |
                           ((slot & 0x1F) << 15) |
                           ((func & 0x07) << 12) |
                           (count & 0xFFF);

        // 这里的 PCI_MMIO_BASE 必须是已经建立好页表映射的内核虚拟地址
        mmio_addr = (volatile uint32_t *)(PCI_MMIO_BASE + offset);

        // 直接像读取内存一样读取寄存器
        *(data++) = *mmio_addr;
    }
}

void pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, struct pci_config *pci_config)
{
    if (USE_MMIO)
    {
        pci_read_config_mmio(bus, slot, func, pci_config);
    }
    else
    {
        pci_read_config_pio(bus, slot, func, pci_config);
    }
}

void pci_probe(void)
{
    struct pci_config pc;
    uint32_t bus, slot, func;

    for (bus = 0; bus < 256; bus++)
    {
        for (slot = 0; slot < 32; slot++)
        {
            for (func = 0; func < 8; func++)
            {
                pci_read_config(bus, slot, func, &pc);
                if ((pc.header.vendor_id != 0xffff) && (pc.header.vendor_id != 0x0000))
                {
                    printf("PCI %04llx:%04llx, HeaderType %02llx\n", pc.header.vendor_id, pc.header.device_id, pc.header.header_type);
                }
            }
        }
    }
}

void pci_init(void)
{
    map_page_range((uint64_t)PCI_MMIO_BASE, (uint64_t)PCI_MMIO_BASE, 0x1b, 65536);
    pci_probe();
}
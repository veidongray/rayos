#include <mm.h>
#include <x86.h>
#include <pci.h>
#include <page.h>
#include <acpi.h>
#include <ahci.h>
#include <printk.h>
#include <lib/string/string.h>

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

        mmio_addr = (volatile uint32_t *)(acpi_find_mcfg_pci_mmio_base(0) + offset);

        // 直接像读取内存一样读取寄存器
        *(data++) = *mmio_addr;
    }
}

void pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, struct pci_config *pci_config)
{
#define USE_MMIO
#ifdef USE_MMIO
    pci_read_config_mmio(bus, slot, func, pci_config);
#else
    pci_read_config_pio(bus, slot, func, pci_config);
#endif
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
                if (pc.header.prof_if == 0x01 && pc.header.sub_class_code == 0x06 && pc.header.base_class_code == 0x01)
                {
                    struct pci_config *ahci = (struct pci_config *)kmalloc(sizeof(struct pci_config));

                    memcpy(ahci, &pc, sizeof(struct pci_config));
                    printk("AHCI Device Found %x:%x\n", ahci->header.vendor_id, ahci->header.device_id);
                    printk("AHCI bar5 %#llx\n", ahci->type0.bar[5]);
                    printk("AHCI interrupt line %u\n", ahci->type0.interrupt_line);
                    printk("AHCI interrupt pin %u\n", ahci->type0.interrupt_pin);
                    ahci_init((uintptr_t)ahci->type0.bar[5]);
                }
                if ((pc.header.vendor_id != 0xffff) && (pc.header.vendor_id != 0x0000))
                {
                    printk("PCI %04llx:%04llx, HeaderType %02llx\n", pc.header.vendor_id, pc.header.device_id, pc.header.header_type);
                }
            }
        }
    }
}

void pci_init(void)
{
    map_page_range((uint64_t)acpi_find_mcfg_pci_mmio_base(0), (uint64_t)acpi_find_mcfg_pci_mmio_base(0), 0x1b, 65536);
    pci_probe();
}
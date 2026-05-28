#include <mm.h>
#include <x86.h>
#include <pci.h>
#include <lib/printf/printf.h>

void pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, struct pci_config *pci_config)
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
                if (pc.header.vendor_id != 0xffff)
                {
                    printf("PCI %04llx:%04llx, HeaderType %02llx\n", pc.header.vendor_id, pc.header.device_id, pc.header.header_type);
                }
            }
        }
    }
}

void pci_init(void)
{
    pci_probe();
}
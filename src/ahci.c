#include <ahci.h>
#include <printk.h>

void ahci_init(uintptr_t ahci_base)
{
    uintptr_t *ahci_virt_base = (uintptr_t *)ahci_base;
    map_page_range((uint64_t)ahci_base, (uint64_t)ahci_virt_base, 0x1b, 2);

    uint32_t *cap = ahci_virt_base;
    uint32_t *ghc = ahci_virt_base + 1;
    uint32_t *pi = ahci_virt_base + 3;

    // AHCI Enable
    *ghc |= (1U << 31);
    // HBA Reset
    *ghc |= (1U << 0);
    // AHCI Enable Again
    *ghc |= (1U << 31);

    // Read HBA capabilities
    uint32_t nr_ports = ((*cap) & 0x1f) + 1;
    uint32_t port_map = (*pi);
    printk("AHCI Ports %u\n", nr_ports);
    printk("AHCI Port Map %#llx\n", port_map);

    for (int i = 0; i < nr_ports; i++)
    {
        volatile uint32_t *port = ahci_virt_base[0x100 + (i * 0x80)];
        printk("Port %d, PxSIG %#llx\n", i, port[9]);
    }
}
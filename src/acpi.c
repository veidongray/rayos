#include <acpi.h>
#include <page.h>
#include <stdint.h>
#include <printk.h>
#include <lib/string/string.h>

static struct acpi_mcfg *mcfg;
static struct acpi_madt *madt;

uint64_t acpi_find_mcfg_pci_mmio_base(uint64_t offset)
{
    uint64_t entries;

    entries = (mcfg->header.length - sizeof(struct acpi_sdt_header) - 8) / 16;
    if (offset >= entries)
    {
        return UINT64_MAX;
    }

    return mcfg->entries[offset].base_address;
}

uint64_t acpi_find_madt_lapic_base(void)
{
    return madt->local_apic_address;
}

struct acpi_rsdp *acpi_find_rsdp(void)
{
    for (uintptr_t ptr = 0xe0000; ptr < 0xfffff; ptr += 8)
    {
        if (!strncmp("RSD PTR ", (char *)ptr, 8))
        {
            return (struct acpi_rsdp *)ptr;
        }
    }
    return NULL;
}

void acpi_init(void)
{
    struct acpi_rsdp *rsdp;
    struct acpi_rsdt *rsdt;
    struct acpi_xsdt *xsdt;

    mcfg = NULL;
    madt = NULL;
    rsdp = acpi_find_rsdp();
    if (rsdp->revision >= 2)
    {
        printk("rsdp->xsdt_address %#llx\n", rsdp->xsdt_address);
        xsdt = (struct acpi_xsdt *)rsdp->xsdt_address;
        map_page((uint64_t)rsdp->xsdt_address, (uint64_t)xsdt, 0x1b);
    }
    else
    {
        printk("rsdp->rsdt_address %#llx\n", rsdp->rsdt_address);
        rsdt = (struct acpi_rsdt *)((uintptr_t)rsdp->rsdt_address);
        map_page((uint64_t)rsdp->rsdt_address, (uint64_t)rsdt, 0x1b);
        uint32_t nr_rsdt_entry = (rsdt->header.length - sizeof(struct acpi_sdt_header)) >> 2;
        for (uint32_t count = 0; count < nr_rsdt_entry; count++)
        {
            struct acpi_sdt_header *header = (struct acpi_sdt_header *)((uintptr_t)rsdt->entries[count]);
            map_page((uint64_t)rsdt->entries[count], (uint64_t)header, 0x1b);
            if (!strncmp(header->signature, "APIC", 4))
            {
                madt = (struct acpi_madt *)header;
                printk("ACPI Found MADT %#llx\n", header);
                printk("MADT LAPIC address %#llx\n", madt->local_apic_address);
            }
            if (!strncmp(header->signature, "MCFG", 4))
            {
                mcfg = (struct acpi_mcfg *)header;
                printk("ACPI Found MCFG %#llx\n", mcfg);
                printk("MCFG base address %#llx\n", mcfg->entries[0].base_address);
                printk("MCFG lenght %u\n", mcfg->header.length);
                printk("MCFG number of tables %u\n", (mcfg->header.length - sizeof(struct acpi_sdt_header) - 8) / 16);
            }
        }
    }
}
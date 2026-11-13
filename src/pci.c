#include <acpi.h>
#include <list.h>
#include <mm.h>
#include <page.h>
#include <pci.h>
#include <printk.h>
#include <sata.h>
#include <string.h>
#include <x86.h>

static LIST_HEAD(pci_device_list);
static LIST_HEAD(pci_driver_list);

void pci_read_config_pio(uint32_t bus, uint32_t slot, uint32_t func,
                         struct pci_config *pci_config)
{
	int count;
	uint32_t *data;
	uint32_t address;

	data = (uint32_t *)pci_config;
	for (count = 0; count < 64; count += 4) {
		address = (1U << 31) | (bus << 16) | (slot << 11) |
		          (func << 8) | (count & 0xFC);
		outl(PCI_CONFIG_ADDRESS_PORT, address);
		*(data++) = inl(PCI_CONFIG_DATA_PORT);
	}
}

void pci_read_config_mmio(uint32_t bus, uint32_t slot, uint32_t func,
                          struct pci_config *pci_config)
{
	int count;
	uint32_t *data;
	volatile uint32_t *mmio_addr;

	data = (uint32_t *)pci_config;

	// 遍历传统 PCI 寄存器的前 64 字节（如果需要 PCIe 的 4KB 空间，可以把 64
	// 改为 1024）
	for (count = 0; count < 64; count += 4) {
		/*
		 * MMIO (ECAM) 地址计算公式:
		 * Base Address + (Bus << 20) + (Device << 15) + (Function <<
		 * 12) + Register
		 */
		uintptr_t offset = ((bus & 0xFF) << 20) |
		                   ((slot & 0x1F) << 15) |
		                   ((func & 0x07) << 12) | (count & 0xFFF);

		mmio_addr =
		        (volatile uint32_t *)(acpi_find_mcfg_pci_mmio_base(0) +
		                              offset);

		// 直接像读取内存一样读取寄存器
		*(data++) = *mmio_addr;
	}
}

void pci_read_config(uint32_t bus, uint32_t slot, uint32_t func,
                     struct pci_config *pci_config)
{
#define USE_MMIO
#ifdef USE_MMIO
	pci_read_config_mmio(bus, slot, func, pci_config);
#else
	pci_read_config_pio(bus, slot, func, pci_config);
#endif
}

static int pci_device_match(struct pci_driver *driver)
{
	struct pci_device *dev;
	list_for_each_entry(dev, &pci_device_list, list)
	{
		if (dev->config->header.vendor_id ==
		            driver->id_table->vendor_id &&
		    dev->config->header.device_id ==
		            driver->id_table->device_id &&
		    dev->config->header.base_class_code ==
		            driver->id_table->base_class_code &&
		    dev->config->header.sub_class_code ==
		            driver->id_table->sub_class_code &&
		    dev->config->header.prof_if == driver->id_table->prof_if) {
			dev->driver = driver;
			driver->device = dev;
			if (driver->probe) {
				driver->probe(dev);
				return 0;
			}
		}
	}
	return 1;
}

int pci_register_driver(struct pci_driver *driver)
{
	list_add_tail(&driver->list, &pci_driver_list);
	/* 尝试匹配device */
	return pci_device_match(driver);
}

void pci_unregister_driver(struct pci_driver *driver)
{
	list_del(&driver->list);
}

int pci_add_device(struct pci_device *dev)
{
	list_add_tail(&dev->list, &pci_device_list);
	return 0;
}

void pci_remove_device(struct pci_device *dev) { list_del(&dev->list); }

void pci_device_detect(void)
{
	struct pci_config pc;
	uint32_t bus, slot, func;

	for (bus = 0; bus < 256; bus++) {
		for (slot = 0; slot < 32; slot++) {
			for (func = 0; func < 8; func++) {
				pci_read_config(bus, slot, func, &pc);
				if ((pc.header.vendor_id != 0xffff) &&
				    (pc.header.vendor_id != 0x0000)) {
					/* 探测到可用pci设备则创建pci_device并加入pci_device_list
					 */
					struct pci_device *dev =
					        (struct pci_device *)kmalloc(
					                sizeof(struct
					                       pci_device));
					dev->bus = bus;
					dev->slot = slot;
					dev->func = func;
					dev->config = (struct pci_config *)
					        kmalloc(sizeof(
					                struct pci_config));
					memcpy(dev->config, &pc,
					       sizeof(struct pci_config));
					pci_add_device(dev);
				}
			}
		}
	}
}

void pci_init(void)
{
	map_page_range((uint64_t)acpi_find_mcfg_pci_mmio_base(0),
	               (uint64_t)acpi_find_mcfg_pci_mmio_base(0) + KERNEL_BASE,
	               0x1b, 65536);
	pci_device_detect();
}
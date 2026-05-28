#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// x86 传统的 PCI 配置空间 I/O 端口
#define PCI_CONFIG_ADDRESS_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

struct pci_config_header
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prof_if;
    uint8_t sub_class_code;
    uint8_t base_class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
};

struct pci_config_type0
{
    uint32_t bar[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base;
    uint32_t capabilities_ptr;
    uint32_t reserved;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
};

struct pci_config_type1
{
    uint32_t bar[2];
    uint8_t primary_bus;
    uint8_t secondary_bus;
    uint8_t subordinate_bus;
    uint8_t secondary_latency;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetch_base;
    uint16_t prefetch_limit;
    uint32_t prefetch_base_upper;
    uint32_t prefetch_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint32_t capabilities_ptr;
    uint32_t expansion_rom_base;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
};

struct pci_config
{
    struct pci_config_header header;
    union {
        struct pci_config_type0 type0;
        struct pci_config_type1 type1;
    };
};

// 暴露给内核初始化的主入口函数
void pci_init(void);

#endif // PCI_H
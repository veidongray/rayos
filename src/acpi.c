#include <acpi.h>
#include <stdint.h>
#include <lib/printf/printf.h>
#include <lib/string/string.h>

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

struct acpi_rsdp *acpi_find_rsdp(void)
{
    for (uintptr_t i = 0xe0000; i < 0xfffff; i += 8)
    {
        if (!strncmp("RSD PTR ", (char *)i, 8))
        {
            return (struct acpi_rsdp *)i;
        }
    }
    return NULL;
}

void acpi_init(void)
{
    struct acpi_rsdp *rsdp;

    rsdp = acpi_find_rsdp();
}
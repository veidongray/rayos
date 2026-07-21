#include <acpi.h>
#include <cpuid.h>
#include <int.h>
#include <lapic.h>
#include <page.h>
#include <printk.h>
#include <smp.h>
#include <string.h>
#include <types.h>

#define AP_MAX 64
#define AP_BASE 0x8000ULL
#define TRAMPOLINE_VECTOR (AP_BASE >> 12) // = 0x08

static __u32 bsp_id = 0;
static __u32 ap_count = 0;
static uint32_t ap_ids[AP_MAX];

// 从 trampoline.S 中引入 trampoline
extern char trampoline[];
extern char trampoline_end[];

void ap_startup(uint8_t apic_id)
{
	// 发送 INIT
	lapic_send_init(apic_id);
	mdelay(10); // Intel 手册要求等 10ms

	// 发送第一次 SIPI
	lapic_send_sipi(apic_id, TRAMPOLINE_VECTOR);
	mdelay(1); // 等 200μs

	// 发送第二次 SIPI（冗余，确保收到）
	lapic_send_sipi(apic_id, TRAMPOLINE_VECTOR);
	mdelay(1);

	// AP 现在应该已经在 0x8000 执行了
}

int smp_init(void)
{
	int ret;
	bsp_id = __get_current_apic_id();
	ap_count = acpi_madt_smp_counter(ap_ids);
	pr_info("ap count %u", ap_count);

	pr_info("Map AP BASE 0x8000");
	ret = map_page_range(AP_BASE, AP_BASE, MAP_KERN_RW, 1);
	switch (ret) {
	case -MAP_ERR_EXIST:
		break;
	default:
		pr_info("Map AP BASE 0x8000 Error code %d", ret);
		while (1)
			;
		break;
	}

	char *ptr = trampoline;
	char *end = trampoline_end;
	// 复制 trampoline 到 0x8000
	memcpy((void *)AP_BASE, ptr, end - ptr);

	local_irq_enable();
	// 发送 SIPI
	for (__u32 cont = 0; cont < ap_count; cont++) {
		if (ap_ids[cont] == bsp_id) {
			continue;
		}
		ap_startup(ap_ids[cont]);
	}
	local_irq_disable();
	return 0;
}

__u32 __get_current_apic_id(void)
{
	unsigned int eax, ebx, ecx, edx;

	/* 尝试 Leaf 0xB (Extended Topology Enumeration) */
	if (__get_cpuid_max(0, NULL) >= 0xB) {
		__cpuid_count(0xB, 0, eax, ebx, ecx, edx);
		if (ebx != 0)       /* SMT/Core 层级有效时 EBX != 0 */
			return edx; /* EDX = x2APIC ID (32-bit) */
	}

	/* 回退: Leaf 1 EBX[31:24] = 8-bit Initial APIC ID */
	__cpuid(1, eax, ebx, ecx, edx);
	return (ebx >> 24) & 0xFF;
}
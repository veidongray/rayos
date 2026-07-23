#include <acpi.h>
#include <int.h>
#include <lapic.h>
#include <page.h>
#include <pic.h>

/*
┌─────────────────────────────────────────────────────────────────┐
│                        Local APIC 内部                          │
│                                                                 │
│   ┌──────────┐                                                  │
│   │ LVT Timer│──→ vector 0x20 ──┐                              │
│   └──────────┘                   │                              │
│   ┌──────────┐                   │                              │
│   │ LVT LINT0│──→ (masked) ✗    │    ┌──────────────┐          │
│   └──────────┘                   ├───→│  中断仲裁器   │──→ CPU   │
│   ┌──────────┐                   │    │  (IRR/ISR)   │          │
│   │ LVT LINT1│──→ (masked) ✗    │    └──────────────┘          │
│   └──────────┘                   │         ▲                    │
│   ┌──────────┐                   │         │                    │
│   │ LVT Error│──→ vector 0x22 ──┘         │                    │
│   └──────────┘                             │                    │
│                                            │                    │
│   ═══════════════ APIC System Bus ═════════╪════════            │
│                                            │                    │
└────────────────────────────────────────────┼────────────────────┘
                                             │
                                    ┌────────┴────────┐
                                    │     IOAPIC      │
                                    │  (Redirection   │
                                    │   Table Entry)  │
                                    └────────┬────────┘
                                             │
                                        COM1 / 键盘 / 磁盘 ...

Note:
        LINT0/LINT1 是 Local APIC 的两个物理引脚，只跟 8259A PIC 和 NMI 有关。
        IOAPIC 通过 APIC 系统总线投递中断，直接写入 IRR，完全不经过 LVT，不受
LINT mask 影响。
*/
static uint32_t lapic_base;
static uint32_t lapic_base_virt;
static uint32_t ticks_per_1ms = 0;
static uint32_t ticks_per_10ms = 0;

// 读写内存映射寄存器的辅助函数
static inline uint32_t __lapic_read(uint32_t reg)
{
	// 先将 64 位基地址与 32 位偏移量相加，得到完整的 64
	// 位虚拟地址，再强转为指针
	return *(volatile uint32_t *)((uintptr_t)(KERNEL_BASE + lapic_base +
	                                          reg));
}

static inline void __lapic_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t *)((uintptr_t)(KERNEL_BASE + lapic_base + reg)) =
	        data;
}

/*
ICR_HIGH (0x310):
┌────────────────────────────────────────────────────────┐
│ 31        24 │ 23                                   0 │
│  Dest Field  │            Reserved (0)                │
│  (APIC ID)   │                                        │
└────────────────────────────────────────────────────────┘

ICR_LOW (0x300):
┌────────────────────────────────────────────────────────┐
│ 31 20│19 18│17│16│15│14│13 12│11  8│7      0│
│ Resv │Dest │  │  │  │  │Deliv│Deliv│ Vector │
│      │Shrt │  │  │  │  │Mode │Status│        │
│      │     │  │  │  │  │     │      │        │
│      │00=  │  │  │  │  │0=物理│0=空闲│ 中断号 │
│      │  无  │  │  │  │  │1=逻辑│1=发送中│ /SIPI │
│      │01=  │  │  │  │  │     │      │ 页号   │
│      │  自己│  │  │  │  │     │      │        │
│      │10=  │  │  │  │  │     │      │        │
│      │  所有│  │  │  │  │     │      │        │
│      │11=  │  │  │  │  │     │      │        │
│      │ 除自己│ │  │  │  │     │      │        │
└────────────────────────────────────────────────────────┘

各字段：
  [7:0]   Vector          — 中断向量 / SIPI 页号
  [10:8]  Delivery Mode   — 发送类型
  [11]    Dest Mode       — 0=物理ID, 1=逻辑ID
  [12]    Delivery Status — 只读，0=空闲, 1=正在发送
  [14]    Level           — 0=De-assert, 1=Assert
  [15]    Trigger Mode    — 0=Edge, 1=Level
  [19:18] Dest Shorthand  — 目标简写
*/
// 等待 IPI 发送完成
void lapic_wait_ipi(void)
{
	volatile uint32_t *lapic_icr_lo =
	        (volatile uint32_t *)(uintptr_t)(lapic_base_virt + 0x300);
	while (*lapic_icr_lo & (1 << 12)) // Delivery Status bit
		asm volatile("pause");
}

// 发送 INIT IPI
void lapic_send_init(uint8_t apic_id)
{
	volatile uint32_t *lapic_icr_lo =
	        (volatile uint32_t *)(uintptr_t)(lapic_base_virt + 0x300);
	volatile uint32_t *lapic_icr_hi =
	        (volatile uint32_t *)(uintptr_t)(lapic_base_virt + 0x310);
	*lapic_icr_hi = (uint32_t)apic_id << 24;
	*lapic_icr_lo = ICR_INIT | ICR_LEVEL_ASSERT | ICR_PHYSICAL;
	lapic_wait_ipi();
}

// 发送 SIPI
void lapic_send_sipi(uint8_t apic_id, uint8_t vector)
{
	volatile uint32_t *lapic_icr_lo =
	        (volatile uint32_t *)(uintptr_t)(lapic_base_virt + 0x300);
	volatile uint32_t *lapic_icr_hi =
	        (volatile uint32_t *)(uintptr_t)(lapic_base_virt + 0x310);
	*lapic_icr_hi = (uint32_t)apic_id << 24;
	*lapic_icr_lo = ICR_SIPI | vector;
	lapic_wait_ipi();
}

void lapic_init(void)
{
	lapic_base = acpi_find_madt_lapic_base();
	lapic_base_virt = KERNEL_BASE + lapic_base;
	map_page(lapic_base, lapic_base_virt, MAP_KERN_MMIO);

	__lapic_write(LAPIC_SVR, 0x1FF);
	lapic_calibrate();
	// mask PIC
	pic_disable();

	// setup APIC timer
	__lapic_write(LAPIC_TICFG, 0x3);
	__lapic_write(LAPIC_LVT_TMR, 0x20000 | X86_APIC_TIMER_VECTOR);
	__lapic_write(LAPIC_TIC, ticks_per_1ms);

	__lapic_write(LAPIC_LVT_LINT0, 0x10000); // Masked
	__lapic_write(LAPIC_LVT_LINT1, 0x10000); // Masked

	__lapic_write(LAPIC_LVT_ERR, X86_APIC_ERROR_VECTOR); // 错误中断向量号

	__lapic_write(LAPIC_ESR, 0);
	__lapic_write(LAPIC_ESR, 0); // 连续写两次是Intel手册建议的

	__lapic_write(LAPIC_EOI, 0);
	__lapic_write(LAPIC_TPR, 0);
}

void ap_lapic_init(void)
{
	__lapic_write(LAPIC_SVR, 0x1FF);

	// setup APIC timer
	__lapic_write(LAPIC_TICFG, 0x3);
	__lapic_write(LAPIC_LVT_TMR, 0x20000 | X86_APIC_TIMER_VECTOR);
	__lapic_write(LAPIC_TIC, ticks_per_1ms);

	__lapic_write(LAPIC_LVT_LINT0, 0x10000); // Masked
	__lapic_write(LAPIC_LVT_LINT1, 0x10000); // Masked

	__lapic_write(LAPIC_LVT_ERR, X86_APIC_ERROR_VECTOR); // 错误中断向量号

	__lapic_write(LAPIC_ESR, 0);
	__lapic_write(LAPIC_ESR, 0); // 连续写两次是Intel手册建议的

	__lapic_write(LAPIC_EOI, 0);
	__lapic_write(LAPIC_TPR, 0);
}

void lapic_calibrate(void)
{
	uint32_t current_tick;

	// 确保 APIC 定时器分频已设置 (比如 16 分频)
	__lapic_write(LAPIC_TDCR, 0x3);
	// 准备 PIT
	pit_prepare_sleep_10ms();
	// 设置 LAPIC 初始值为最大 (0xFFFFFFFF)
	__lapic_write(LAPIC_TIC, 0xFFFFFFFF);
	// 开始等待 10ms
	pit_wait_10ms();
	// 10ms 到了，读取 LAPIC 当前剩下的数值
	current_tick = __lapic_read(LAPIC_TCC);
	// 停止 LAPIC 定时器
	__lapic_write(LAPIC_LVT_TMR, 0x10000); // Masked
	// 计算差值
	ticks_per_10ms = UINT32_MAX - current_tick;
	ticks_per_1ms = ticks_per_10ms / 10;
}

void lapic_send_eoi(void) { __lapic_write(LAPIC_EOI, 0); }
#include <pic.h>
#include <acpi.h>
#include <page.h>
#include <lapic.h>

static uint32_t ticks_per_10ms = 0;

void lapic_init(void)
{
    // map 0xfee00000
    map_page(LAPIC_BASE, KERNEL_BASE + acpi_find_madt_lapic_base(), 0x1b);

    lapic_calibrate();
    // mask PIC
    pic_disable();

    lapic_write(LAPIC_SVR, 0x1FF);

    // setup APIC timer
    lapic_write(LAPIC_TICFG, 0x3);
    lapic_write(LAPIC_LVT_TMR, 0x20000 | 32);
    lapic_write(LAPIC_TIC, ticks_per_10ms);

    lapic_write(LAPIC_LVT_LINT0, 0x10000); // Masked
    lapic_write(LAPIC_LVT_LINT1, 0x10000); // Masked

    lapic_write(LAPIC_LVT_ERR, 33); // 错误中断向量号 33

    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0); // 连续写两次是Intel手册建议的

    lapic_write(LAPIC_EOI, 0);
    lapic_write(LAPIC_TPR, 0);
}

void lapic_calibrate(void)
{
    uint32_t current_tick;

    // 确保 APIC 定时器分频已设置 (比如 16 分频)
    lapic_write(LAPIC_TDCR, 0x3);
    // 准备 PIT
    pit_prepare_sleep_10ms();
    // 设置 LAPIC 初始值为最大 (0xFFFFFFFF)
    lapic_write(LAPIC_TIC, 0xFFFFFFFF);
    // 开始等待 10ms
    pit_wait_10ms();
    // 10ms 到了，读取 LAPIC 当前剩下的数值
    current_tick = lapic_read(LAPIC_TCC);
    // 停止 LAPIC 定时器
    lapic_write(LAPIC_LVT_TMR, 0x10000); // Masked
    // 计算差值
    ticks_per_10ms = UINT32_MAX - current_tick;
}

void lapic_send_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}
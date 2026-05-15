#include "io.h"
#include "pic_8259.h"

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20 /* End-of-interrupt command code */

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

#define CASCADE_IRQ 2

void timer_init(void)
{
    // configure channel 0 to mode 2
    outb(PIT_COMMAND_PORT, PIT_CMD_CH0_MODE2);

    // write DIVISOR LSB first.
    outb(PIT_CHANNEL0_PORT, (uint8_t)(PIT_DIVISOR & 0xFF));        // LSB
    outb(PIT_CHANNEL0_PORT, (uint8_t)((PIT_DIVISOR >> 8) & 0xFF)); // MSB
}

/*
arguments:
    offset1 - vector offset for master PIC
        vectors on the master become offset1..offset1+7
    offset2 - same for slave PIC: offset2..offset2+7
*/
void pic_remap(int offset1, int offset2)
{
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
    io_wait();
    outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
    io_wait();
    outb(PIC1_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC that there is a slave PIC at IRQ2
    io_wait();
    outb(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outb(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Unmask both PICs.
    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);
}

void pic_disable(void)
{
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void pic_set_mask(uint8_t IRQline)
{
    uint16_t port;
    uint8_t value;

    if (IRQline < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);
}

void pic_clear_mask(uint8_t IRQline)
{
    uint16_t port;
    uint8_t value;

    if (IRQline < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

void pic_sendEOI(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

// 让 PIT 等待大约 10 毫秒
void pit_prepare_sleep_10ms(void)
{
    uint16_t count = 11932; // 1.193182 MHz / 100 = 11932 次计数约为 10ms

    // 设置 PIT 通道 0, 模式 0 (Interrupt on Terminal Count), 16位计数
    outb(0x43, 0x30);
    outb(0x40, (uint8_t)(count & 0xFF));        // 低8位
    outb(0x40, (uint8_t)((count >> 8) & 0xFF)); // 高8位
}

void pit_wait_10ms(void)
{
    // 轮询直到 PIT 计数归零
    // 在模式 0 下，我们可以通过读取回读命令或简单等待
    // 为了简单，这里演示读取当前计数值直到它翻转或到达 0
    // 注意：实际内核中建议使用更精确的读取方式
    while (1)
    {
        outb(0x43, 0x00); // 锁存计数值
        uint8_t low = inb(0x40);
        uint8_t high = inb(0x40);
        if (high == 0 && low <= 1)
            break;
        if (high > 0x30)
            break; // 防止已经翻转
    }
}
#include <x86.h>
#include <uart.h>

// UART 基地址
static uint16_t uart_base = UART_COM1_BASE;

void uart_init(void)
{
    // 1. 禁用所有中断
    outb(uart_base + UART_IER, 0x00);

    // 2. 启用 DLAB (Divisor Latch Access Bit)，以便设置波特率
    outb(uart_base + UART_LCR, UART_LCR_DLAB);

    // 3. 设置波特率除数
    // 标准时钟频率为 1.8432 MHz
    // 波特率 = 时钟频率 / (16 * 除数)
    // 对于 115200 波特率: 除数 = 1843200 / (16 * 115200) = 1
    // DLL = 除数 & 0xFF, DLM = (除数 >> 8) & 0xFF
    outb(uart_base + UART_DLL, 0x01); // 低字节
    outb(uart_base + UART_DLM, 0x00); // 高字节

    // 4. 设置数据格式: 8 位, 无奇偶校验, 1 停止位, 并关闭 DLAB
    outb(uart_base + UART_LCR, UART_LCR_8BIT);

    // 5. 启用 FIFO，并清空接收/发送缓冲区
    // 注意: 在非常早期的启动代码中，有时会跳过此步以简化
    outb(uart_base + UART_FCR, 0xC7);

    // 6. 启用 RTS 和 DTR 信号 (通常需要)
    outb(uart_base + UART_MCR, 0x0B);
}

static inline void uart_wait_for_transmit_empty(void)
{
    while ((inb(uart_base + UART_LSR) & UART_LSR_EMPTY) == 0)
    {
        // 忙等待
    }
}

void uart_putc(char c)
{
    // 特殊处理换行符 '\n' -> '\r\n'
    if (c == '\n')
    {
        uart_putc('\r');
    }

    // 等待发送缓冲区为空
    uart_wait_for_transmit_empty();
    // 写入发送保持寄存器
    outb(uart_base + UART_THR, (uint8_t)c);
}

void uart_puts(const char *str)
{
    while (*str)
    {
        uart_putc(*str++);
    }
}
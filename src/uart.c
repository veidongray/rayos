#include <mm.h>
#include <x86.h>
#include <uart.h>
#include <types.h>
#include <stddef.h>
#include <printk.h>

// UART 基地址
static uint16_t uart_base = UART_COM1_BASE;

static struct uart_ringbuffer uart_rx_buf;

void uart_init(void)
{
    uart_rx_buf.data = (__u8 *)kzalloc(UART_BUF_SIZE);
    uart_rx_buf.head = 0;
    uart_rx_buf.tail = 0;

    // 禁用所有中断
    outb(uart_base + UART_IER, 0x00);

    // 启用 DLAB (Divisor Latch Access Bit)，以便设置波特率
    outb(uart_base + UART_LCR, UART_LCR_DLAB);

    // 设置波特率除数
    // 标准时钟频率为 1.8432 MHz
    // 波特率 = 时钟频率 / (16 * 除数)
    // 对于 115200 波特率: 除数 = 1843200 / (16 * 115200) = 1
    // DLL = 除数 & 0xFF, DLM = (除数 >> 8) & 0xFF
    outb(uart_base + UART_DLL, 0x01); // 低字节
    outb(uart_base + UART_DLM, 0x00); // 高字节

    // 设置数据格式: 8 位, 无奇偶校验, 1 停止位, 并关闭 DLAB
    outb(uart_base + UART_LCR, UART_LCR_8BIT);

    // 启用 FIFO，并清空接收/发送缓冲区
    // 注意: 在非常早期的启动代码中，有时会跳过此步以简化
    outb(uart_base + UART_FCR, 0xC7);

    // 启用 RTS 和 DTR 信号 (通常需要)
    outb(uart_base + UART_MCR, 0x0B);

    // 打开 COM1 中断
    outb(uart_base + UART_IER, 0x01);
}

void uart_enable_irq(void)
{
    outb(uart_base + UART_IER, 0x01);
}

void uart_disable_irq(void)
{
    outb(uart_base + UART_IER, 0x00);
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
        uart_wait_for_transmit_empty();
        outb(uart_base + UART_THR, (uint8_t)'\r');
    }
    else if (c == '\r')
    {
        uart_wait_for_transmit_empty();
        outb(uart_base + UART_THR, (uint8_t)'\n');
    }

    uart_wait_for_transmit_empty();
    outb(uart_base + UART_THR, (uint8_t)c);
}

void uart_puts(const char *str)
{
    while (*str)
    {
        uart_putc(*str++);
    }
}

void uart_isr_receive(void)
{
    // 必须读走所有数据以清除硬件中断条件，防止中断风暴
    while (inb(UART_COM1_BASE + UART_LSR) & UART_LSR_READY)
    {
        uint8_t ch = inb(UART_COM1_BASE + UART_RHR);

        size_t next_head = (uart_rx_buf.head + 1) % UART_BUF_SIZE;
        // 缓冲区未满时才写入，满了则丢弃新数据
        if (next_head != uart_rx_buf.tail)
        {
            uart_rx_buf.data[uart_rx_buf.head] = ch;
            uart_rx_buf.head = next_head;
        }
    }
}

char uart_getc(void)
{
    // 缓冲区为空时，挂起 CPU 等待串口中断唤醒
    while (uart_rx_buf.head == uart_rx_buf.tail)
    {
        // Do nothing.
    }

    // 被中断唤醒且缓冲区有数据，取出字符
    char ch = uart_rx_buf.data[uart_rx_buf.tail];
    uart_rx_buf.tail = (uart_rx_buf.tail + 1) % UART_BUF_SIZE;
    return ch;
}

char *uart_gets(char *s)
{
    int count;
    __u8 ch;

    count = 0;
    while (count < UART_BUF_SIZE)
    {
        ch = (__u8)uart_getc();
        if ((ch >= 32) && (ch <= 127))
        {
            uart_putc(ch);
            s[count++] = ch;
        }
        else if ((ch == '\r') || (ch == '\n') || (ch == '\0'))
        {
            uart_putc(ch);
            s[count] = ch;
            return s;
        }
        else if (ch == 0x8)
        {
            // 处理退格
            if (count)
            {
                uart_putc(ch);
                count--;
            }
        }
    }
    return NULL;
}
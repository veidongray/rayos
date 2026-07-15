#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define UART_BUF_SIZE 2048

// COM1 的基地址
#define UART_COM1_BASE 0x3F8

// UART 寄存器相对于基地址的偏移
#define UART_RHR 0 // 接收保持寄存器 (Receive Holding Register) - 只读
#define UART_THR 0 // 发送保持寄存器 (Transmit Holding Register) - 只写
#define UART_DLL 0 // 除数锁存器低字节 (Divisor Latch LSB) - DLAB=1 时
#define UART_IER 1 // 中断使能寄存器 (Interrupt Enable Register)
#define UART_DLM 1 // 除数锁存器高字节 (Divisor Latch MSB) - DLAB=1 时
#define UART_FCR 2 // FIFO 控制寄存器 (FIFO Control Register) - 只写
#define UART_LCR 3 // 线路控制寄存器 (Line Control Register)
#define UART_MCR 4 // 调制解调器控制寄存器 (Modem Control Register)
#define UART_LSR 5 // 线路状态寄存器 (Line Status Register)
#define UART_MSR 6 // 调制解调器状态寄存器 (Modem Status Register)
#define UART_SCR 7 // 暂存寄存器 (Scratch Register)

// LCR 寄存器中的位定义
#define UART_LCR_DLAB (1 << 7) // 除数锁存器访问位
#define UART_LCR_8BIT 0x03     // 8 位数据，无奇偶校验

// LSR 寄存器中的位定义
#define UART_LSR_EMPTY (1 << 5) // 发送保持寄存器为空
#define UART_LSR_READY (1 << 0) // 接收数据就绪

struct uart_ringbuffer {
	__u8 *data;
	volatile size_t head; // ISR 写入位置
	volatile size_t tail; // 上层读取位置
};

// 初始化并启用 UART
void uart_init(void);

// 阻塞式发送一个字符
void uart_putc(char c);

// 发送一个字符串
void uart_puts(const char *str);

// 阻塞式接收一个字符
char uart_getc(void);

// 接收一个字符串
char *uart_gets(char *s);

void uart_isr_receive(void);
void uart_enable_irq(void);
void uart_disable_irq(void);

#endif // UART_H
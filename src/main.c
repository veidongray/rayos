#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"

void main(void)
{
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    timer_init();
    asm volatile ("sti"); // Enable interrupts
    
    while (1) {
        asm volatile ("hlt\r\n");
    }
}

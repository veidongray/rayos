#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "systicks.h"

void exception_handler(uint32_t vector) {
    if (vector == IRQ0_VECTOR) {
        set_systicks(get_systicks() + 1);
        cga_printf("systicks = %u\n", get_systicks());
    } else {
        cga_printf("Unhandled exception: %d\n", vector);
        asm volatile ("cli; hlt"); // Completely hangs the computer
    }
    pic_sendEOI(vector);
}

#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "systicks.h"

void exception_handler(uint32_t vector) {
    if (vector == IRQ0_VECTOR) {
        set_systicks(get_systicks() + 1);
    } else if (vector == IRQ14_VECTOR) {
        cga_printf("Page fault!\n");
        disable_irq();
    } else {
        cga_printf("Unhandled exception: %d\n", vector);
        disable_irq();
    }
    pic_sendEOI(vector);
}

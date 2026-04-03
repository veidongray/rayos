#include <stdint.h>
#include "../inc/print.h"

void exception_handler(uint32_t vector) {
    vector = vector; // Suppress unused variable warning
    cga_printf("Unhandled exception: %d\n", vector);
    asm volatile ("cli; hlt"); // Completely hangs the computer
}

ENTRY(_start)
KERN_BASE = 0xC0100000;
VIRT_OFFSET = 0xC0000000;
SECTIONS
{
    . = KERN_BASE;
    _kernel_start = .;
    .text ALIGN(4K) : AT(ADDR(.text) - VIRT_OFFSET) {
	    *(.multiboot2)
	    *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - VIRT_OFFSET) {
	    *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - VIRT_OFFSET) {
	    *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - VIRT_OFFSET) {
	    *(.bss)
	    *(COMMON)
    }
    _kernel_end = .;
    . = ALIGN(4K);
    _kernel_end_aligned = .;
}

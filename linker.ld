ENTRY(_start)

KERNEL_VADDR = 0xC0010000;

SECTIONS
{
    . = KERNEL_VADDR;
    _kernel_start = .;
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VADDR) {
	*(.multiboot2)
	*(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VADDR) {
	*(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VADDR) {
	*(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VADDR) {
	*(.bss)
	*(COMMON)
    }
    _kernel_end = .;
    . = ALIGN(4K);
    _kernel_end_aligned = .;
}

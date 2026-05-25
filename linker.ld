ENTRY(_start)
VIRT_OFFSET = 0xffff800000000000;
KERN_BASE = VIRT_OFFSET + 0x100000;
SECTIONS
{
    . = KERN_BASE;
    _virt_offset = VIRT_OFFSET;
    _kernel_base = KERN_BASE;
    _kernel_virt_start = .;
    _kernel_phys_start = _kernel_virt_start - VIRT_OFFSET;

    .text ALIGN(4K) : AT(ADDR(.text) - VIRT_OFFSET) {
	    *(.multiboot2)
        *(.text.head)
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
    _kernel_virt_end = .;
    _kernel_phys_end = _kernel_virt_end - VIRT_OFFSET;
    . = ALIGN(4K);
    _kernel_virt_end_aligned = .;
    _kernel_phys_end_aligned = _kernel_virt_end_aligned - VIRT_OFFSET;
}

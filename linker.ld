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

    .initcalls ALIGN(8) : AT(ADDR(.initcalls) - VIRT_OFFSET) {
        __initcall_start = .;
        __initcall_0_start = .; KEEP(*(.initcall.0))
        __initcall_1_start = .; KEEP(*(.initcall.1))
        __initcall_2_start = .; KEEP(*(.initcall.2))
        __initcall_3_start = .; KEEP(*(.initcall.3))
        __initcall_4_start = .; KEEP(*(.initcall.4))
        __initcall_5_start = .; KEEP(*(.initcall.5))
        __initcall_6_start = .; KEEP(*(.initcall.6))
        __initcall_7_start = .; KEEP(*(.initcall.7))
        __initcall_end = .;
    }

    .exitcalls ALIGN(8) : AT(ADDR(.exitcalls) - VIRT_OFFSET) {
        __exitcall_start = .;
        __exitcall_7_start = .; KEEP(*(.exitcall.7))
        __exitcall_6_start = .; KEEP(*(.exitcall.6))
        __exitcall_5_start = .; KEEP(*(.exitcall.5))
        __exitcall_4_start = .; KEEP(*(.exitcall.4))
        __exitcall_3_start = .; KEEP(*(.exitcall.3))
        __exitcall_2_start = .; KEEP(*(.exitcall.2))
        __exitcall_1_start = .; KEEP(*(.exitcall.1))
        __exitcall_0_start = .; KEEP(*(.exitcall.0))
        __exitcall_end = .;
    }

    _kernel_virt_end = .;
    _kernel_phys_end = _kernel_virt_end - VIRT_OFFSET;
    . = ALIGN(4K);
    _kernel_virt_end_aligned = .;
    _kernel_phys_end_aligned = _kernel_virt_end_aligned - VIRT_OFFSET;
}

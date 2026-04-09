ENTRY(_start)

SECTIONS
{
    . = 1M;
    _kernel_start = .;
    .text ALIGN(4K) : {
	*(.multiboot2)
	*(.text)
    }

    .rodata ALIGN(4K) : {
	*(.rodata)
    }

    .data ALIGN(4K) : {
	*(.data)
    }

    .bss ALIGN(4K) : {
	*(.bss)
	*(COMMON)
    }
    _kernel_end = .;
    . = ALIGN(4K);
    _kernel_end_aligned = .;
}

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(entry)
SECTIONS {
    . = 0x100000;
    .entry ALIGN(4) : AT(0x0) {
        build/entry.o(.text)
    }

    .text ALIGN(4) : {
        *(.text)
    }

    .data ALIGN(4) : {
        *(.data)
    }

    .rodata ALIGN(4) : {
        *(.rodata)
    }

    .bss ALIGN(4) : {
        *(.bss)
    }
}
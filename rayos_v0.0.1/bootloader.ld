OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS {
    . = 0x7c00;
    . = ALIGN(4);
    .boot_text : AT(0x0) {
        *(.text)
    }

    . = 0x7da0;
    . = ALIGN(4);
    .boot_gdt : AT(0x1a0) {
        *(.gdt)
    }

    .boot_flag : AT(0x200 - 0x2) {
        *(.bootflag)
    }
}
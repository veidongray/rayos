OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS {
    . = 0x7c00;
    .text ALIGN(4) : AT(0x0) {
        *(.text)
    }

    .flags ALIGN(4) : AT(0x200 - 0x2) {
        *(.boot_flags)
    }
}
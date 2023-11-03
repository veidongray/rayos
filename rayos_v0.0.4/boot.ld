OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS {
    . = 0x7c00;
    . = ALIGN(4);

    .text : AT(0x0) {
        *(.text)
    }

    .boot_flags : AT(0x200 - 0x2) {
        *(.boot_flags)
    }

    /DISCARD/ : {
        *(.eh_frame)
    }
}
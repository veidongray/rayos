OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(entry)
SECTIONS {
    . = 0x100000;
    . = ALIGN(4);
    .first_text : {
        entry.o(.text)
    }

    .second_text : {
        *(.text)
    }
}
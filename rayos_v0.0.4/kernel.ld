OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(entry)
SECTIONS {
    . = 0x10000;
    . = ALIGN(4096);
    .text : {
        entry.o(.text)
    }
}
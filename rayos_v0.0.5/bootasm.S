#include "asm.h"

    .code16
    .global start
start:
    cli

    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

seta20_1:
    inb     $0x64, %al
    testb   $0x2, %al
    jnz     seta20_1

    movb    $0xd1, %al
    outb    %al, $0x64

seta20_2:
    inb     $0x64, %al
    testb   $0x2, %al
    jnz     seta20_2

    movb    $0xdf, %al
    outb    %al, $0x60

    lgdt    gdtdesc
    movl    %cr0, %eax
    orl     $0x00000001, %eax
    movl    %eax, %cr0

    ljmp    $(0x1 << 3), $start32

    .code32
start32:
    movw    $(0x2 << 3), %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    $0x0, %ax
    movw    %ax, %fs
    movw    %ax, %gs

    movl    $0x10000, %esp
    call    bootmain

    # If bootmain returns, trigger a Bochs
    # breakpoint if running under Bochs, then loop.
    movw    $0x8a00, %ax
    movw    %ax, %dx
    outw    %ax, %dx
    movw    $0x8ae0, %ax
    outw    %ax, %dx
spin:
    jmp spin

    .align 4
gdt:
    GDTSEGASM(0, 0, 0, 0, 0, 0, 0, 0, 0)
    GDTSEGASM(0x0, 0xfffff, 1, 1, 0, 1, 0, 1, 0xa)
    GDTSEGASM(0x0, 0xfffff, 1, 1, 0, 1, 0, 1, 0x2)

gdtdesc:
    .word       (gdtdesc - gdt - 1)
    .long       gdt

    ## boot flags
    .section    ".boot_flags", "a"
flags:
    .byte 0x55, 0xaa
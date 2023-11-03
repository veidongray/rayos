#define KERNEL_GDT_CODESEG  (0x1 << 3)
#define KERNEL_GDT_DATASEG  (0x2 << 3)
#define GDTSEGASM(BASE, LIMIT, G, DoB, AVL, P, DPL, S, TYPE) \
.word (LIMIT & 0xffff), (BASE & 0xffff);\
.byte ((BASE >> 16) & 0xff), (((P << 7) | (DPL << 5) | (S << 4)) | TYPE),\
(((G << 7) | (DoB << 6) | 0 | (AVL << 4)) | ((LIMIT >> 16) & 0xf)),\
((BASE >> 24) & 0xff)

    .code16
    .section    .text
    .global     start
start:
    ## close interrupt
    cli

    ## init regs
    xorl     %eax, %eax
    movw     %ax, %ds
    movw     %ax, %es
    movw     %ax, %ss
    movw     %ax, %fs
    movw     %ax, %gs

    ## open A20 address line
seta20_1:
    in      $0x64, %al
    testb    $0x2, %al
    jnz     seta20_1

    movb     $0xd1, %al
    out     %al, $0x64

seta20_2:
    in      $0x64, %al
    testb    $0x2, %al
    jnz     seta20_2

    movb     $0xdf, %al
    out     %al, $0x60

    ## load gdt base to gdtr
    lgdt    gdtdesc

    ## open pm
    movl     %cr0, %eax
    orl      $0x1, %eax
    movl     %eax, %cr0

    ## flush cpu stream to new 32bit code
    ljmp    $KERNEL_GDT_CODESEG, $start32
    .code32
start32:
    movl     $KERNEL_GDT_DATASEG, %eax
    movw     %ax, %ds
    movw     %ax, %es
    movw     %ax, %ss
    xorl     %eax, %eax
    movw     %ax, %fs
    movw     %ax, %gs

    ## load kernel
load_kernel:
    movl     $0x7c00, %esp
    call bootc

    ## start to kernel
start_kernel:
    pushl    $gdt                       # transmit gdt address
    call    0x10000
    hlt

io_delay:
    nop
    nop
    nop
    nop
    ret

    ## gdt defined
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
.section .note.GNU-stack,"",@progbits
.section .multiboot2
.align 8
mboot:
    .long 0xe85250d6          # magic
    .long 0                   # architecture (0 = i386)
    .long end_mboot - mboot   # total size
    .long -(0xe85250d6 + 0 + (end_mboot - mboot))  # checksum

    # End tag (mandatory!)
    .word 0                   # type = 0 (end tag)
    .word 0                   # flags = 0
    .long 8                   # size = 8 bytes
end_mboot:

.section .text
.code32                       # 切换到 32 位模式
.global _start
_start:
    # Set up the stack
    mov $stack_top, %esp
    call main
    hlt

.section .bss
.align 16
stack:
    .space 4096  # 4KB stack
stack_top:

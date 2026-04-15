.section .note.GNU-stack,"",@progbits
.section .text
.global default_isr
default_isr:
.default_loop:
    hlt
    jmp .default_loop
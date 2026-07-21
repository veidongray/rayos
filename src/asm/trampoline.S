.section .note.GNU-stack,"",@progbits

.section .text.trampoline, "ax"
.code16
.align 16
.global trampoline
trampoline:
    cli

    mov $0x3F8, %dx     # COM1 串口端口

    # 获取 Local APIC ID
    mov $0xFEE00020, %esi
    mov (%esi), %eax
    shr $24, %eax               # APIC ID 在高 8 位
    mov %al, %bl                # 保存到 BL

    # 打印 "AP" + ID + " running"
    mov $'A', %al
    out %al, %dx
    mov $'P', %al
    out %al, %dx

    # 打印 APIC ID (0-9)
    mov %bl, %al
    add $'0', %al
    out %al, %dx

    mov $' ', %al
    out %al, %dx

    # running
    mov $'r', %al; out %al, %dx
    mov $'u', %al; out %al, %dx
    mov $'n', %al; out %al, %dx
    mov $'n', %al; out %al, %dx
    mov $'i', %al; out %al, %dx
    mov $'n', %al; out %al, %dx
    mov $'g', %al; out %al, %dx

    mov $'\r', %al
    out %al, %dx
    mov $'\n', %al
    out %al, %dx

l:
    hlt
    jmp l

.global trampoline_end
trampoline_end:
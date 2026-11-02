# RayOS 开发手记：从零构建 X86_64 操作系统

## 简介

RayOS 是一个基于 X86_64 架构的操作系统。从小学接触计算机开始，我就怀揣着一个自制 OS 的梦想，总想着能自研系统改变世界。虽然事与愿违，世界暂时还没被改变，但能有口饭吃也挺好的 :)

写 RayOS 既是为了圆儿时的梦，也是为了巩固计算机底层知识。项目中参考了许多 Linux 的设计（直到动手写才真正体会到 UNIX 哲学的精妙）。当然，类似的优秀项目还有很多，比如 XV6、Minix（坦白说，我还没认真研读过它们的源码，这个坑以后一定得补上）。

下面我将对项目的核心代码进行大致讲解，希望能帮助自己溯源巩固，也希望能帮助有兴趣的同学快速理解代码含义。

## 1. 使用 Grub 引导

为了方便启动，RayOS 直接使用 Grub 引导，并通过 `grub-mkrescue` 构建 ISO 镜像。由于 Grub 默认会进入保护模式，因此在我们的启动代码（`src/asm/head.S`）中省略了实模式相关的代码，直接配置页表并进入长模式（Long Mode）。

## 2. 启动代码解析

启动代码位于 `src/asm/head.S`。

### 2.1 Multiboot2 协议头

代码头部的第一部分是 Grub 相关的 Multiboot2 协议头。使用这个协议可以获取 Grub 传递的系统信息（例如内存总大小等）。

```asm
.section .multiboot2
.align 8
_mboot:
    .long 0xe85250d6
    .long 0
    .long _end_mboot - _mboot
    .long -(0xe85250d6 + 0 + (_end_mboot - _mboot))

    .word 6, 0, 8
    .word 0, 0, 8
_end_mboot:
```

### 2.2 初始化与页表构建

真正进入可执行代码后，首先要保存 Multiboot 的地址和魔术数（Magic Number），以便进入 C 语言环境后使用。

接下来需要关闭分页，因为 Grub 可能已经开启了分页。我们需要在关闭分页的情况下配置长模式的页表。长模式使用四级页表，分别是 PML4、PDPT、PD、PT。每个表有 512 项，寻址能力如下：

- **PML4**：每项可寻址 512G
- **PDPT**：每项寻址 2G
- **PD**：每项寻址 2M
- **PT**：每项寻址 4K（页内寻址，页大小 4K）

这里我们将物理地址的 `0~128M` 映射到两个虚拟地址区间：

1. `0x00000000_00000000 ~ 0x00000000_08000000`（低端恒等映射）
2. `0xffff8000_00000000 ~ 0xffff8000_08000000`（高端 Higher-half 映射）

这部分临时映射的 128M 足够系统初始化使用，后续会在进入 C 语言环境后根据实际物理内存大小重新映射。

```asm
    # =====================================================================
    # 1. 创建 64-bit 页表（Higher-half 映射）
    # =====================================================================
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    xorl %eax, %eax
    movl $4096*4/4, %ecx          # 清空 16KB 页表空间
    rep stosl

    # PML4[0]   → PDPT（低端恒等映射，方便过渡）
    # PML4[256] → PDPT（高端 higher-half）
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    movl $(_boot_pdpt - VIRT_OFFSET), %eax
    orl  $0x3, %eax               # 设置 Present + Writable 位

    movl %eax, (%edi)             # PML4[0]
    movl %eax, 256*8(%edi)        # PML4[256]
    movl %edi, 511*8(%edi)        # 递归映射 PML4[511]
    addl $0x3, 511*8(%edi)

    # PDPT[0] → PD（低端 0~1GB）
    movl $(_boot_pdpt - VIRT_OFFSET), %edi
    movl $(_boot_pd - VIRT_OFFSET), %eax
    orl  $0x3, %eax
    movl %eax, (%edi)             # PDPT[0]

    # PD[0] → PT0
    movl $(_boot_pd - VIRT_OFFSET), %edi
    movl $(_boot_pt - VIRT_OFFSET), %eax
    orl  $0x3, %eax
    movl $0x0, %edx
    movl $NUM_PT, %ecx

0:
    movl %eax, (%edi, %edx, 8)
    addl $0x1000, %eax
    incl %edx
    decl %ecx
    jnz 0b

    # Setup PT
    movl $(_boot_pt - VIRT_OFFSET), %edi
    movl $0x00000003, %eax          # Present + Writable
    movl $(512 * NUM_PT), %ecx

1:  movl %eax, (%edi)               # 低 32 位
    movl $0, 4(%edi)                # 高 32 位清零
    addl $0x1000, %eax
    addl $8, %edi
    decl %ecx
    jnz 1b
```

### 2.3 进入长模式

接下来开始正式进入长模式。步骤如下：

1. 设置 PAE（物理地址扩展）。
2. 打开 LME（长模式使能）。
3. 开启 PG（分页）。
4. 加载新的 GDT（这里只是临时的，后面会在 `gdt_init()` 中重新初始化）。
5. 使用远跳转（Far Jump/Return）跳到 64 位模式。注意这里不能用 `ljmp`，因为它会汇编码成 32 位指令，无法正确进入 64 位。

跳转进入 64 位模式后，初始化段寄存器，设置临时的 C 语言栈（C 语言环境有大量栈操作，必须有正确的栈空间），最后通过间接调用进入 C 语言环境的入口 `start_kernel()`。

```asm
    # =====================================================================
    # 2. 进入长模式
    # =====================================================================
    movl $(_boot_pml4 - VIRT_OFFSET), %eax
    movl %eax, %cr3                 # 加载页表基地址

    movl %cr4, %eax
    orl  $(1 << 5), %eax            # 开启 PAE
    movl %eax, %cr4

    movl $0xC0000080, %ecx          # EFER MSR
    rdmsr
    orl  $(1 << 8), %eax            # 开启 LME
    wrmsr

    movl %cr0, %eax
    orl  $0x80000000, %eax          # 开启 PG (分页)
    movl %eax, %cr0

    # 加载 64-bit GDT
    movl $(_gdt64_ptr - VIRT_OFFSET), %eax
    lgdt (%eax)

    # 远跳转进入长模式
    pushl $0x0008                    # 代码段选择子
    pushl $_start64 - VIRT_OFFSET    # 64位目标地址
    lret                             # 执行远返回，切换 CS 并跳转

# =====================================================================
.code64
_start64:
    # 初始化段寄存器
    movabsq $0x10, %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    # 现在可以安全使用高端地址了，设置内核栈
    movabsq $_kstack_top, %rsp
    movabsq $start_kernel, %rax
    call *%rax                      # 进入 C 语言入口

_loop:
    hlt
    jmp _loop

# =====================================================================
.align 4096
.section .data
_boot_pml4: .space 4096
_boot_pdpt: .space 4096
_boot_pd:   .space 4096
_boot_pt:   .space 4096 * NUM_PT

.align 8
_gdt64:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff   # 64-bit code segment
    .quad 0x00cf92000000ffff   # 64-bit data segment
_gdt64_end:

_gdt64_ptr:
    .word _gdt64_end - _gdt64 - 1
    .long _gdt64 - VIRT_OFFSET     # 32位阶段使用物理地址
```

整个启动代码的汇编部分逻辑非常清晰：初始化页表 -> 打开分页 -> 设置 GDT -> 远跳转进入 64 位 -> 设置初始栈 -> 间接调用进入 C 语言环境。进入 C 语言环境后，开发效率会大大提高，毕竟高效编码还是得靠 C，除非你想手撕汇编写 OS :)

## 3. 读取 Grub 提供的内存信息

Grub 在启动时会进行相关初始化，将系统从实模式初始化至保护模式，并使用 Tag 机制保存和传递系统信息。我们需要在 C 语言环境下读取这些地址，获取系统内存大小，以便后续进行内存管理。

```c
#include <multiboot2.h>

static uint64_t total_mem = 0;

extern uint32_t _mboot_info[]; // 在 head.S 中保存的 boot 信息
extern uint32_t _mboot_magic[]; // 在 head.S 中保存的 boot 魔术值

void parse_multiboot2_mmap(void *mbi_addr)
{
    // 跳过 total_size 和 reserved 字段
    uint8_t *ptr = (uint8_t *)mbi_addr + 8;
    uint8_t *end = (uint8_t *)mbi_addr + *(uint32_t *)mbi_addr;

    while (ptr < end) {
        struct multiboot2_tag *tag = (struct multiboot2_tag *)ptr;
        if (tag->type == 0)
            break; // 结束标签

        if (tag->type == MULTIBOOT2_TAG_MMAP) {
            // 跳过 type, size, entry_size, version
            uint8_t *entry_ptr = ptr + 16;
            uint8_t *entry_end = ptr + tag->size;

            while (entry_ptr < entry_end) {
                struct multiboot2_mmap_entry *e =
                    (struct multiboot2_mmap_entry *)entry_ptr;
                
                if (e->type == 1) { // 可用内存
                    total_mem += e->len;
                }
                entry_ptr += 24; // sizeof(multiboot2_mmap_entry)
            }
            // total_mem 即为物理 RAM 大小（字节）
        }
        // 对齐到 8 字节边界
        ptr += (tag->size + 7) & ~7;
    }
}

uint64_t get_total_mem(void) { return total_mem; }

void total_memory_init(void)
{
    if (_mboot_magic[0] == 0x36d76289) {
        parse_multiboot2_mmap((void *)((uint64_t)_mboot_info[0]));
    }
}
```

---

### 版本二：深度重构版（结构更严谨，适合技术文档）

# RayOS：基于 X86_64 的操作系统内核引导流程详解

## 项目背景

RayOS 是一个从零开始构建的 X86_64 架构操作系统内核。开发 RayOS 的初衷源于对计算机底层原理的探索欲，旨在通过实践巩固操作系统、计算机组成原理等核心知识。

在设计上，RayOS 参考了 Linux 的许多经典设计思想，力求在简洁的代码中体现 UNIX 哲学的精妙。本文档将详细解析 RayOS 从加电到进入 C 语言环境的完整引导流程。

## 1. 引导加载：Grub 与 Multiboot2

为了简化引导加载器的开发，RayOS 采用 Grub 作为引导加载程序，并使用 `grub-mkrescue` 构建可启动 ISO。

Grub 加载内核后，默认会将 CPU 置于保护模式（Protected Mode）。因此，RayOS 的汇编入口代码（`src/asm/head.S`）无需处理实模式到保护模式的切换，而是直接着手配置长模式（Long Mode）所需的页表。

内核头部遵循 **Multiboot2 协议**，定义了标准的引导头结构，以便 Grub 能够正确识别并传递系统信息（如内存映射）。

```asm
.section .multiboot2
.align 8
_mboot:
    .long 0xe85250d6  # Magic Number
    .long 0           # Architecture (0 for i386)
    .long _end_mboot - _mboot # Header length
    .long -(0xe85250d6 + 0 + (_end_mboot - _mboot)) # Checksum
    # ... 标签定义 ...
_end_mboot:
```

## 2. 汇编引导阶段 (`head.S`)

汇编引导阶段主要完成以下核心任务：

1. **构建临时页表**：实现 Higher-half 内核映射。
2. **开启长模式**：配置 CR0, CR4, EFER 寄存器。
3. **环境切换**：加载 GDT，切换栈指针，跳转至 C 语言入口。

### 2.1 构建 Higher-half 页表

长模式采用四级分页机制（PML4 -> PDPT -> PD -> PT）。为了支持 Higher-half 内核（即内核运行在高地址空间，如 `0xffff8000...`），我们需要在汇编阶段建立临时的映射关系。

代码将物理内存的低 128MB 同时映射到低地址（恒等映射）和高地址（Higher-half），确保开启分页后 CPU 能正确取指。

```asm
    # 1. 清空页表内存区域
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    xorl %eax, %eax
    movl $4096*4/4, %ecx
    rep stosl

    # 2. 设置 PML4 表项
    # PML4[0] 指向 PDPT (用于低端恒等映射)
    # PML4[256] 指向 PDPT (用于高端 Higher-half 映射)
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    movl $(_boot_pdpt - VIRT_OFFSET), %eax
    orl  $0x3, %eax               # Present + Writable
    movl %eax, (%edi)             # PML4[0]
    movl %eax, 256*8(%edi)        # PML4[256]
    
    # ... 后续 PDPT, PD, PT 的填充逻辑 ...
```

### 2.2 开启长模式与切换环境

页表构建完成后，按以下步骤开启长模式：

1. **加载 CR3**：指向 PML4 基地址。
2. **开启 PAE**：设置 CR4.PAE。
3. **开启 LME**：设置 EFER.LME。
4. **开启分页**：设置 CR0.PG。

开启分页后，立即加载 64 位 GDT，并通过 `lret`（远返回）指令刷新 CS 段寄存器，正式进入 64 位代码段。

```asm
    # 开启 PAE, LME, PG ...
    # 加载 GDT
    lgdt (%eax)

    # 远跳转进入 64 位模式
    pushl $0x0008                 # Code Segment Selector
    pushl $_start64 - VIRT_OFFSET # Entry Point
    lret                          # Jump to 64-bit mode

.code64
_start64:
    # 初始化数据段寄存器
    movabsq $0x10, %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    # ...
    
    # 设置内核栈并调用 C 入口
    movabsq $_kstack_top, %rsp
    movabsq $start_kernel, %rax
    call *%rax
```

## 3. C 语言初始化：解析内存信息

进入 `start_kernel` 后，首要任务之一是解析 Grub 传递的 Multiboot2 信息结构，获取物理内存布局（Memory Map），为后续的物理内存管理分配器提供数据支持。

```c
void parse_multiboot2_mmap(void *mbi_addr)
{
    uint8_t *ptr = (uint8_t *)mbi_addr + 8;
    uint8_t *end = (uint8_t *)mbi_addr + *(uint32_t *)mbi_addr;

    while (ptr < end) {
        struct multiboot2_tag *tag = (struct multiboot2_tag *)ptr;
        if (tag->type == 0) break; // End tag

        if (tag->type == MULTIBOOT2_TAG_MMAP) {
            // 遍历内存映射条目，累加可用内存
            // ...
        }
        ptr += (tag->size + 7) & ~7; // 8-byte alignment
    }
}
```

# RayOS

RayOS 是一个基于 x86_64 架构的实验型操作系统内核项目，目标是从零开始实现一个可引导、可运行、且便于继续扩展的裸机内核原型。项目重点放在操作系统内核核心机制的理解与验证，而不是追求完整桌面系统体验。

## Project Positioning

-   教学/实验型内核项目
-   以源码阅读、机制验证和逐步扩展为主
-   当前重点是引导、内存管理、中断处理、任务调度、文件系统与用户态交互等基础能力

## Current Implementation Status

项目已经具备较完整的内核雏形，并且基本能够通过 GRUB 引导进入内核并运行。当前已实现的核心模块包括：

-   引导与启动
    -   GRUB / Multiboot2 引导环境支持
    -   汇编入口和 C 语言内核初始化流程
-   CPU 与中断
    -   GDT / IDT 初始化
    -   PIC、IOAPIC、LAPIC 中断控制器驱动
    -   基础异常与中断处理框架
    -   基于 ACPI MADT 的多核检测与 AP 启动
    -   LAPIC INIT/SIPI 与 AP trampoline 次级 CPU 初始化
-   内存管理
    -   物理内存初始化
    -   分页与页表管理
    -   基础内存池 / 分配器相关实现
-   内核工具
    -   简单算法模块实现，用于内核内部通用运算支持
-   任务与调度
    -   基础线程与任务创建框架
    -   简易的内核线程运行流程
    -   用户态 ELF 程序加载与执行流程
    -   支持核心空闲线程与多核任务启动
-   设备与输入输出
    -   UART 串口输出与基础输入处理
    -   ACPI 与 PCI 初始化逻辑
-   文件系统
    -   VFS 抽象层
    -   文件系统管理相关代码
    -   块设备、磁盘镜像与文件访问接口
    -   tmpfs / FAT32 等相关文件系统支持
-   用户态支持
    -   简单的用户态 init 程序
    -   系统调用框架与基础 libc 封装

## Project Goals

RayOS 的主要目标是：

-   理解 x86_64 架构下操作系统内核的基本工作方式
-   通过实际代码搭建完整的引导、初始化和内核运行流程
-   逐步完善内存管理、中断处理、任务调度和文件系统能力
-   为后续增加 shell、驱动、用户态程序和更稳定的系统服务打基础

## Project Structure

```text
.
├── inc/                  # 内核头文件与公共接口定义
│   ├── asm/              # 汇编相关头文件
│   ├── lib/              # 字符串/打印库头文件
│   ├── user/             # 用户态系统调用头文件
│   └── ...               # 内存、任务、PCI、VFS 等模块头文件
├── src/                  # 内核实现源码
│   ├── asm/              # 汇编启动代码与中断桩代码
│   ├── fs/               # 文件系统实现
│   ├── mm/               # 内存管理相关实现
│   ├── lib/              # 字符串格式化处理库相关实现
│   ├── user/             # 用户态 init 与系统调用实现
│   └── ...               # 中断、任务、PCI、UART 等模块源码
├── iso/                  # GRUB 配置与 ISO 镜像构建文件
├── linker.ld             # 内核链接脚本
├── Makefile              # 构建、运行与调试脚本
└── tools/                # 构建辅助脚本与检查脚本
```

## Prerequisites

建议在 Ubuntu 或其他基于 Debian 的 Linux 发行版上构建和运行，至少需要安装以下依赖：

-   make
-   gcc / clang
-   ld
-   grub-mkrescue / xorriso
-   qemu-system-x86_64
-   mkfs.fat
-   mtools
-   gdb

也可以先执行下面的脚本检查当前环境是否满足构建条件：

```bash
make check-deps
# or
python3 tools/check-deps.py
```

## Quick Start

### 1. Compile Kernel

```bash
make build
```

### 2. Generate Bootable ISO Image

```bash
make iso
```

### 3. Run in QEMU

```bash
make qemu
```

### 4. Debug Mode

```bash
make qemudbg
# or
make qemugdb
```

## Common Commands

```bash
help         help: Show help message
check-deps   check-deps: Check software dependencies
build        build: Compile kernel and generate executable (parallel-safe)
disk         disk: Create FAT32 disk image (incremental)
iso          iso: Generate bootable GRUB ISO
qemu         qemu: Run system in QEMU
qemudbg      qemudbg: Run QEMU with debug logging
qemugdb      qemugdb: Run QEMU and wait for GDB connection
clean        clean: Remove build artifacts except disk image
distclean    distclean: Remove all build artifacts including user programs and disk
rebuild      rebuild: Clean and rebuild from scratch
format       format: Format all C/C++ source files
```

## Features & Limitations

-   当前仍属于教学 / 实验型内核实现，适合学习和源码阅读。
-   用户态能力仍较基础，尚未提供完整 shell 或复杂应用环境。
-   部分子系统仍在演进中，稳定性与兼容性还有待进一步验证。
-   代码风格偏向教学与可维护性，部分实现仍有继续优化的空间。

## Roadmap

接下来可以继续推进以下方向：

-   完善进程与线程调度逻辑
-   增强系统调用机制与用户态交互
-   提升文件系统功能与稳定性
-   引入更友好的 shell 与终端交互
-   补充更多调试工具与测试流程

## License

本项目当前尚未显式声明许可证。如果你希望参与开发或进行二次使用，请先与项目维护者确认。
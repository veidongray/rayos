# RayOS

RayOS 是一个基于 x86_64 架构的实验型操作系统内核项目，当前已经具备较完整的内核雏形。项目重点是通过从零实现引导、内存管理、中断处理、任务调度、文件系统与用户态交互等机制，构建一个可运行并便于继续扩展的裸机内核原型。

## Current Status

截至目前，项目已经实现并集成了以下核心模块：

- x86_64 平台下的内核启动与初始化流程
- GRUB / Multiboot2 引导环境支持
- GDT、IDT 与基础中断处理框架
- PIC、IOAPIC、LAPIC 中断控制器初始化
- ACPI 与 PCI 相关初始化逻辑
- 物理内存与分页管理
- 基础线程与任务管理
- UART 串口输出与日志打印
- VFS 与 FAT32 文件系统相关代码
- 用户态初始化程序与系统调用框架

当前项目仍然偏向“教学与实验型内核实现”，重点在于验证内核机制的正确性和可扩展性，而不是追求完整桌面系统体验。

## Project Goals

RayOS 的目标是：

- 理解 x86_64 架构下操作系统内核的基本工作方式
- 通过实际代码搭建完整的引导与内核初始化流程
- 逐步完善内存管理、中断处理和任务调度能力
- 为文件系统与用户态程序加载提供可扩展基础

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
└── Makefile              # 构建、运行与调试脚本
```

## Requirements

建议在 Ubuntu 或其他基于 Debian 的 Linux 发行版上构建和运行，需安装以下依赖：

- make
- gcc / clang
- ld
- grub-mkrescue
- qemu-system-x86_64
- mkfs.fat

## Getting Started

### 1. 编译内核

```bash
make build
```

### 2. 生成可引导 ISO 镜像

```bash
make iso
```

### 3. 在 QEMU 中运行

```bash
make qemu
```

### 4. 调试模式

```bash
make qemugdb
```

## Useful Commands

```bash
make build       # 编译内核
make iso         # 生成可启动 ISO
make qemu        # 启动 QEMU
make qemudbg     # 启动带调试日志的 QEMU
make qemugdb     # 启动 QEMU 并等待 GDB 连接
make clean       # 清理编译产物
make cleanall    # 清理编译产物和磁盘镜像
make rebuild     # 从干净状态重新构建项目
```

## Development Notes

- 本项目以学习、实验和源码阅读为主要目的，代码风格偏向教学与可维护性。
- 目前更关注内核机制的正确性与可扩展性，而非完整的用户体验。
- 代码中保留了较多的模块化结构，方便继续扩展驱动、调度和文件系统能力。

## Roadmap

接下来可以继续推进以下方向：

- 完善进程与线程调度逻辑
- 增强系统调用机制与用户态交互
- 提升文件系统功能与稳定性
- 引入更友好的 shell 与终端交互
- 补充更多调试工具与测试流程

## License

本项目当前尚未显式声明许可证。如果你希望参与开发或进行二次使用，请先与项目维护者确认。
# RayOS

RayOS 是一个基于 x86_64 架构的实验型操作系统内核项目，致力于通过从零实现内核核心机制，帮助开发者理解操作系统的启动、内存管理、中断处理、任务调度与文件系统结构。

## Overview

RayOS 目前处于教学与实验性质的开发阶段，重点是构建一个可运行、可扩展的内核原型，而不是追求完整的桌面系统体验。项目使用 GRUB 引导，并通过 QEMU 进行虚拟机运行与调试。

## Features

- x86_64 架构内核启动流程
- GRUB / Multiboot2 引导支持
- GDT、IDT 与中断处理初始化
- PIC、IOAPIC、LAPIC 中断控制器支持
- 物理内存与分页管理
- 基础线程与任务管理
- UART 串口输出与简单调试输出
- VFS 与 FAT32 文件系统支持
- PCI 与 ACPI 初始化
- 用户态初始化程序与系统调用框架
- 可生成 ISO 镜像并在 QEMU 中运行

## Project Structure

```text
.
├── inc/                # 内核头文件与公共接口定义
├── src/                # 内核实现源码
│   ├── asm/            # 汇编启动代码与中断/异常桩代码
│   ├── fs/             # 文件系统实现
│   └── user/           # 用户态初始化程序与系统调用
├── iso/                # GRUB 配置与 ISO 构建文件
├── linker.ld           # 内核链接脚本
└── Makefile            # 构建、运行与调试脚本
```

## Requirements

建议在 Ubuntu 环境中进行构建和运行，需安装以下依赖：

- make
- xorriso
- GCC / Clang
- ld
- grub-mkrescue
- qemu-system-x86_64
- mkfs.fat

## Getting Started

### Build

```bash
make build
```

### Create bootable ISO

```bash
make iso
```

### Run in QEMU

```bash
make qemu
```

### Debug mode

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
make rebuild     # 重新构建整个项目
```

## Development Notes

- 本项目以学习与实验为主，代码结构偏向教学性与可读性。
- 编译输出要求保持 0 Errors、0 Warnings。
- 欢迎围绕内核机制、驱动支持、文件系统与调试体验进行扩展。

## Roadmap

未来可以继续推进以下方向：

- 完善进程与线程调度
- 增加更完整的系统调用机制
- 提升文件系统稳定性与功能覆盖
- 引入更友好的 shell 与终端交互
- 补充更多内核调试与测试流程

## License

本项目当前未显式声明许可证，若你希望参与开发或二次使用，请先与项目维护者确认。
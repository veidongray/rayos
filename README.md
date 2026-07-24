<p align="center">
  <b>English</b> | <a href="./README_CN.md">中文</a>
</p>

# RayOS

RayOS is an experimental operating system kernel project based on the x86_64 architecture. It aims to build a bootable, runnable, and extensible bare-metal kernel prototype from scratch. The project focuses on understanding and verifying core OS kernel mechanisms rather than delivering a complete desktop experience.

## Project Positioning

- Educational and experimental kernel project.
- Focuses on source code reading, mechanism verification, and incremental extension.
- Current priorities include booting, memory management, interrupt handling, task scheduling, file systems, and user-space interaction.

## Current Implementation Status

The project has achieved a relatively complete kernel prototype and can successfully boot and run via GRUB. Currently implemented core modules include:

- **Boot & Startup**
  - GRUB / Multiboot2 boot environment support.
  - Assembly entry point and C language kernel initialization flow.
- **CPU & Interrupts**
  - GDT / IDT initialization.
  - PIC, IOAPIC, and LAPIC interrupt controller drivers.
  - Basic exception and interrupt handling framework.
  - Multi-core detection and AP startup based on ACPI MADT.
  - Secondary CPU initialization via LAPIC INIT/SIPI and AP trampoline.
- **Memory Management**
  - Physical memory initialization.
  - Paging and page table management.
  - Basic memory pool / allocator implementation.
- **Kernel Utilities**
  - Simple algorithm modules for general-purpose internal computations.
- **Tasks & Scheduling**
  - Basic thread and task creation framework.
  - Simple kernel thread execution flow.
  - User-space ELF program loading and execution.
  - Support for core idle threads and multi-core task startup.
- **Devices & I/O**
  - UART serial output and basic input handling.
  - ACPI and PCI initialization logic.
- **File System**
  - VFS abstraction layer.
  - File system management infrastructure.
  - Block device, disk image, and file access interfaces.
  - tmpfs / FAT32 file system support.
- **User-Space Support**
  - Simple user-space init program.
  - System call framework and basic libc wrappers.

## Project Goals

- Understand the fundamental workings of an OS kernel on x86_64 architecture.
- Build a complete boot, initialization, and runtime flow through practical coding.
- Incrementally improve memory management, interrupt handling, task scheduling, and file system capabilities.
- Lay the foundation for future additions like shells, drivers, user-space applications, and stable system services.

## Project Structure

```text
.
├── inc/                  # Kernel headers & public API definitions
│   ├── asm/              # Assembly-related headers
│   ├── lib/              # String/print library headers
│   ├── user/             # User-space syscall headers
│   └── ...               # Memory, task, PCI, VFS headers
├── src/                  # Kernel implementation source
│   ├── asm/              # Assembly boot & interrupt stubs
│   ├── fs/               # File system implementation
│   ├── mm/               # Memory management implementation
│   ├── lib/              # String formatting library
│   ├── user/             # User-space init & syscalls
│   └── ...               # Interrupt, task, PCI, UART sources
├── iso/                  # GRUB config & ISO build files
├── linker.ld             # Kernel linker script
├── Makefile              # Build, run & debug scripts
└── tools/                # Build helper & check scripts
```

## Prerequisites

It is recommended to build and run on Ubuntu or other Debian-based Linux distributions. The following dependencies are required:

- make
- gcc / clang
- ld
- grub-mkrescue / xorriso
- qemu-system-x86_64
- mkfs.fat
- mtools
- gdb

You can verify your environment by running:

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

| Command | Description |
| :--- | :--- |
| `help` | Show help message |
| `check-deps` | Check software dependencies |
| `build` | Compile kernel and generate executable (parallel-safe) |
| `disk` | Create FAT32 disk image (incremental) |
| `iso` | Generate bootable GRUB ISO |
| `qemu` | Run system in QEMU |
| `qemudbg` | Run QEMU with debug logging |
| `qemugdb` | Run QEMU and wait for GDB connection |
| `clean` | Remove build artifacts except disk image |
| `distclean` | Remove all build artifacts including user programs and disk |
| `rebuild` | Clean and rebuild from scratch |
| `format` | Format all C/C++ source files |

## Features & Limitations

- This is currently an educational/experimental kernel implementation, suitable for learning and source code reading.
- User-space capabilities remain basic; no complete shell or complex application environment is provided yet.
- Some subsystems are still evolving; stability and compatibility require further verification.
- Code style prioritizes readability and maintainability for educational purposes, leaving room for further optimization.

## Roadmap

- Improve process and thread scheduling logic.
- Enhance system call mechanisms and user-space interaction.
- Improve file system functionality and stability.
- Introduce a more user-friendly shell and terminal interface.
- Add more debugging tools and testing workflows.

## License

This project is licensed under the [MIT License](LICENSE).

You are free to use, copy, modify, merge, publish, distribute, sublicense, and sell copies of this software for any purpose, including commercial use, provided that the copyright notice and permission notice are included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. See the [LICENSE](LICENSE) file for full details.

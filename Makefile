# RayOS 构建脚本
# 负责编译内核、生成可启动镜像、运行 QEMU 以及清理构建产物。

# 最终参与链接的内核对象文件列表。
# 这些对象文件分别来自汇编启动代码、内核主体、文件系统以及基础库。
BUILT-IN = 	src/asm/built-in.o			\
			src/built-in.o				\
			src/fs/built-in.o			\
			src/lib/built-in.o			\
			src/mm/built-in.o

# 头文件搜索路径。
# 将内核公共头文件目录以及字符串/打印库头文件目录加入编译器搜索路径。
INCDIR = -I $(CURDIR)/inc -I $(CURDIR)/inc/lib -I $(CURDIR)/inc/user

# 用于构建 freestanding 内核的编译选项。
# 这些参数会关闭大部分标准库依赖和栈保护机制，适合裸机/内核环境。
CFLAGS = -m64 -fno-pic                      \
    -ffreestanding                          \
    -fno-builtin                            \
    -fno-stack-protector                    \
    -mno-red-zone                           \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387  \
    -Wall -Wextra                           \
    -g -fno-omit-frame-pointer              \
    -fno-asynchronous-unwind-tables         \
	-nostdlib 								\
    -std=gnu99                              \
	-mcmodel=large							\
    $(INCDIR)

# 链接器使用的 ELF64 x86_64 目标格式。
LDFLAGS = -m elf_x86_64

# 输出 ISO 镜像文件名。
ISO := rayos.iso

# QEMU 虚拟机执行器路径。
QEMU := qemu-system-x86_64

# QEMU 的基础启动参数。
# 使用 q35 平台、2 核 CPU、128MB 内存，并挂载 ISO 和磁盘镜像。
QEMU_FLAGS := -M q35 -smp 2 -m 128M \
    -cdrom $(ISO) -boot d \
    -drive file=disk.img,if=none,id=disk0,format=raw \
    -device ide-hd,drive=disk0,bus=ide.0

# 调试模式下额外使用的日志参数。
# 这些参数有助于输出中断、MMU 和 guest 错误信息，方便定位问题。
QEMU_DBG_FLAGS := -no-reboot -serial file:serial0.log -d int,guest_errors,mmu -D qemu.log

# 将编译和链接参数导出给子 Makefile 使用。
export INCDIR
export CFLAGS
export LDFLAGS

# 声明伪目标，避免与实际文件名冲突。
.PHONY: build iso qemu qemudbg qemugdb clean rebuild builddisk cleandisk cleanall format check-deps

## help
help:
	python3 tools/make_help.py

## 检查软件依赖
check-deps:
	python3 tools/check-deps.py

## 编译整个内核及其依赖模块，并生成最终可启动镜像。
build: check-deps
	$(MAKE) -C src/lib built-in.o
	$(MAKE) -C src/asm built-in.o
	$(MAKE) -C src/fs built-in.o
	$(MAKE) -C src/mm built-in.o
	$(MAKE) -C src/user init
	$(MAKE) -C src built-in.o
	$(LD) $(LDFLAGS) -T linker.ld -o vmrayos $(BUILT-IN)
	find src -name "*.o" ! -name "built-in.o" -type f -exec rm -f {} +

## 创建一个 FAT32 格式的磁盘镜像，并将用户态初始化程序复制进去。
builddisk:
	dd if=/dev/zero of=disk.img bs=1M count=128
	mkfs.fat -F 32 disk.img
	sudo mount disk.img /mnt
	sudo cp src/user/init /mnt
	sudo umount /mnt

## 删除构建生成的磁盘镜像。
cleandisk:
	$(RM) disk.img

## 生成可引导的 GRUB ISO 镜像。
iso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o $(ISO) iso/

## 启动 QEMU 运行系统镜像。
qemu: iso builddisk
	$(QEMU) $(QEMU_FLAGS)

## 启动 QEMU 并输出详细调试日志。
qemudbg: iso builddisk
	$(QEMU) $(QEMU_FLAGS) $(QEMU_DBG_FLAGS)

## 启动 QEMU 并等待 GDB 连接。
qemugdb: iso builddisk
	$(QEMU) $(QEMU_FLAGS) $(QEMU_DBG_FLAGS) -S -s

## 清理编译产物、日志和生成的 ISO 文件。
clean:
	find . -type f \( -name "*.o" -o -name "*.log" -o -name "*.iso" \) -exec rm -f {} +
	$(RM) *.iso vmrayos iso/boot/vmrayos *.log

## 清理所有构建产物，并进一步清理用户态构建结果。
cleanall: clean cleandisk
	$(MAKE) -C src/user clean

## 从干净状态重新构建整个项目。
rebuild: cleanall build

## 进行所有C/C++源码的格式化
format:
	python3 tools/format.py

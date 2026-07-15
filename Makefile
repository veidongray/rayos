# =============================================================================
# RayOS Build Script (Parallel-Safe)
# =============================================================================

SHELL       := /bin/bash
.SHELLFLAGS := -euo pipefail -c

# -----------------------------------------------------------------------------
# Global Configuration
# -----------------------------------------------------------------------------
DISK_SIZE_MB := 128
MEM_SIZE     := 128M
CPU_CORES    := 2
ISO_FILE     := rayos.iso
QEMU         := qemu-system-x86_64

# -----------------------------------------------------------------------------
# Kernel Object Files
# -----------------------------------------------------------------------------
BUILTIN_OBJS := src/asm/built-in.o \
                src/built-in.o     \
                src/fs/built-in.o  \
                src/lib/built-in.o \
                src/mm/built-in.o

# -----------------------------------------------------------------------------
# Include Paths
# -----------------------------------------------------------------------------
INCDIR := -I$(CURDIR)/inc -I$(CURDIR)/inc/lib -I$(CURDIR)/inc/user

# -----------------------------------------------------------------------------
# Compiler Flags
# -m64                    Generate code for the x86-64 (64-bit) architecture.
# -fno-pic                Disable position-independent code generation; not
#                         needed and potentially unsafe in freestanding kernels.
# -ffreestanding          Assert that the target environment lacks a standard
#                         library; compiler will not assume hosted runtime.
# -fno-builtin            Disable implicit built-in versions of standard
#                         functions to avoid references to unavailable symbols.
# -fno-stack-protector    Turn off stack canary insertion, which requires
#                         runtime support absent in bare-metal environments.
# -mno-red-zone           Prohibit use of the 128-byte red zone below RSP so
#                         interrupt/exception handlers cannot corrupt local data.
# -mno-sse                Disable SSE instruction emission.
# -mno-sse2               Disable SSE2 instruction emission.
# -mno-mmx                Disable MMX instruction emission.
# -mno-80387              Disable x87 FPU instruction emission.
#                         (The four flags above prevent any SIMD/FP usage,
#                          eliminating the need to save/restore FPU state.)
# -Wall                   Enable all commonly useful warning diagnostics.
# -Wextra                 Enable additional warnings beyond those from -Wall.
# -g                      Embed DWARF debugging information for GDB/debuggers.
# -fno-omit-frame-pointer Preserve the frame pointer register for reliable
#                         stack unwinding during debugging and profiling.
# -fno-asynchronous-unwind-tables
#                         Suppress .eh_frame generation to reduce binary size;
#                         exception handling is unused in the kernel.
# -nostdlib               Do not link against host startup files or standard
#                         libraries, ensuring zero host runtime dependencies.
# -std=gnu99              Select the GNU dialect of ISO C99 as the language
#                         standard, providing C99 semantics plus GNU extensions.
# -mcmodel=large          Use the large code model, allowing code and data to
#                         reside anywhere in the 64-bit address space.
# -MMD                    Emit a .d dependency file listing included headers,
#                         excluding system headers, for incremental rebuilds.
# -MP                     Add phony targets for each header in the .d file so
#                         deleting a header does not break make with an error.
# $(INCDIR)               Project-specific include search paths defined above.
# -----------------------------------------------------------------------------
CFLAGS := -m64 -fno-pic                      \
    -ffreestanding                           \
    -fno-builtin                             \
    -fno-stack-protector                     \
    -mno-red-zone                            \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387   \
    -Wall -Wextra                            \
    -g -fno-omit-frame-pointer               \
    -fno-asynchronous-unwind-tables          \
    -nostdlib                                \
    -std=gnu99                               \
    -mcmodel=large                           \
    -MMD -MP                                 \
    $(INCDIR)

LDFLAGS := -m elf_x86_64

# -----------------------------------------------------------------------------
# QEMU Options
# -----------------------------------------------------------------------------
QEMU_OPTS     := -M q35 -smp $(CPU_CORES) -m $(MEM_SIZE) \
                 -cdrom $(ISO_FILE) -boot d              \
                 -drive file=disk.img,if=none,id=disk0,format=raw \
                 -device ide-hd,drive=disk0,bus=ide.0

QEMU_DBG_OPTS := -no-reboot -serial file:serial0.log \
                 -d int,guest_errors,mmu -D qemu.log

export INCDIR CFLAGS LDFLAGS

# -----------------------------------------------------------------------------
# Automatic Dependency Tracking
# -----------------------------------------------------------------------------
DEPS := $(shell find src -name "*.d" 2>/dev/null)
-include $(DEPS)

# -----------------------------------------------------------------------------
# Phony Targets
# -----------------------------------------------------------------------------
.PHONY: help check-deps format \
		build iso disk \
		qemu qemudbg qemugdb \
        clean rebuild distclean \
        build-lib build-asm build-fs \
		build-mm build-user build-src

# =============================================================================
# Targets
# =============================================================================

## help: Show help message
help:
	python3 tools/make_help.py

## check-deps: Check software dependencies
check-deps:
	python3 tools/check-deps.py

# -----------------------------------------------------------------------------
# Parallel-safe sub-module targets
# Each module is an independent phony target so make can schedule them in
# parallel when invoked with -j. Inter-module ordering constraints are
# expressed via prerequisites rather than sequential recipe lines.
# -----------------------------------------------------------------------------

build-lib: check-deps
	@$(MAKE) --no-print-directory -C src/lib built-in.o

build-asm: check-deps
	@$(MAKE) --no-print-directory -C src/asm built-in.o

build-fs: check-deps
	@$(MAKE) --no-print-directory -C src/fs built-in.o

build-mm: check-deps
	@$(MAKE) --no-print-directory -C src/mm built-in.o

build-user: check-deps
	@$(MAKE) --no-print-directory -C src/user init

# src/built-in.o depends on all other modules being complete before linking.
# If additional inter-module header dependencies exist, add them here, e.g.:
#   build-fs: build-lib
build-src: build-lib build-asm build-fs build-mm build-user
	@$(MAKE) --no-print-directory -C src built-in.o

## build: Compile kernel and generate executable (parallel-safe)
build: build-src
	@echo "[LINK] vmrayos"
	@for obj in $(BUILTIN_OBJS); do \
		if [ ! -f "$$obj" ]; then \
			echo "ERROR: Missing object file: $$obj" >&2; exit 1; \
		fi; \
	done
	$(LD) $(LDFLAGS) -T linker.ld -o vmrayos $(BUILTIN_OBJS)

## disk: Create FAT32 disk image (incremental)
disk: check-deps disk.img
disk.img: src/user/init
	@echo "[DISK] Creating $@ ($(DISK_SIZE_MB)MB FAT32)"
	dd if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE_MB) status=none
	mkfs.fat -F 32 $@ >/dev/null
	mcopy -i $@ $< ::/init

## iso: Generate bootable GRUB ISO
iso: build
	@echo "[ISO] Generating $(ISO_FILE)"
	cp vmrayos iso/boot/
	grub-mkrescue -o $(ISO_FILE) iso/ 2>/dev/null

## qemu: Run system in QEMU
qemu: iso disk
	$(QEMU) $(QEMU_OPTS)

## qemudbg: Run QEMU with debug logging
qemudbg: iso disk
	$(QEMU) $(QEMU_OPTS) $(QEMU_DBG_OPTS)

## qemugdb: Run QEMU and wait for GDB connection
qemugdb: iso disk
	$(QEMU) $(QEMU_OPTS) $(QEMU_DBG_OPTS) -S -s

## clean: Remove build artifacts except disk image
clean:
	find . -type f \( -name "*.o" -o -name "*.d" -o -name "*.log" \) -delete
	$(RM) $(ISO_FILE) vmrayos iso/boot/vmrayos

## distclean: Remove all build artifacts including user programs and disk
distclean: clean
	$(MAKE) -C src/user clean
	$(RM) disk.img

## rebuild: Clean and rebuild from scratch
rebuild: distclean build

## format: Format all C/C++ source files
format:
	python3 tools/format.py
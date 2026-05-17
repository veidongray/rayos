BUILT-IN = src/libc/built-in.o src/asm/built-in.o src/built-in.o src/tools/built-in.o

INCDIR="$(CURDIR)/inc"
CFLAGS = -m64 -fno-pic                      \
    -ffreestanding                          \
    -fno-builtin                            \
    -fno-stack-protector                    \
    -mno-red-zone                           \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387  \
    -Wall -Wextra                           \
    -g -fno-omit-frame-pointer              \
    -fno-asynchronous-unwind-tables         \
    -std=gnu99                              \
	-mcmodel=kernel							\
    -I $(INCDIR)

LDFLAGS = -m elf_x86_64

export INCDIR
export CFLAGS
export LDFLAGS

build:
	$(MAKE) -C src/asm built-in.o
	$(MAKE) -C src built-in.o
	$(MAKE) -C src/libc built-in.o
	$(MAKE) -C src/tools built-in.o
	$(LD) $(LDFLAGS) -T linker.ld -o vmrayos $(BUILT-IN)

iso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o rayos.iso iso/

qemu: iso
	qemu-system-x86_64 -smp 2 -m 128M -no-reboot -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

qemu-dbg: iso
	qemu-system-x86_64 -smp 2 -m 128M -S -s -no-reboot -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

clean:
	$(RM) *.iso vmrayos iso/boot/vmrayos src/*.o src/libc/*.o src/asm/*.o src/tools/*.o *.log

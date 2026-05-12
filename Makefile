BUILT-IN = src/libc/built-in.o src/asm/built-in.o src/built-in.o src/tools/built-in.o

INCDIR="$(CURDIR)/inc"
CFLAGS = -m32 -fno-pic                      \
    -ffreestanding                          \
    -fno-builtin                            \
    -fno-stack-protector                    \
    -mno-red-zone                           \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387  \
    -Wall -Wextra                           \
    -g -fno-omit-frame-pointer              \
    -fno-asynchronous-unwind-tables         \
    -std=gnu99                              \
    -I $(INCDIR)

export INCDIR
export CFLAGS

build:
	$(MAKE) -C src/asm built-in.o
	$(MAKE) -C src built-in.o
	$(MAKE) -C src/libc built-in.o
	$(MAKE) -C src/tools built-in.o
	$(LD) -m elf_i386 -T linker.ld -o vmrayos $(BUILT-IN)

iso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o rayos.iso iso/

qemu: iso
	qemu-system-i386 -m 512M -no-reboot -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

qemu-dbg: iso
	qemu-system-i386 -m 512M -S -s -no-reboot -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

clean:
	$(RM) *.iso vmrayos iso/boot/vmrayos src/*.o src/libc/*.o src/asm/*.o src/tools/*.o *.log

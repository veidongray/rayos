BUILT-IN = src/asm/built-in.o src/built-in.o src/lib/printf/built-in.o src/lib/string/built-in.o

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
	-nostdlib 								\
    -std=gnu99                              \
	-mcmodel=large							\
    -I $(INCDIR)

LDFLAGS = -m elf_x86_64

export INCDIR
export CFLAGS
export LDFLAGS

.PHONY: build iso qemu qemu-dbg clean rebuild

build:
	$(MAKE) -C src/lib/printf built-in.o
	$(MAKE) -C src/lib/string built-in.o
	$(MAKE) -C src/asm built-in.o
	$(MAKE) -C src built-in.o
	$(LD) $(LDFLAGS) -T linker.ld -o vmrayos $(BUILT-IN)
	find src -name "*.o" ! -name "built-in.o" -type f -exec rm -f {} +

iso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o rayos.iso iso/

qemu: iso
	qemu-system-x86_64 -smp 2 -m 128M -no-reboot -serial file:serial0.log -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

qemu-dbg: iso
	qemu-system-x86_64 -smp 2 -m 128M -S -s -no-reboot -serial file:serial0.log -d int,guest_errors,mmu -D qemu.log -cdrom rayos.iso

clean:
	find . -name "*.o" -type f -exec rm -f {} +
	$(RM) *.iso vmrayos iso/boot/vmrayos *.log

rebuild: clean build

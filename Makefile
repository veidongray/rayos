BUILT-IN = src/asm/built-in.o src/built-in.o src/fs/built-in.o src/lib/printf/built-in.o src/lib/string/built-in.o

INCDIR = -I $(CURDIR)/inc -I $(CURDIR)/inc/lib/string -I $(CURDIR)/inc/lib/printf
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

LDFLAGS = -m elf_x86_64

ISO := rayos.iso
QEMU := qemu-system-x86_64
QEMU_FLAGS := -M q35 -smp 2 -m 128M \
    -cdrom $(ISO) -boot d \
    -drive file=disk.img,if=none,id=disk0,format=raw \
    -device ide-hd,drive=disk0,bus=ide.0
QEMU_DBG_FLAGS := -no-reboot -serial file:serial0.log -d int,guest_errors,mmu -D qemu.log

export INCDIR
export CFLAGS
export LDFLAGS

.PHONY: build iso qemu qemudbg qemugdb clean rebuild builddisk cleandisk cleanall

build:
	$(MAKE) -C src/lib/printf built-in.o
	$(MAKE) -C src/lib/string built-in.o
	$(MAKE) -C src/asm built-in.o
	$(MAKE) -C src/fs built-in.o
	$(MAKE) -C src/user init
	$(MAKE) -C src built-in.o
	$(LD) $(LDFLAGS) -T linker.ld -o vmrayos $(BUILT-IN)
	find src -name "*.o" ! -name "built-in.o" -type f -exec rm -f {} +

builddisk:
	dd if=/dev/zero of=disk.img bs=1M count=128
	mkfs.fat -F 32 disk.img
	sudo mount disk.img /mnt
	sudo cp src/user/init /mnt
	sudo umount /mnt

cleandisk:
	$(RM) disk.img

iso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o $(ISO) iso/

qemu: iso builddisk
	$(QEMU) $(QEMU_FLAGS)

qemudbg: iso builddisk
	$(QEMU) $(QEMU_FLAGS) $(QEMU_DBG_FLAGS)

qemugdb: iso builddisk
	$(QEMU) $(QEMU_FLAGS) $(QEMU_DBG_FLAGS) -S -s

clean:
	find . -type f \( -name "*.o" -o -name "*.log" -o -name "*.iso" \) -exec rm -f {} +
	$(RM) *.iso vmrayos iso/boot/vmrayos *.log

cleanall: clean cleandisk
	$(MAKE) -C src/user clean

rebuild: cleanall build

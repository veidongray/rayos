BUILT-IN = src/built-in.o src/libc/built-in.o

build:
	$(MAKE) INCDIR="$(CURDIR)/inc" -C src built-in.o
	$(MAKE) INCDIR="$(CURDIR)/inc" -C src/libc built-in.o
	$(LD) -m elf_i386 -T linker.ld -o vmrayos $(BUILT-IN)

mkiso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o rayos.iso iso/

qemu: mkiso
	qemu-system-i386 -m 128M -cdrom rayos.iso

qemu-dbg: mkiso
	qemu-system-i386 -m 128M -S -gdb tcp::1234 -cdrom rayos.iso

clean:
	$(RM) *.iso vmrayos iso/boot/vmrayos src/*.o src/libc/*.o

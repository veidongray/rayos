build:
	$(MAKE) -C src built-in.o
	ld -m elf_i386 -T linker.ld -o vmrayos src/built-in.o

mkiso: build
	cp vmrayos iso/boot/
	grub-mkrescue -o rayos.iso iso/

qemu: mkiso
	qemu-system-i386 -cdrom rayos.iso

clean:
	rm -rf *.iso vmrayos iso/boot/vmrayos src/*.o

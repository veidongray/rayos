.PHONY : boot.bin
boot.bin:
	gcc -m32 -fno-pie -c boot.S
	gcc -m32 -fno-pie -c bootmain.c
	ld boot.o bootmain.o -T boot.ld -o boot.elf
	objcopy -S -O binary boot.elf boot.bin
	objdump -D boot.elf > boot.dis

.PHONY : kernel.bin
kernel.bin:
	gcc -m32 -fno-pie -c entry.S
	gcc -m32 -fno-pie -c print.S
	gcc -m32 -fno-pie -c cursor.S
	gcc -m32 -fno-pie -c 8259A.S
	gcc -m32 -fno-pie -c keyboard.S
	gcc -m32 -fno-pie -c main.c
	ld entry.o print.o cursor.o 8259A.o keyboard.o main.o -N -T kernel.ld -o kernel.elf
	objcopy -S -O binary kernel.elf kernel.bin
	objdump -D kernel.elf > kernel.dis

.PHONY : qemu
qemu: boot.bin kernel.bin
	qemu-img create kernel.img 4M
	dd if=boot.bin of=kernel.img bs=512 seek=0 conv=notrunc
	dd if=kernel.bin of=kernel.img bs=512 seek=1 conv=notrunc
	qemu-system-i386 -drive file=kernel.img,format=raw -m 4

.PHONY : clean
clean:
	rm *.bin *.o *.elf *.img *.dis
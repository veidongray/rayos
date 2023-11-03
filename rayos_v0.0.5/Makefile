.PHONY : boot.bin
boot.bin:
	gcc -m32 -fno-pie -no-pie -fno-pic -c bootasm.S
	gcc -m32 -fno-pie -no-pie -fno-pic -c bootmain.c
	ld bootasm.o bootmain.o -T bootload.ld -o bootload.elf
	objcopy -S -O binary bootload.elf bootload.bin
	objdump -D bootload.elf > bootload.dis

.PHONY : kernel.bin
kernel.bin:
	gcc -m32 -fno-pie -no-pie -fno-pic -c entry.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c print.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c cursor.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c seg.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c int.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c 8259a.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c intasm.S
	gcc -m32 -fno-pie -no-pie -fno-pic -c keyboard.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c timer.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c task0.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c task1.c
	gcc -m32 -fno-pie -no-pie -fno-pic -c sched.c
	ld entry.o print.o cursor.o seg.o int.o 8259a.o intasm.o keyboard.o timer.o task0.o task1.o sched.o -T kernel.ld -o kernel.elf
	objcopy -S -O binary kernel.elf kernel.bin
	objdump -D kernel.elf > kernel.dis

.PHONY : qemu
qemu: boot.bin kernel.bin
	qemu-img create kernel.img 4M
	dd if=bootload.bin of=kernel.img bs=512 seek=0 conv=notrunc
	dd if=kernel.bin of=kernel.img bs=512 seek=1 conv=notrunc
	qemu-system-i386 -drive file=kernel.img,format=raw -m 4

.PHONY : clean
clean:
	rm *.o *.bin *.elf *.dis *.img
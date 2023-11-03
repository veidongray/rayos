CC 			:= gcc
LD 			:= ld
OBJCOPY 	:= objcopy
OBJDUMP 	:= objdump
CFLAGS      := -static -m32 -fno-pie -no-pie

BOOTOBJS    := bootloader.o
KERNELOBJS  :=  entry.o\
			    main.o\
				io.o

%.o:%.S
	$(CC) $(CFLAGS) -c $^

%.o:%.c
	$(CC) $(CFLAGS) -c $^

.PHONY : run
run: build
	qemu-system-i386 -hda test/rayos.img -m 4

.PHONY : build
build: $(BOOTOBJS) $(KERNELOBJS)
#	$(CC) $(CFLAGS) -c bootloader.S
#	$(CC) $(CFLAGS) -c entry.S
#	$(CC) $(CFLAGS) -c main.c
####
	$(LD) $(BOOTOBJS) -N -T bootloader.ld -o bootloader.elf
	$(LD) $(KERNELOBJS) -N -T kernel.ld -o kernel.elf
####
	$(OBJCOPY) -S -O binary bootloader.elf bootloader.bin
	$(OBJCOPY) -S -O binary kernel.elf kernel.bin
	$(OBJDUMP) -D bootloader.elf > bootloader.dis
	$(OBJDUMP) -D kernel.elf > kernel.dis
####
	qemu-img create test/rayos.img 8M
	dd if=bootloader.bin of=test/rayos.img bs=512 seek=0 conv=notrunc
	dd if=kernel.bin of=test/rayos.img bs=512 seek=1 conv=notrunc

.PHONY : clean
clean:
	rm test/*.img *.bin *.elf *.dis *.o
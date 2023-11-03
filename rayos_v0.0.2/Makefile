## (QEMU_MEMSIZE and QEMU_IMGSIZE) >= 16
QEMU_MEMSIZE := 16
QEMU_IMGSIZE := 16
INCDIR := include/
KERDIR := kernel/
OUTDIR := build/
BOOTDIR := boot/
BOBJS := bootloader.o
KOBJS := entry.o\
		 main.o\
		 print.o\
		 cursor.o

%.o:%.S
	gcc -I $(INCDIR) -m32 -c $^ -o $(OUTDIR)$(notdir $@)

%.o:%.c
	gcc -I $(INCDIR) -m32 -c $^ -o $(OUTDIR)$(notdir $@)

.PHONY : build
build: $(addprefix $(BOOTDIR), $(BOBJS)) $(addprefix $(KERDIR), $(KOBJS))
	ld $(addprefix $(OUTDIR), $(BOBJS)) -N -T $(BOOTDIR)bootloader.ld -o $(OUTDIR)bootloader.elf
	ld $(addprefix $(OUTDIR), $(KOBJS)) -N -T $(KERDIR)kernel.ld -o $(OUTDIR)rayos.elf
	objcopy -S -O binary $(OUTDIR)bootloader.elf $(OUTDIR)bootloader.bin
	objcopy -S -O binary $(OUTDIR)rayos.elf $(OUTDIR)rayos.bin
	objdump -D $(OUTDIR)rayos.elf > $(OUTDIR)rayos.dis

.PHONY : qemu-i386
qemu-i386: build
	qemu-img create $(OUTDIR)rayos.img $(QEMU_IMGSIZE)M
	dd if=$(OUTDIR)bootloader.bin of=$(OUTDIR)rayos.img bs=512 seek=0 conv=notrunc
	dd if=$(OUTDIR)rayos.bin of=$(OUTDIR)rayos.img bs=512 seek=1 conv=notrunc
	qemu-system-i386 -hda $(OUTDIR)rayos.img -m $(QEMU_MEMSIZE)

.PHONY : clean
clean:
	rm $(OUTDIR)*.o $(OUTDIR)*.elf $(OUTDIR)*.bin $(OUTDIR)*.img $(OUTDIR)*.dis
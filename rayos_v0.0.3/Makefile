## (QEMU_MEMSIZE and QEMU_IMGSIZE) >= 4
QEMU_MEMSIZE := 4
QEMU_IMGSIZE := 4
CC := gcc
LD := ld
CCFLAGS := -m32 -fno-pic
LDFLAGS := -N --no-warn-rwx-segments -z execstack
INCDIR := include/
KERDIR := kernel/
OUTDIR := build/
BOOTDIR := boot/
BOBJS := bootloader.o
KOBJS := entry.o\
		 main.o\
		 cursor.o\
		 print.o

%.o:%.S
	$(CC) -I $(INCDIR) $(CCFLAGS) -c $^ -o $(OUTDIR)$(notdir $@)

%.o:%.c
	$(CC) -I $(INCDIR) $(CCFLAGS) -c $^ -o $(OUTDIR)$(notdir $@)

.PHONY : build
build: $(addprefix $(BOOTDIR), $(BOBJS)) $(addprefix $(KERDIR), $(KOBJS))
	$(LD) $(addprefix $(OUTDIR), $(BOBJS)) $(LDFLAGS) -T $(BOOTDIR)bootloader.ld -o $(OUTDIR)bootloader.elf
	$(LD) $(addprefix $(OUTDIR), $(KOBJS)) $(LDFLAGS) -T $(KERDIR)kernel.ld -o $(OUTDIR)rayos.elf
	objcopy -S -O binary $(OUTDIR)bootloader.elf $(OUTDIR)bootloader.bin
	objcopy -S -O binary $(OUTDIR)rayos.elf $(OUTDIR)rayos.bin
	objdump -D $(OUTDIR)rayos.elf > $(OUTDIR)rayos.dis

.PHONY : qemu-i386
qemu-i386: build
	qemu-img create $(OUTDIR)rayos.img $(QEMU_IMGSIZE)M
	dd if=$(OUTDIR)bootloader.bin of=$(OUTDIR)rayos.img bs=512 seek=0 conv=notrunc
	dd if=$(OUTDIR)rayos.bin of=$(OUTDIR)rayos.img bs=512 seek=1 conv=notrunc
	qemu-system-i386 -drive file=$(OUTDIR)rayos.img,format=raw -m $(QEMU_MEMSIZE)

.PHONY : clean
clean:
	rm $(OUTDIR)*.o $(OUTDIR)*.elf $(OUTDIR)*.bin $(OUTDIR)*.img $(OUTDIR)*.dis
INCDIR = ../inc
OBJS =  head.o          \
        main.o          \
        print.o         \
        multiboot2.o    \
        gdt.o           \
        gdt_asm.o       \
        idt.o           \
        idt_asm.o       \
        pic_8259.o      \
        io.o            \
        paging.o        \
        paging_asm.o

CFLAGS = -m32                               \
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

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

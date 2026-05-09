OBJS =  main.o          \
        tty.o         \
        multiboot2.o    \
        gdt.o           \
        idt.o           \
        pic_8259.o      \
        io.o            \
        paging.o        \
        task.o          \
        mm.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

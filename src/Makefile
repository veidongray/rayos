OBJS =  main.o          \
        print.o         \
        multiboot2.o    \
        gdt.o           \
        idt.o           \
        pic_8259.o      \
        io.o            \
        paging.o        \
        kheap.o         \
        task.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

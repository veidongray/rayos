OBJS =  main.o          \
        tty.o           \
        multiboot2.o    \
        gdt.o           \
        idt.o           \
        pic_8259.o      \
        io.o            \
        paging.o        \
        task.o          \
        mm.o            \
        syscall.o       \
        apic.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o built-in.o $(OBJS)

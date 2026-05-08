# head.o must be first
OBJS = head.o \
    switch_to.o \
    gdt.o \
    idt.o \
    paging.o

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

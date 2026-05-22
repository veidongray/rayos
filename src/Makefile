OBJS =  main.o multiboot2.o gdt.o page.o mm.o pic.o int.o lapic.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o built-in.o $(OBJS)

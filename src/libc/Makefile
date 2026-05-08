OBJS =  string.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

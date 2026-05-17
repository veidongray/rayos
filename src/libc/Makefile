OBJS = string.o stdio.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o built-in.o $(OBJS)

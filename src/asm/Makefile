# head.o must be first
OBJS = head.o

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o built-in.o $(OBJS)

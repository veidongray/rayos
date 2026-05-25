OBJS := $(patsubst %.S,%.o,$(wildcard *.S))

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o $@ $^
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean built-in.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o $@ $^

clean:
	$(RM) -rf *.o
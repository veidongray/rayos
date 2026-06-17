OBJS := $(patsubst %.S,%.o,$(wildcard *.S))

.PHONY: clean built-in.o

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) $(LDFLAGS) -r -o $@ $^

clean:
	$(RM) -rf *.o
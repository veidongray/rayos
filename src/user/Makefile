OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean init

%.o: %.c
	$(CC) $(CFLAGS) -static -c $< -o $@

init: $(OBJS)
	$(LD) $(LDFLAGS) -e main -o $@ $^

clean:
	$(RM) init
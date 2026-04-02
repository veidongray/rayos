CFLAGS = -m32 \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -mno-red-zone \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387 \
    -Wall -Wextra -Werror \
    -g \
    -std=gnu99 \
    -nostdlib

OBJS = head.o main.o

%.o: %.S
	gcc $(CFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	ld -m elf_i386 -r -o built-in.o $(OBJS)

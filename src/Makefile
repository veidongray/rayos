CFLAGS = -m32 \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -mno-red-zone \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387 \
    -Wall -Wextra \
    -g -fno-omit-frame-pointer \
    -fno-asynchronous-unwind-tables \
    -std=gnu99 \
    -I ../inc

OBJS = head.o main.o isr_stubs.o isr_handlers.o print.o pic.o io.o gdt.o idt.o systicks.o

%.o: %.S
	gcc $(CFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	ld -m elf_i386 -r -o built-in.o $(OBJS)

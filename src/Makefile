INCDIR = ../inc
OBJS =  head.o          \
        main.o          \
        isr_stubs.o     \
        isr_handlers.o  \
        print.o         \
        pic.o           \
        io.o            \
        gdt.o           \
        idt.o           \
        systicks.o      \
        page.o

CFLAGS = -m32                               \
    -ffreestanding                          \
    -fno-builtin                            \
    -fno-stack-protector                    \
    -mno-red-zone                           \
    -mno-sse -mno-sse2 -mno-mmx -mno-80387  \
    -Wall -Wextra -Werror                   \
    -g -fno-omit-frame-pointer              \
    -fno-asynchronous-unwind-tables         \
    -std=gnu99                              \
    -I $(INCDIR)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

built-in.o: $(OBJS)
	$(LD) -m elf_i386 -r -o built-in.o $(OBJS)

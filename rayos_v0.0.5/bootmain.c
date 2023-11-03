#include "x86.h"
#define SECTSIZE 512

static uchar *kerneladdr = (uchar *)0x100000;
static int readsectors = 0x80;

void (*entry)(void);
void waitdisk(void);
// Read a single sector at offset into dst.
void readsect(void *, uint);

void bootmain(void)
{
    int i;
    entry = ((void (*)(void))kerneladdr);

    for (i = 0; i < readsectors; i++)
    {
        readsect(kerneladdr + (i * 0x200), i + 1);
    }
    entry();
}

void waitdisk(void)
{
    // Wait for disk ready.
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}

// Read a single sector at offset into dst.
void readsect(void *dst, uint offset)
{
    // Issue command.
    waitdisk();
    outb(0x1F2, 1); // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20); // cmd 0x20 - read sectors

    // Read data.
    waitdisk();
    insl(0x1F0, dst, SECTSIZE / 4);
}
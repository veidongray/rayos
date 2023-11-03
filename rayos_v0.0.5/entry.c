void entry(void)
{
    vga_refresh();
    puts("[OK] Loading kernel to 0x100000\n");
    init_gdt();
    puts("[OK] Initialize global descriptions to 0x10000\n");
    init_int();
    puts("[OK] Initialize interrupt descriptions to 0x20000\n");
    init_timer();
    puts("[OK] Initialize timer\n");
    main();
}

void main(void)
{
    schedule();
    while (1);
}
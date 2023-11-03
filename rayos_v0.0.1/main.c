static unsigned int cursor_position = 0;

int main(void)
{
    refresh_print();
    relocate_gdt();
    while (1);
    return 0;
}

void refresh_print(void)
{
    char ch = ' ';

    for (cursor_position = 0; cursor_position < 80 * 25; cursor_position++) {
        putc(ch, cursor_position << 1);
    }
    cursor_position = 0;
}

void relocate_gdt(void)
{
    return;
}
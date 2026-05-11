#include "tty.h"
#include "idt.h"
#include <stdint.h>
#include "libc/string.h"
#include "aligned.h"

ALIGN_ATTR static uint8_t *cgaptr = (uint8_t *)0xC00B8000;
ALIGN_ATTR static uint8_t cga_buffer[4000];
static uint8_t **get_cgaptr(void)
{
    return &cgaptr;
}

int cga_shift(uint8_t **ptr)
{
    int i;
    memset(cga_buffer, 0, 4000);
    if (*ptr >= (uint8_t *)(0xC00B8000 + 4000))
    {
        *ptr = (uint8_t *)0xC00B8000 + 4000 - 160;
        for (i = 0; i < 4000 - 160; ++i)
        {
            cga_buffer[i] = ((uint8_t *)0xC00B8000)[i + 160];
        }
        for (i = 4000 - 160; i < 4000; i += 2)
        {
            cga_buffer[i] = ' ';
            cga_buffer[i + 1] = 0x07; // Light grey on black
        }
        for (i = 0; i < 4000; ++i)
        {
            ((uint8_t *)0xC00B8000)[i] = cga_buffer[i];
        }
    }
    return 0;
}

char *itoa(int value, char *str, int base)
{
    char *rc;
    char *ptr;
    char *low;
    // Check for supported base.
    if (base < 2 || base > 36)
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if (value < 0 && base == 10)
    {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do
    {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while (value);
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while (low < ptr)
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

char *uitoa(uint32_t value, char *str, int base)
{
    char *ptr = str;
    char *low;

    if (base < 2 || base > 36)
    {
        *str = '\0';
        return str;
    }

    low = ptr;

    // Handle zero specially
    if (value == 0)
    {
        *ptr++ = '0';
    }
    else
    {
        // Convert
        while (value)
        {
            *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
            value /= base;
        }
    }

    *ptr-- = '\0';

    // Reverse
    while (low < ptr)
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }

    return str;
}

int cga_putc(const char ch)
{
    uint8_t **cga = get_cgaptr();
    if (ch >= 32)
    {
        **cga = ch;
        *cga += 2;
        cga_shift(cga);
    }
    else if (ch == '\n' || ch == '\r')
    {
        *cga += 160 - ((*cga - (uint8_t *)0xC00B8000) % 160);
        cga_shift(cga);
    }
    return ch;
}

int cga_puts(const char *str)
{
    int i;
    for (i = 0; i < 2000 && str[i] != '\0'; ++i)
    {
        cga_putc(str[i]);
    }
    return i;
}

int cga_printf(const char *format, ...)
{
    const char *p = format;
    char *args = (char *)(&format + 1);
    char str_buffer[32];
    int count = 0;

    while (*p)
    {
        if (*p != '%')
        {
            cga_putc(*p);
            p++;
            count++;
            continue;
        }

        p++;

        switch (*p)
        {
        case 'c':
        {
            char c = *(char *)args;
            args += sizeof(int);
            cga_putc(c);
            count++;
            break;
        }
        case 's':
        {
            char *str = *(char **)args;
            args += sizeof(char *);
            if (str == (char *)0)
                str = "(null)";
            int len = cga_puts(str);
            count += len;
            break;
        }
        case 'd':
        case 'i':
        {
            int val = *(int *)args;
            args += sizeof(int);
            itoa(val, str_buffer, 10);
            int len = cga_puts(str_buffer);
            count += len;
            break;
        }
        case 'u':
        {
            unsigned int val = *(unsigned int *)args;
            args += sizeof(unsigned int);
            uitoa(val, str_buffer, 10);
            int len = cga_puts(str_buffer);
            count += len;
            break;
        }
        case 'x':
        case 'X':
        {
            unsigned int val = *(unsigned int *)args;
            args += sizeof(unsigned int);
            uitoa(val, str_buffer, 16);
            if (*p == 'X')
            {
                for (int i = 0; str_buffer[i]; i++)
                {
                    if (str_buffer[i] >= 'a' && str_buffer[i] <= 'f')
                        str_buffer[i] = str_buffer[i] - 'a' + 'A';
                }
            }
            int len = cga_puts(str_buffer);
            count += len;
            break;
        }
        case '%':
        {
            cga_putc('%');
            count++;
            break;
        }
        default:
        {
            cga_putc('%');
            cga_putc(*p);
            count += 2;
            break;
        }
        }
        p++;
    }

    return count;
}

int tty_init(void)
{
    // Do nothing...
    return 0;
}
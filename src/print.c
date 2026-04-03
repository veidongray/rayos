#include "print.h"
#include <stdint.h>

static uint8_t *cgaptr = (uint8_t *)0xB8000;
static uint8_t **get_cgaptr(void) { return &cgaptr; }

char *itoa( int value, char * str, int base )
{
    char * rc;
    char * ptr;
    char * low;
    // Check for supported base.
    if ( base < 2 || base > 36 )
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if ( value < 0 && base == 10 )
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
    } while ( value );
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while ( low < ptr )
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

int cga_putc(const char ch)
{
    uint8_t **cga = get_cgaptr();
    if (ch >= 32) {
        **cga = ch;
        *cga += 2;
        if (*cga >= (uint8_t *)(0xB8000 + 4000)) {
            *cga = (uint8_t *)0xB8000;
        }
    } else if (ch == '\n' || ch == '\r') {
        *cga += 160 - ((*cga - (uint8_t *)0xB8000) % 160);
    }
    return ch;
}

int cga_puts(const char *str)
{
    int i;
    for (i = 0; i < 2000 && str[i] != '\0'; ++i) {
        cga_putc(str[i]);
    }
    return i;
}

int cga_printf(const char *format, ...)
{
    const char *p = format;
    char *args = (char *)(&format + 1);
    char buffer[32];
    int count = 0;

    while (*p) {
        if (*p != '%') {
            cga_putc(*p);
            p++;
            count++;
            continue;
        }

        p++;

        switch (*p) {
            case 'c': {
                char c = *(char *)args;
                args += sizeof(int);
                cga_putc(c);
                count++;
                break;
            }
            case 's': {
                char *str = *(char **)args;
                args += sizeof(char *);
                if (str == (char *)0) str = "(null)";
                int len = cga_puts(str);
                count += len;
                break;
            }
            case 'd':
            case 'i': {
                int val = *(int *)args;
                args += sizeof(int);
                itoa(val, buffer, 10);
                int len = cga_puts(buffer);
                count += len;
                break;
            }
            case 'x':
            case 'X': {
                int val = *(int *)args;
                args += sizeof(int);
                itoa(val, buffer, 16);
                if (*p == 'X') {
                    for (int i = 0; buffer[i]; i++) {
                        if (buffer[i] >= 'a' && buffer[i] <= 'f')
                            buffer[i] = buffer[i] - 'a' + 'A';
                    }
                }
                int len = cga_puts(buffer);
                count += len;
                break;
            }
            case 'u': {
                unsigned int val = *(unsigned int *)args;
                args += sizeof(unsigned int);
                itoa((int)val, buffer, 10);
                int len = cga_puts(buffer);
                count += len;
                break;
            }
            case '%': {
                cga_putc('%');
                count++;
                break;
            }
            default: {
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
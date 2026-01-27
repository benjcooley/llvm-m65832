/* stdio.c - Standard I/O implementation */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/* Platform hooks - provided by emulator platform layer */
extern void uart_putc(int c);
extern int uart_getc(void);

int putchar(int c) {
    uart_putc(c);
    return c;
}

int puts(const char *s) {
    while (*s) {
        putchar(*s++);
    }
    putchar('\n');
    return 0;
}

int getchar(void) {
    return uart_getc();
}

char *gets(char *s) {
    char *p = s;
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        *p++ = c;
    }
    *p = '\0';
    return s;
}

/*
 * Minimal printf implementation
 * Supports: %d, %i, %u, %x, %X, %c, %s, %p, %%
 * Supports: width, left-justify (-), zero-pad (0)
 * Does NOT support: precision, floating point, long long
 */

static void print_char(char **buf, size_t *remaining, int c) {
    if (buf) {
        if (*remaining > 1) {
            **buf = c;
            (*buf)++;
            (*remaining)--;
        }
    } else {
        putchar(c);
    }
}

static void print_string(char **buf, size_t *remaining, const char *s, int width, int left) {
    int len = strlen(s);
    int pad = (width > len) ? (width - len) : 0;
    
    if (!left) {
        while (pad-- > 0) print_char(buf, remaining, ' ');
    }
    while (*s) {
        print_char(buf, remaining, *s++);
    }
    if (left) {
        while (pad-- > 0) print_char(buf, remaining, ' ');
    }
}

static void print_num(char **buf, size_t *remaining, unsigned long val, int base, 
                      int is_signed, int width, int zero_pad, int left, int upper) {
    char tmp[12];  /* Enough for 32-bit in any base */
    char *p = tmp + sizeof(tmp) - 1;
    int neg = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    
    *p = '\0';
    
    if (is_signed && (long)val < 0) {
        neg = 1;
        val = -(long)val;
    }
    
    if (val == 0) {
        *--p = '0';
    } else {
        while (val) {
            *--p = digits[val % base];
            val /= base;
        }
    }
    
    if (neg) *--p = '-';
    
    int len = (tmp + sizeof(tmp) - 1) - p;
    int pad = (width > len) ? (width - len) : 0;
    char pad_char = zero_pad ? '0' : ' ';
    
    if (!left) {
        /* If zero-padding with negative, put sign first */
        if (zero_pad && neg) {
            print_char(buf, remaining, '-');
            p++;  /* Skip the sign in the buffer */
            len--;
        }
        while (pad-- > 0) print_char(buf, remaining, pad_char);
    }
    while (*p) {
        print_char(buf, remaining, *p++);
    }
    if (left) {
        while (pad-- > 0) print_char(buf, remaining, ' ');
    }
}

static int do_printf(char *str, size_t size, const char *format, va_list ap) {
    char **buf = str ? &str : NULL;
    size_t remaining = str ? size : (size_t)-1;
    int count = 0;
    
    while (*format) {
        if (*format != '%') {
            print_char(buf, &remaining, *format++);
            count++;
            continue;
        }
        
        format++;  /* Skip '%' */
        
        /* Parse flags */
        int left = 0, zero_pad = 0;
        while (*format == '-' || *format == '0') {
            if (*format == '-') left = 1;
            if (*format == '0') zero_pad = 1;
            format++;
        }
        if (left) zero_pad = 0;  /* Left justify overrides zero-pad */
        
        /* Parse width */
        int width = 0;
        while (isdigit(*format)) {
            width = width * 10 + (*format++ - '0');
        }
        
        /* Parse length modifier (ignored for now) */
        if (*format == 'l') format++;
        
        /* Parse conversion specifier */
        switch (*format++) {
        case 'd':
        case 'i':
            print_num(buf, &remaining, va_arg(ap, int), 10, 1, width, zero_pad, left, 0);
            break;
        case 'u':
            print_num(buf, &remaining, va_arg(ap, unsigned int), 10, 0, width, zero_pad, left, 0);
            break;
        case 'x':
            print_num(buf, &remaining, va_arg(ap, unsigned int), 16, 0, width, zero_pad, left, 0);
            break;
        case 'X':
            print_num(buf, &remaining, va_arg(ap, unsigned int), 16, 0, width, zero_pad, left, 1);
            break;
        case 'p':
            print_string(buf, &remaining, "0x", 0, 0);
            print_num(buf, &remaining, (unsigned long)va_arg(ap, void *), 16, 0, 8, 1, 0, 0);
            break;
        case 'c':
            print_char(buf, &remaining, va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            print_string(buf, &remaining, s ? s : "(null)", width, left);
            break;
        }
        case '%':
            print_char(buf, &remaining, '%');
            break;
        default:
            /* Unknown specifier - print literally */
            print_char(buf, &remaining, '%');
            print_char(buf, &remaining, format[-1]);
            break;
        }
    }
    
    /* Null terminate if writing to buffer */
    if (str && size > 0) {
        *str = '\0';
    }
    
    return count;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = do_printf(NULL, 0, format, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *format, va_list ap) {
    return do_printf(NULL, 0, format, ap);
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = do_printf(str, (size_t)-1, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    return do_printf(str, (size_t)-1, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = do_printf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    return do_printf(str, size, format, ap);
}

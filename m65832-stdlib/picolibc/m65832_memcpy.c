/*
 * m65832_memcpy.c - Optimized memcpy/memmove for M65832
 *
 * Processes 32 bytes per loop via eight 32-bit load/store pairs.
 * The M65832 has LDQ/STQ for 64-bit transfers:
 *   LDQ dp: T:A = [D+dp], [D+dp+4]  (load 64-bit into T:A)
 *   STQ dp: [D+dp] = A, [D+dp+4] = T (store T:A as 64-bit)
 * but using them requires D/B register manipulation; the C compiler
 * will generate efficient ST/LD extended instructions automatically.
 */

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    /* Byte-copy until 4-byte aligned */
    while (n > 0 && ((uintptr_t)d & 3)) {
        *d++ = *s++;
        n--;
    }

    /* 32-byte blocks: 8 × 32-bit load/store pairs */
    if (!((uintptr_t)s & 3)) {
        /* Both pointers are 4-byte aligned */
        uint32_t *wd = (uint32_t *)d;
        const uint32_t *ws = (const uint32_t *)s;
        while (n >= 32) {
            wd[0] = ws[0]; wd[1] = ws[1];
            wd[2] = ws[2]; wd[3] = ws[3];
            wd[4] = ws[4]; wd[5] = ws[5];
            wd[6] = ws[6]; wd[7] = ws[7];
            wd += 8; ws += 8;
            n -= 32;
        }
        /* 4-byte tail blocks */
        while (n >= 4) {
            *wd++ = *ws++;
            n -= 4;
        }
        d = (unsigned char *)wd;
        s = (const unsigned char *)ws;
    }

    /* Byte tail */
    while (n > 0) {
        *d++ = *s++;
        n--;
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || n == 0) return dst;

    if (d < s || d >= s + n) {
        /* No overlap or dst before src: forward copy */
        return memcpy(dst, src, n);
    }

    /* Overlapping: copy backwards */
    d += n;
    s += n;
    while (n-- > 0)
        *--d = *--s;

    return dst;
}

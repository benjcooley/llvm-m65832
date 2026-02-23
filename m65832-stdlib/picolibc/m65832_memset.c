/*
 * m65832_memset.c - Optimized memset for M65832
 *
 * Processes 32 bytes per loop via eight 32-bit STA-indirect stores.
 * The M65832 compiler emits `ST $10,(Rn),Y` for each 32-bit word store.
 * With the hardware's extended ALU, each store is a single instruction.
 *
 * Future: use STQ (64-bit store, $02 $9E) for 8 bytes/instruction.
 *   STQ dp $oo : stores T:A to [D+oo] and [D+oo+4] in one instruction.
 */

#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int c, size_t n) {
    unsigned char *p = (unsigned char *)dst;
    unsigned char uc = (unsigned char)c;

    /* Expand to 32-bit word fill value */
    uint32_t v = (uint32_t)uc;
    v |= (v << 8);
    v |= (v << 16);

    /* Byte-fill until 4-byte aligned */
    while (n > 0 && ((uintptr_t)p & 3)) {
        *p++ = uc;
        n--;
    }

    /* 32-byte blocks: 8 × 32-bit stores per iteration */
    uint32_t *w = (uint32_t *)p;
    while (n >= 32) {
        w[0] = v; w[1] = v; w[2] = v; w[3] = v;
        w[4] = v; w[5] = v; w[6] = v; w[7] = v;
        w += 8;
        n -= 32;
    }

    /* 4-byte tail blocks */
    while (n >= 4) {
        *w++ = v;
        n -= 4;
    }

    /* Byte tail */
    p = (unsigned char *)w;
    while (n > 0) {
        *p++ = uc;
        n--;
    }

    return dst;
}

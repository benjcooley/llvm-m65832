/*
 * m65832_malloc.c - Simple freelist allocator for M65832 baremetal
 *
 * Uses a first-fit freelist with coalescing.  Much simpler than picolibc's
 * allocator and doesn't rely on BSS-initialized sbrk tracking variables.
 *
 * Block layout (each allocated or free block):
 *   [size | flags (4 bytes)] [payload ...]
 *   Flag bit 0: 0 = free, 1 = in use
 *   Size includes the 4-byte header.
 *   Minimum block = 8 bytes (4 header + 4 payload).
 */

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

/* Provided by linker script */
extern char _heap_start[];
extern char _heap_end[];

#define HDR_SIZE   4
#define MIN_BLOCK  8
#define ALIGN      8          /* align allocations to 8 bytes */
#define INUSE_BIT  1u
#define SIZE_MASK  (~(uint32_t)7)   /* low 3 bits are flags */

typedef struct block {
    uint32_t hdr;             /* size (aligned) | flags */
} block_t;

#define BLK_SIZE(b)   ((b)->hdr & SIZE_MASK)
#define BLK_INUSE(b)  ((b)->hdr & INUSE_BIT)
#define BLK_PAYLOAD(b) ((char*)(b) + HDR_SIZE)
#define NEXT_BLK(b)   ((block_t*)((char*)(b) + BLK_SIZE(b)))

static block_t *heap_top = 0;  /* One-past-end of all known blocks */
static block_t *first_blk = 0; /* First block in heap */

static void heap_init(void) {
    first_blk = (block_t *)_heap_start;
    first_blk->hdr = 0;       /* Sentinel: size=0, not yet grown */
    heap_top = first_blk;
}

/* Grow heap by at least `need` bytes; returns new free block or NULL */
static block_t *heap_grow(size_t need) {
    /* Round up to 64-byte granularity for efficiency */
    size_t grow = (need + 63) & ~63u;
    if ((char*)heap_top + grow > _heap_end)
        return NULL;

    block_t *b = heap_top;
    b->hdr = (uint32_t)grow;  /* free, size=grow */
    heap_top = (block_t*)((char*)b + grow);
    return b;
}

void *malloc(size_t n) {
    if (n == 0) n = 1;
    /* Total block size: header + payload, rounded up to ALIGN */
    size_t total = (HDR_SIZE + n + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    if (total < MIN_BLOCK) total = MIN_BLOCK;

    if (!first_blk) heap_init();

    /* First fit search */
    block_t *b = first_blk;
    while ((char*)b < (char*)heap_top) {
        if (!BLK_INUSE(b) && BLK_SIZE(b) >= total) {
            /* Split if remainder is large enough */
            size_t rem = BLK_SIZE(b) - total;
            if (rem >= MIN_BLOCK + HDR_SIZE) {
                block_t *split = (block_t*)((char*)b + total);
                split->hdr = (uint32_t)rem;  /* free */
                b->hdr = (uint32_t)total | INUSE_BIT;
            } else {
                b->hdr |= INUSE_BIT;
            }
            return BLK_PAYLOAD(b);
        }
        if (BLK_SIZE(b) == 0) break;  /* shouldn't happen */
        b = NEXT_BLK(b);
    }

    /* No free block found - grow heap */
    b = heap_grow(total);
    if (!b) {
        errno = 12; /* ENOMEM */
        return NULL;
    }
    /* Use this new block */
    size_t rem = BLK_SIZE(b) - total;
    if (rem >= MIN_BLOCK + HDR_SIZE) {
        block_t *split = (block_t*)((char*)b + total);
        split->hdr = (uint32_t)rem;
        b->hdr = (uint32_t)total | INUSE_BIT;
    } else {
        b->hdr |= INUSE_BIT;
    }
    return BLK_PAYLOAD(b);
}

void free(void *p) {
    if (!p) return;
    block_t *b = (block_t*)((char*)p - HDR_SIZE);
    if (!BLK_INUSE(b)) return;  /* already free / double-free */
    b->hdr &= ~INUSE_BIT;       /* mark free */

    /* Coalesce forward */
    block_t *nxt = NEXT_BLK(b);
    while ((char*)nxt < (char*)heap_top && !BLK_INUSE(nxt) && BLK_SIZE(nxt) > 0) {
        b->hdr = (BLK_SIZE(b) + BLK_SIZE(nxt));  /* free, combined */
        nxt = NEXT_BLK(b);
    }
}

void *calloc(size_t nmemb, size_t size) {
    size_t n = nmemb * size;
    void *p = malloc(n);
    if (p) {
        /* Use __builtin_memset so we don't need to call libc memset */
        __builtin_memset(p, 0, n);
    }
    return p;
}

void *realloc(void *p, size_t n) {
    if (!p) return malloc(n);
    if (n == 0) { free(p); return NULL; }

    block_t *b = (block_t*)((char*)p - HDR_SIZE);
    size_t old_payload = BLK_SIZE(b) - HDR_SIZE;

    /* Try to grow in-place by coalescing with next free block */
    size_t need = (HDR_SIZE + n + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    block_t *nxt = NEXT_BLK(b);
    if ((char*)nxt < (char*)heap_top && !BLK_INUSE(nxt)) {
        size_t combined = BLK_SIZE(b) + BLK_SIZE(nxt);
        if (combined >= need) {
            b->hdr = (uint32_t)combined | INUSE_BIT;
            return p;
        }
    }

    /* Allocate new block and copy */
    void *np = malloc(n);
    if (np) {
        size_t copy = (n < old_payload) ? n : old_payload;
        __builtin_memcpy(np, p, copy);
        free(p);
    }
    return np;
}

/* malloc_usable_size - needed by some picolibc internals */
size_t malloc_usable_size(void *p) {
    if (!p) return 0;
    block_t *b = (block_t*)((char*)p - HDR_SIZE);
    return BLK_SIZE(b) - HDR_SIZE;
}

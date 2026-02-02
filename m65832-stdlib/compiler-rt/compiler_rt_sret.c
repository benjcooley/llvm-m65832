/* M65832 Compiler Runtime Support Functions - SRET Version
 *
 * These functions use the sret (structure return) calling convention
 * that LLVM generates for the M65832 target when returning 64-bit values.
 *
 * Calling convention for 64-bit returns:
 *   - R0 contains a pointer to where the 64-bit result should be stored
 *   - R1:R2 contains the first 64-bit argument (low:high)
 *   - R3:R4 contains the second 64-bit argument (low:high)
 *   - The function writes the result to *R0 (low word) and *(R0+4) (high word)
 */

#include <stdint.h>
#include <limits.h>

typedef int32_t si_int;
typedef uint32_t su_int;
typedef int64_t di_int;
typedef uint64_t du_int;

typedef union {
    di_int all;
    struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        su_int low;
        si_int high;
#else
        si_int high;
        su_int low;
#endif
    } s;
} dwords;

typedef union {
    du_int all;
    struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        su_int low;
        su_int high;
#else
        su_int high;
        su_int low;
#endif
    } s;
} udwords;

/*
 * 32-bit multiply returning 64-bit result
 * Only uses 32-bit operations to avoid recursion
 */
static du_int mul32x32_64(su_int a, su_int b)
{
    /* Split into 16-bit halves to avoid overflow */
    su_int a_lo = a & 0xFFFF;
    su_int a_hi = a >> 16;
    su_int b_lo = b & 0xFFFF;
    su_int b_hi = b >> 16;
    
    /* Four 16x16->32 multiplies */
    su_int p0 = a_lo * b_lo;           /* Low * Low */
    su_int p1 = a_lo * b_hi;           /* Low * High */
    su_int p2 = a_hi * b_lo;           /* High * Low */
    su_int p3 = a_hi * b_hi;           /* High * High */
    
    /* Combine: result = p0 + (p1 + p2) << 16 + p3 << 32 */
    udwords result;
    result.s.low = p0;
    result.s.high = p3;
    
    /* Add middle terms with carry handling */
    su_int mid = p1 + p2;
    su_int mid_carry = (mid < p1) ? 1 : 0;  /* Detect overflow */
    
    /* Add low 16 bits of mid to upper 16 bits of low word */
    su_int add_lo = (mid & 0xFFFF) << 16;
    su_int new_low = result.s.low + add_lo;
    su_int carry = (new_low < result.s.low) ? 1 : 0;
    result.s.low = new_low;
    
    /* Add high 16 bits of mid and carries to high word */
    result.s.high += (mid >> 16) + carry + (mid_carry << 16);
    
    return result.all;
}

/*
 * 64-bit multiplication
 * Uses SRET: result written to the address in the first parameter
 */
void __muldi3(di_int *result, di_int a, di_int b)
{
    dwords x, y;
    udwords r;
    x.all = a;
    y.all = b;
    
    /* x.low * y.low gives us full 64 bits */
    r.all = mul32x32_64(x.s.low, y.s.low);
    
    /* Add cross terms to high word (ignore overflow beyond 64 bits) */
    r.s.high += (su_int)x.s.high * y.s.low;
    r.s.high += x.s.low * (su_int)y.s.high;
    
    *result = (di_int)r.all;
}

/*
 * 64-bit unsigned division
 */

/* Helper: divide 64-bit by 32-bit using only 32-bit ops */
static du_int udiv64by32(du_int n, su_int d)
{
    udwords nn, qq;
    nn.all = n;
    
    if (d == 0) return 0;
    
    /* If dividend fits in 32 bits, use simple 32-bit divide */
    if (nn.s.high == 0) {
        qq.s.high = 0;
        qq.s.low = nn.s.low / d;
        return qq.all;
    }
    
    /* Divide high word first */
    qq.s.high = nn.s.high / d;
    su_int rem = nn.s.high % d;
    
    /* Binary long division for low word */
    su_int q = 0;
    su_int dividend_lo = nn.s.low;
    
    for (int i = 31; i >= 0; i--) {
        su_int rem_high_bit = rem >> 31;
        rem = (rem << 1) | ((dividend_lo >> i) & 1);
        
        if (rem_high_bit || rem >= d) {
            rem -= d;
            q |= (1U << i);
        }
    }
    
    qq.s.low = q;
    return qq.all;
}

static du_int udivdi3_impl(du_int n, du_int d)
{
    if (d == 0) return 0;
    
    su_int n_high = (su_int)(n >> 32);
    su_int d_high = (su_int)(d >> 32);
    su_int d_low = (su_int)d;
    
    if (d_high == 0) {
        return udiv64by32(n, d_low);
    }
    
    du_int quotient = 0;
    du_int remainder = n;
    
    while (remainder >= d) {
        remainder -= d;
        quotient++;
    }
    
    return quotient;
}

void __udivdi3(du_int *result, du_int n, du_int d)
{
    *result = udivdi3_impl(n, d);
}

void __divdi3(di_int *result, di_int a, di_int b)
{
    int neg = 0;
    if (a < 0) { a = -a; neg = !neg; }
    if (b < 0) { b = -b; neg = !neg; }
    du_int q = udivdi3_impl((du_int)a, (du_int)b);
    *result = neg ? -(di_int)q : (di_int)q;
}

void __umoddi3(du_int *result, du_int n, du_int d)
{
    du_int q = udivdi3_impl(n, d);
    *result = n - q * d;
}

void __moddi3(di_int *result, di_int a, di_int b)
{
    int neg = 0;
    if (a < 0) { a = -a; neg = 1; }
    if (b < 0) { b = -b; }
    du_int q = udivdi3_impl((du_int)a, (du_int)b);
    du_int r = (du_int)a - q * (du_int)b;
    *result = neg ? -(di_int)r : (di_int)r;
}

/*
 * 64-bit shifts
 */
void __ashrdi3(di_int *result, di_int a, int b)
{
    dwords x;
    x.all = a;
    
    if (b >= 64) {
        x.s.low = x.s.high >> 31;
        x.s.high = x.s.high >> 31;
    } else if (b >= 32) {
        x.s.low = x.s.high >> (b - 32);
        x.s.high = x.s.high >> 31;
    } else if (b > 0) {
        x.s.low = ((su_int)x.s.low >> b) | ((su_int)x.s.high << (32 - b));
        x.s.high = x.s.high >> b;
    }
    *result = x.all;
}

void __lshrdi3(du_int *result, du_int a, int b)
{
    su_int *words = (su_int *)&a;
    su_int a_low = words[0];
    su_int a_high = words[1];
    
    du_int r;
    su_int *r_words = (su_int *)&r;
    
    if (b >= 64) {
        r_words[0] = 0;
        r_words[1] = 0;
    } else if (b >= 32) {
        r_words[0] = a_high >> (b - 32);
        r_words[1] = 0;
    } else if (b > 0) {
        r_words[0] = (a_low >> b) | (a_high << (32 - b));
        r_words[1] = a_high >> b;
    } else {
        *result = a;
        return;
    }
    *result = r;
}

void __ashldi3(di_int *result, di_int a, int b)
{
    dwords x;
    x.all = a;
    
    if (b >= 64) {
        x.s.low = 0;
        x.s.high = 0;
    } else if (b >= 32) {
        x.s.high = x.s.low << (b - 32);
        x.s.low = 0;
    } else if (b > 0) {
        x.s.high = ((su_int)x.s.high << b) | (x.s.low >> (32 - b));
        x.s.low = x.s.low << b;
    }
    *result = x.all;
}

void __lshldi3(di_int *result, di_int a, int b)
{
    __ashldi3(result, a, b);
}

void __negdi2(di_int *result, di_int a)
{
    *result = -a;
}

/*
 * Count leading zeros
 */
int __clzsi2(su_int a)
{
    if (a == 0) return 32;
    int n = 0;
    if ((a & 0xFFFF0000) == 0) { n += 16; a <<= 16; }
    if ((a & 0xFF000000) == 0) { n += 8; a <<= 8; }
    if ((a & 0xF0000000) == 0) { n += 4; a <<= 4; }
    if ((a & 0xC0000000) == 0) { n += 2; a <<= 2; }
    if ((a & 0x80000000) == 0) { n += 1; }
    return n;
}

int __clzdi2(du_int a)
{
    udwords x;
    x.all = a;
    if (x.s.high)
        return __clzsi2(x.s.high);
    return 32 + __clzsi2(x.s.low);
}

/*
 * Count trailing zeros
 */
int __ctzsi2(su_int a)
{
    if (a == 0) return 32;
    int n = 0;
    if ((a & 0x0000FFFF) == 0) { n += 16; a >>= 16; }
    if ((a & 0x000000FF) == 0) { n += 8; a >>= 8; }
    if ((a & 0x0000000F) == 0) { n += 4; a >>= 4; }
    if ((a & 0x00000003) == 0) { n += 2; a >>= 2; }
    if ((a & 0x00000001) == 0) { n += 1; }
    return n;
}

int __ctzdi2(du_int a)
{
    udwords x;
    x.all = a;
    if (x.s.low)
        return __ctzsi2(x.s.low);
    return 32 + __ctzsi2(x.s.high);
}

/*
 * Population count
 */
int __popcountsi2(su_int a)
{
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    a = (a + (a >> 4)) & 0x0F0F0F0F;
    a = a + (a >> 8);
    a = a + (a >> 16);
    return a & 0x3F;
}

int __popcountdi2(du_int a)
{
    udwords x;
    x.all = a;
    return __popcountsi2(x.s.low) + __popcountsi2(x.s.high);
}

/*
 * Find first set bit
 */
int __ffssi2(si_int a)
{
    if (a == 0) return 0;
    return __ctzsi2((su_int)a) + 1;
}

int __ffsdi2(di_int a)
{
    if (a == 0) return 0;
    return __ctzdi2((du_int)a) + 1;
}

/*
 * Parity
 */
int __paritysi2(su_int a)
{
    return __popcountsi2(a) & 1;
}

int __paritydi2(du_int a)
{
    return __popcountdi2(a) & 1;
}

/*
 * Byte swap
 */
su_int __bswapsi2(su_int a)
{
    return ((a & 0xFF000000) >> 24) |
           ((a & 0x00FF0000) >> 8) |
           ((a & 0x0000FF00) << 8) |
           ((a & 0x000000FF) << 24);
}

void __bswapdi2(du_int *result, du_int a)
{
    udwords x, r;
    x.all = a;
    r.s.high = __bswapsi2(x.s.low);
    r.s.low = __bswapsi2(x.s.high);
    *result = r.all;
}

/*
 * 64-bit comparison
 */
int __cmpdi2(di_int a, di_int b)
{
    if (a < b) return 0;  /* Less than: return 0 */
    if (a > b) return 2;  /* Greater than: return 2 */
    return 1;             /* Equal: return 1 */
}

int __ucmpdi2(du_int a, du_int b)
{
    if (a < b) return 0;
    if (a > b) return 2;
    return 1;
}

/*
 * Overflow checking multiplication
 */
si_int __mulosi4(si_int a, si_int b, int *overflow)
{
    di_int result_full;
    __muldi3(&result_full, (di_int)a, (di_int)b);
    *overflow = (result_full > INT32_MAX || result_full < INT32_MIN);
    return (si_int)result_full;
}

void __mulodi4(di_int *result, di_int a, di_int b, int *overflow)
{
    __muldi3(result, a, b);
    *overflow = 0;
    /* Simplified overflow detection */
    if (a != 0) {
        di_int check;
        __divdi3(&check, *result, a);
        if (check != b) *overflow = 1;
    }
}

/* M65832 Compiler Runtime Support Functions
 *
 * These functions provide software implementations of operations
 * that the compiler may generate calls to when hardware support
 * is not available.
 */

#include <stdint.h>

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
 * Only uses 32-bit operations to avoid recursion
 */
di_int __muldi3(di_int a, di_int b)
{
    dwords x, y;
    udwords r;
    x.all = a;
    y.all = b;
    
    /* (x.high * 2^32 + x.low) * (y.high * 2^32 + y.low)
     * = x.high * y.high * 2^64           <- overflow, ignore
     * + x.high * y.low * 2^32
     * + x.low * y.high * 2^32
     * + x.low * y.low
     */
    
    /* x.low * y.low gives us full 64 bits */
    r.all = mul32x32_64(x.s.low, y.s.low);
    
    /* Add cross terms to high word (ignore overflow beyond 64 bits) */
    r.s.high += (su_int)x.s.high * y.s.low;
    r.s.high += x.s.low * (su_int)y.s.high;
    
    return (di_int)r.all;
}

/*
 * 64-bit unsigned division
 * This implementation only uses 32-bit operations to avoid recursion.
 */

/* Helper: divide 64-bit by 32-bit using only 32-bit ops
 * Uses shift-and-subtract (binary long division)
 */
static du_int udiv64by32(du_int n, su_int d)
{
    if (d == 0) return 0;

    udwords nn, qq;
    const su_int *np = (const su_int *)&n;
    nn.s.low = np[0];
    nn.s.high = np[1];

    if (nn.s.high == 0) {
        qq.s.high = 0;
        qq.s.low = nn.s.low / d;
        return qq.all;
    }

    qq.s.high = nn.s.high / d;
    su_int rem = nn.s.high % d;

    su_int qlo = 0;
    for (int i = 31; i >= 0; --i) {
        rem = (rem << 1) | ((nn.s.low >> i) & 1U);
        if (rem >= d) {
            rem -= d;
            qlo |= (1U << i);
        }
    }

    qq.s.low = qlo;
    return qq.all;
}

static int ucmp64(udwords a, udwords b)
{
    if (a.s.high < b.s.high) return -1;
    if (a.s.high > b.s.high) return 1;
    if (a.s.low < b.s.low) return -1;
    if (a.s.low > b.s.low) return 1;
    return 0;
}

static udwords usub64(udwords a, udwords b)
{
    udwords r;
    su_int borrow = (a.s.low < b.s.low) ? 1 : 0;
    r.s.low = a.s.low - b.s.low;
    r.s.high = a.s.high - b.s.high - borrow;
    return r;
}

static udwords ushl1_64(udwords a)
{
    udwords r;
    r.s.high = (a.s.high << 1) | (a.s.low >> 31);
    r.s.low = a.s.low << 1;
    return r;
}

static du_int make_u64(udwords v)
{
    du_int out;
    su_int *p = (su_int *)&out;
    p[0] = v.s.low;
    p[1] = v.s.high;
    return out;
}

static void udivmod64(du_int n, du_int d, du_int *q_out, du_int *r_out)
{
    udwords nn, dd;
    const su_int *np = (const su_int *)&n;
    const su_int *dp = (const su_int *)&d;
    nn.s.low = np[0];
    nn.s.high = np[1];
    dd.s.low = dp[0];
    dd.s.high = dp[1];

    if (d == 0) {
        if (q_out) *q_out = 0;
        if (r_out) *r_out = 0;
        return;
    }

    /* 32-bit fast path */
    if (nn.s.high == 0 && dd.s.high == 0) {
        su_int q = nn.s.low / dd.s.low;
        su_int r = nn.s.low % dd.s.low;
        if (q_out) {
            udwords qv = { .s = { q, 0 } };
            *q_out = make_u64(qv);
        }
        if (r_out) {
            udwords rv = { .s = { r, 0 } };
            *r_out = make_u64(rv);
        }
        return;
    }

    /* 64/32 path */
    if (dd.s.high == 0) {
        udwords qq;
        su_int rem = 0;

        qq.s.high = nn.s.high / dd.s.low;
        rem = nn.s.high % dd.s.low;

        su_int qlo = 0;
        for (int i = 31; i >= 0; --i) {
            rem = (rem << 1) | ((nn.s.low >> i) & 1U);
            if (rem >= dd.s.low) {
                rem -= dd.s.low;
                qlo |= (1U << i);
            }
        }
        qq.s.low = qlo;

        if (q_out) *q_out = make_u64(qq);
        if (r_out) {
            udwords rv = { .s = { rem, 0 } };
            *r_out = make_u64(rv);
        }
        return;
    }

    /* If high words match, quotient is 0 or 1 */
    if (nn.s.high == dd.s.high) {
        int cmp = ucmp64(nn, dd);
        if (cmp < 0) {
            if (q_out) *q_out = 0;
            if (r_out) *r_out = make_u64(nn);
            return;
        }
        udwords one;
        one.s.low = 1;
        one.s.high = 0;
        if (q_out) *q_out = make_u64(one);
        if (r_out) *r_out = make_u64(usub64(nn, dd));
        return;
    }

    /* Full 64/64 binary long division */
    udwords q;
    udwords r;
    q.s.low = 0;
    q.s.high = 0;
    r.s.low = 0;
    r.s.high = 0;

    for (int i = 0; i < 64; ++i) {
        su_int bit = nn.s.high >> 31;
        nn = ushl1_64(nn);
        r = ushl1_64(r);
        r.s.low |= bit;

        if (ucmp64(r, dd) >= 0) {
            r = usub64(r, dd);
            q = ushl1_64(q);
            q.s.low |= 1;
        } else {
            q = ushl1_64(q);
        }
    }

    if (q_out) *q_out = make_u64(q);
    if (r_out) *r_out = make_u64(r);
}

du_int __udivdi3(du_int n, du_int d)
{
    du_int q = 0;
    udivmod64(n, d, &q, 0);
    return q;
}

/*
 * 64-bit signed division
 */
di_int __divdi3(di_int a, di_int b)
{
    int neg = 0;
    if (a < 0) {
        a = -a;
        neg = !neg;
    }
    if (b < 0) {
        b = -b;
        neg = !neg;
    }
    du_int q = __udivdi3((du_int)a, (du_int)b);
    return neg ? -(di_int)q : (di_int)q;
}

/*
 * 64-bit unsigned modulo
 */
du_int __umoddi3(du_int n, du_int d)
{
    du_int r = 0;
    udivmod64(n, d, 0, &r);
    return r;
}

/*
 * 64-bit signed modulo
 */
di_int __moddi3(di_int a, di_int b)
{
    int neg = 0;
    if (a < 0) {
        a = -a;
        neg = 1;
    }
    if (b < 0) {
        b = -b;
    }
    du_int r = __umoddi3((du_int)a, (du_int)b);
    return neg ? -(di_int)r : (di_int)r;
}

/*
 * 64-bit integer/float conversion helpers
 */
/* Signed 64-bit <-> float/double: see implementations after unsigned versions below */

/*
 * 64-bit arithmetic shift right
 */
di_int __ashrdi3(di_int a, int b)
{
    dwords x;
    x.all = a;
    
    if (b >= 64) {
        x.s.low = x.s.high >> 31; /* All sign bits */
        x.s.high = x.s.high >> 31;
    } else if (b >= 32) {
        x.s.low = x.s.high >> (b - 32);
        x.s.high = x.s.high >> 31;
    } else if (b > 0) {
        x.s.low = ((su_int)x.s.low >> b) | ((su_int)x.s.high << (32 - b));
        x.s.high = x.s.high >> b;
    }
    return x.all;
}

/*
 * 64-bit logical shift right
 * Use pointer casts to access words, avoiding potential struct access bugs
 */
du_int __lshrdi3(du_int a, int b)
{
    /* Access via pointers to avoid struct member access issues */
    su_int *words = (su_int *)&a;
    su_int a_low = words[0];   /* Little-endian: low word first */
    su_int a_high = words[1];  /* High word second */
    
    du_int result;
    su_int *r_words = (su_int *)&result;
    
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
        return a;
    }
    return result;
}

/*
 * 64-bit shift left
 */
di_int __ashldi3(di_int a, int b)
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
    return x.all;
}

/* Alias */
di_int __lshldi3(di_int a, int b)
{
    return __ashldi3(a, b);
}

/*
 * 64-bit negation
 */
di_int __negdi2(di_int a)
{
    return -a;
}

/*
 * Count leading zeros (32-bit)
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

/*
 * Count leading zeros (64-bit)
 */
int __clzdi2(du_int a)
{
    udwords x;
    x.all = a;
    if (x.s.high)
        return __clzsi2(x.s.high);
    return 32 + __clzsi2(x.s.low);
}

/*
 * Count trailing zeros (32-bit)
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

/*
 * Count trailing zeros (64-bit)
 */
int __ctzdi2(du_int a)
{
    udwords x;
    x.all = a;
    if (x.s.low)
        return __ctzsi2(x.s.low);
    return 32 + __ctzsi2(x.s.high);
}

/*
 * Population count (32-bit)
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

/*
 * Population count (64-bit)
 */
int __popcountdi2(du_int a)
{
    udwords x;
    x.all = a;
    return __popcountsi2(x.s.low) + __popcountsi2(x.s.high);
}

/*
 * Find first set bit (32-bit)
 */
int __ffssi2(si_int a)
{
    if (a == 0) return 0;
    return __ctzsi2((su_int)a) + 1;
}

/*
 * Find first set bit (64-bit)
 */
int __ffsdi2(di_int a)
{
    if (a == 0) return 0;
    return __ctzdi2((du_int)a) + 1;
}

/*
 * Parity (32-bit)
 */
int __paritysi2(su_int a)
{
    return __popcountsi2(a) & 1;
}

/*
 * Parity (64-bit)
 */
int __paritydi2(du_int a)
{
    return __popcountdi2(a) & 1;
}

/*
 * Byte swap (32-bit)
 */
su_int __bswapsi2(su_int a)
{
    return ((a & 0xFF000000) >> 24) |
           ((a & 0x00FF0000) >> 8) |
           ((a & 0x0000FF00) << 8) |
           ((a & 0x000000FF) << 24);
}

/*
 * Byte swap (64-bit)
 */
du_int __bswapdi2(du_int a)
{
    udwords x, r;
    x.all = a;
    r.s.high = __bswapsi2(x.s.low);
    r.s.low = __bswapsi2(x.s.high);
    return r.all;
}

/*
 * 64-bit comparison (for qsort, etc.)
 */
int __cmpdi2(di_int a, di_int b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int __ucmpdi2(du_int a, du_int b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

/*
 * Overflow checking multiplication
 */
si_int __mulosi4(si_int a, si_int b, int *overflow)
{
    di_int result = (di_int)a * b;
    *overflow = (result > INT32_MAX || result < INT32_MIN);
    return (si_int)result;
}

di_int __mulodi4(di_int a, di_int b, int *overflow)
{
    /* Simplified - doesn't detect all overflows */
    di_int result = __muldi3(a, b);
    *overflow = 0;
    if (a != 0 && result / a != b)
        *overflow = 1;
    return result;
}

/* ============================================================================
 * Unsigned 64-bit <-> float/double conversions
 * These avoid using 64-bit int operations that would recurse.
 * ========================================================================= */

float __floatundisf(du_int a)
{
    if (a == 0) return 0.0f;
    /* Split into high and low 32-bit parts */
    su_int hi = (su_int)(a >> 32);
    su_int lo = (su_int)a;
    if (hi == 0) return (float)lo;
    /* hi * 2^32 + lo */
    float result = (float)hi;
    result *= 4294967296.0f;  /* 2^32 */
    result += (float)lo;
    return result;
}

double __floatundidf(du_int a)
{
    if (a == 0) return 0.0;
    su_int hi = (su_int)(a >> 32);
    su_int lo = (su_int)a;
    if (hi == 0) return (double)lo;
    double result = (double)hi;
    result *= 4294967296.0;  /* 2^32 */
    result += (double)lo;
    return result;
}

du_int __fixunssfdi(float a)
{
    if (a <= 0.0f || a != a) return 0;  /* negative, zero, or NaN */
    if (a >= 18446744073709551616.0f) return ~(du_int)0;  /* overflow */
    /* Split: hi = a / 2^32, lo = remainder */
    float hi_f = a * (1.0f / 4294967296.0f);
    su_int hi = (su_int)hi_f;
    float lo_f = a - (float)hi * 4294967296.0f;
    su_int lo = (lo_f >= 0.0f) ? (su_int)lo_f : 0;
    return ((du_int)hi << 32) | lo;
}

du_int __fixunsdfdi(double a)
{
    if (a <= 0.0 || a != a) return 0;
    if (a >= 18446744073709551616.0) return ~(du_int)0;
    double hi_f = a * (1.0 / 4294967296.0);
    su_int hi = (su_int)hi_f;
    double lo_f = a - (double)hi * 4294967296.0;
    su_int lo = (lo_f >= 0.0) ? (su_int)lo_f : 0;
    return ((du_int)hi << 32) | lo;
}

/* Signed 64-bit <-> float/double conversions */

float __floatdisf(di_int a)
{
    if (a < 0) return -__floatundisf((du_int)(-a));
    return __floatundisf((du_int)a);
}

double __floatdidf(di_int a)
{
    if (a < 0) return -__floatundidf((du_int)(-a));
    return __floatundidf((du_int)a);
}

di_int __fixsfdi(float a)
{
    if (a < 0.0f) return -(di_int)__fixunssfdi(-a);
    return (di_int)__fixunssfdi(a);
}

di_int __fixdfdi(double a)
{
    if (a < 0.0) return -(di_int)__fixunsdfdi(-a);
    return (di_int)__fixunsdfdi(a);
}

/* ============================================================================
 * Complex number arithmetic
 * ========================================================================= */

float _Complex __mulsc3(float a, float b, float c, float d)
{
    /* (a+bi) * (c+di) = (ac-bd) + (ad+bc)i */
    float ac = a * c;
    float bd = b * d;
    float ad = a * d;
    float bc = b * c;
    float _Complex z;
    __real__ z = ac - bd;
    __imag__ z = ad + bc;
    return z;
}

double _Complex __muldc3(double a, double b, double c, double d)
{
    double ac = a * c;
    double bd = b * d;
    double ad = a * d;
    double bc = b * c;
    double _Complex z;
    __real__ z = ac - bd;
    __imag__ z = ad + bc;
    return z;
}

float _Complex __divsc3(float a, float b, float c, float d)
{
    /* (a+bi) / (c+di) = ((ac+bd) + (bc-ad)i) / (c*c+d*d) */
    float denom = c * c + d * d;
    float _Complex z;
    __real__ z = (a * c + b * d) / denom;
    __imag__ z = (b * c - a * d) / denom;
    return z;
}

double _Complex __divdc3(double a, double b, double c, double d)
{
    double denom = c * c + d * d;
    double _Complex z;
    __real__ z = (a * c + b * d) / denom;
    __imag__ z = (b * c - a * d) / denom;
    return z;
}

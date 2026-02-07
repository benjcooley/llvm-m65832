#include <strings.h>

int ffs(int i) {
    if (i == 0) return 0;
    int bit = 1;
    while (!(i & 1)) {
        i >>= 1;
        bit++;
    }
    return bit;
}

int ffsl(long i) {
    return ffs((int)i);
}

int ffsll(long long i) {
    if (i == 0) return 0;
    int bit = 1;
    while (!(i & 1)) {
        i >>= 1;
        bit++;
    }
    return bit;
}

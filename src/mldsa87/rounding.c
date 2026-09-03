#include "rounding.h"
#include "constant.h"

int32_t power2_round(int32_t *a0, int32_t a) {
    int32_t a1 = 0;

    a1 = (a + (1 << (D - 1)) - 1) >> D;
    *a0 = a - (a1 << D);
    return a1;
}

int32_t decompose(int32_t *a0, int32_t a) {
    int32_t a1 = (a + 127) >> 7;
    a1 = (a1 * 1025 + (1 << 21)) >> 22;
    a1 &= 15;

    *a0 = a - a1 * 2 * GAMMA2;
    *a0 -= (((Q_CONST - 1) / 2 - *a0) >> 31) & Q_CONST;

    return a1;
}

uint32_t make_hint(int32_t a0, int32_t a1) {
    if (a0 > GAMMA2 || a0 < -GAMMA2 || (a0 == -GAMMA2 && a1 != 0)) {
        return 1;
    }

    return 0;
}

int32_t use_hint(int32_t a, int32_t hint) {
    int32_t a0 = 0, a1 = 0;

    a1 = decompose(&a0, a);
    if (hint == 0) {
        return a1;
    }

    if (a0 > 0) {
        return (a1 + 1) & 15;
    } else {
        return (a1 - 1) & 15;
    }
}
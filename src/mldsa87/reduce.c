#include "reduce.h"
#include "constant.h"

int32_t montgomery_reduce(int64_t a) {
    int32_t t = 0;
    t = (int32_t) ((int64_t) (int32_t) a * Q_INV);
    t = (int32_t) ((a - (int64_t) t * Q_CONST) >> 32);
    return t;
}
int32_t reduce_32(int32_t a) {
    int32_t t = 0;

    t = (a + (1 << 22)) >> 23;
    t = a - t * Q_CONST;

    return t;
}
int32_t c_addq(int32_t a) {
    a += (a >> 31) & Q_CONST;
    return a;
}
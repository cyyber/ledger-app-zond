#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>

int32_t montgomery_reduce(int64_t a);
int32_t reduce_32(int32_t a);
int32_t c_addq(int32_t a);

#endif
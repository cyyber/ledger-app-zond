#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "constant.h"

void ntt(int32_t (*a)[N]);
void inv_ntt_to_mont(int32_t (*a)[N]);

#endif // !NTT_H

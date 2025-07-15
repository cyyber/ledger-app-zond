#include "ntt.h"
#include "reduce.h"
#include "cx.h"

void ntt(int32_t (*a)[N]) {
    uint32_t count = 0, start = 0, j = 0, k = 0;
	int32_t zeta = 0, t = 0;

	k = 0;
	for(count = 128; count > 0; count >>= 1) {
		for(start = 0; start < N; start = j + count) {
			k++;
			zeta = ZETAS[k];
			for(j = start; j < start+count; j++) {
				t = montgomery_reduce((int64_t)zeta * (int64_t)(*a)[j+count]);
				(*a)[j+count] = (*a)[j] - t;
				(*a)[j] = (*a)[j] + t;
			}
		}
	}
}

void inv_ntt_to_mont(int32_t (*a)[N]) {
    uint32_t count = 0, start = 0, j = 0, k = 0;
	int32_t zeta = 0, t = 0;
	int32_t f = (int32_t)41978;

	k = 256;
	
	for(count = 1; count < N; count <<= 1) {
		for(start = 0; start < N; start = (j + count)) {
			k--;
			zeta = -ZETAS[k];
			for(j = start; j < (start+count); j++) {
				t = (*a)[j];
				(*a)[j] = t + (*a)[j+count];
				(*a)[j+count] = t - (*a)[j+count];
				(*a)[j+count] = montgomery_reduce((int64_t)zeta * (int64_t)(*a)[j+count]);
			}
		}
	}

	for(j = 0; j < N; j++) {
		(*a)[j] = montgomery_reduce((int64_t)f * (int64_t)(*a)[j]);
	}
}
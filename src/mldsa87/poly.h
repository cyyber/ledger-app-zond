#ifndef POLY_H
#define POLY_H

#include "constant.h"
#include <stdint.h>
#include <stdlib.h>
#include "error.h"

typedef struct {
    int32_t coeffs[N];
} Poly;

void poly_c_addq(Poly *a);
void poly_reduce(Poly *a);
void poly_add(Poly *c, Poly *a, Poly *b);
void poly_sub(Poly *c, Poly *a, Poly *b);
void poly_shitf_l(Poly *a);
void poly_ntt(Poly *a);
void poly_inv_ntt_to_mont(Poly *a);
void poly_pointwise_montgomery(Poly *c, Poly *a, Poly *b);
void poly_power2_round(Poly *a1, Poly *a0, Poly *a);
void poly_decompose(Poly *a1, Poly *a0, Poly *a);
uint32_t poly_make_hint(Poly *h, Poly *a0, Poly *a1);
void poly_use_hint(Poly *b, Poly *a, Poly *h);
int32_t poly_chk_norm(Poly *a, int32_t B);
ErrorCode poly_uniform(Poly *a, uint8_t (*seed)[SEED_BYTES], uint16_t nonce);
uint32_t rej_uniform(int32_t a[], size_t a_len, uint8_t buf[], size_t buf_len);
uint32_t rej_eta(int32_t a[], size_t a_len, uint8_t buf[], size_t buf_len);
ErrorCode poly_uniform_eta(Poly *a, uint8_t (*seed)[CRH_BYTES], uint16_t nonce);
void poly_uniform_gamma1(Poly *a, uint8_t seed[CRH_BYTES], uint16_t nonce);
ErrorCode poly_challenge(Poly *c, uint8_t seed[], size_t seed_len);
void poly_eta_pack(uint8_t r[], Poly *a);
void poly_eta_unpack(Poly *r, uint8_t a[]);
void poly_t1_pack(uint8_t r[], Poly *a);
void poly_t1_unpack(Poly *r, uint8_t a[]);
void poly_t0_pack(uint8_t r[], Poly *a);
void poly_t0_unpack(Poly *r, uint8_t a[]);
void poly_z_pack(uint8_t r[], Poly *a);
void poly_z_unpack(Poly *r, uint8_t a[]);
void poly_w1_pack(uint8_t r[], Poly *a);

#endif
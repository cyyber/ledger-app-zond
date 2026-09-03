#ifndef POLYVEC_H
#define POLYVEC_H

#include "poly.h"

typedef struct {
    Poly vec[K];
} PolyVecK;

typedef struct {
    Poly vec[L];
} PolyVecL;

void poly_vec_l_uniform_gamma1(PolyVecL *v, uint8_t seed[CRH_BYTES], uint16_t nonce);
void poly_vec_l_reduce(PolyVecL *v);
void poly_vec_l_add(PolyVecL *w, PolyVecL *u, PolyVecL *v);
void poly_vec_l_ntt(PolyVecL *v);
void poly_vec_l_inv_ntt_to_mont(PolyVecL *v);
void poly_vec_l_pointwise_poly_montgomery(PolyVecL *r, Poly *a, PolyVecL *v);
ErrorCode poly_vec_matrix_expand(PolyVecL (*mat)[K], uint8_t (*rho)[SEED_BYTES]);
int32_t poly_vec_l_chk_norm(PolyVecL *v, int32_t bound);
void poly_vec_k_add(PolyVecK *w, PolyVecK *u, PolyVecK *v);
void poly_vec_k_sub(PolyVecK *w, PolyVecK *u, PolyVecK *v);
void poly_vec_k_shift_l(PolyVecK *v);
void poly_vec_k_ntt(PolyVecK *v);
void poly_vec_k_inv_ntt_to_mont(PolyVecK *v);
void poly_vec_k_pointwise_poly_montgomery(PolyVecK *r, Poly *a, PolyVecK *v);
int32_t poly_vec_k_chk_norm(PolyVecK *v, int32_t bound);
void poly_vec_k_power2_round(PolyVecK *v1, PolyVecK *v0, PolyVecK *v);
void poly_vec_k_decompose(PolyVecK *v1, PolyVecK *v0, PolyVecK *v);
uint32_t poly_vec_k_make_hint(PolyVecK *h, PolyVecK *v0, PolyVecK *v1);
void poly_vec_k_use_hint(PolyVecK *w, PolyVecK *u, PolyVecK *h);
void poly_vec_l_pointwise_acc_montgomery(Poly *w, PolyVecL *u, PolyVecL *v);
void poly_vec_matrix_pointwise_montgomery(PolyVecK *t, PolyVecL (*mat)[K], PolyVecL *v);
ErrorCode poly_vec_l_uniform_eta(PolyVecL *v, uint8_t (*seed)[CRH_BYTES], uint16_t nonce);
ErrorCode poly_vec_k_uniform_eta(PolyVecK *v, uint8_t (*seed)[CRH_BYTES], uint16_t nonce);
void poly_vec_k_reduce(PolyVecK *v);
void poly_vec_k_c_addq(PolyVecK *v);
ErrorCode poly_vec_k_pack_w1(uint8_t r[], size_t r_len, PolyVecK *w1);

#endif
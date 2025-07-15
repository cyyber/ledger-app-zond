#include "packing.h"
#include <string.h>
#include <stdint.h>
#include "utils.h"

void pack_pk(uint8_t (*pkb)[CRYPTO_PUBLIC_KEY_BYTES], uint8_t rho[SEED_BYTES], PolyVecK *t1) {
	copy_array(pkb, CRYPTO_PUBLIC_KEY_BYTES, rho, SEED_BYTES);
	for(int i = 0; i < K; i++) {
		poly_t1_pack(pkb + SEED_BYTES + i*POLY_T1_PACKED_BYTES, &t1->vec[i]);
	}
}

void unpack_pk(uint8_t (*rho)[SEED_BYTES],
	PolyVecK *t1,
	uint8_t (*pkb)[CRYPTO_PUBLIC_KEY_BYTES]) {
	copy_array((uint8_t *) rho, SEED_BYTES, pkb, CRYPTO_PUBLIC_KEY_BYTES);
	for(int i = 0; i < K; i++) {
		poly_t1_unpack(&t1->vec[i], pkb + SEED_BYTES + i*POLY_T1_PACKED_BYTES);
	}
}

void pack_sk(uint8_t rho[SEED_BYTES], uint8_t tr[TR_BYTES], uint8_t key[SEED_BYTES],
	PolyVecK *t0,
	PolyVecL *s1,
	PolyVecK *s2) {

    uint8_t sk[CRYPTO_SECRET_KEY_BYTES] = {0};

	// copy_array(sk, CRYPTO_SECRET_KEY_BYTES, rho, SEED_BYTES);

    // sk = sk + SEED_BYTES;
	// copy_array(sk, CRYPTO_SECRET_KEY_BYTES - SEED_BYTES, key, SEED_BYTES);
    // sk = sk + SEED_BYTES;
	// copy_array(sk, CRYPTO_SECRET_KEY_BYTES - 2 * SEED_BYTES, tr, TR_BYTES);

	// sk = sk + TR_BYTES;
	uint8_t *sk_ptr = sk;
	for(int i = 0; i < L; i++) {
        sk_ptr = sk_ptr + i*POLY_ETA_PACKED_BYTES;
		poly_eta_pack(sk_ptr, &s1->vec[i]);
	}

	// sk = sk + L*POLY_ETA_PACKED_BYTES;

	// for(int i = 0; i < K; i++) {
    //     sk = sk + i*POLY_ETA_PACKED_BYTES;
	// 	poly_eta_pack(sk, &s2->vec[i]);
	// }
	// sk = sk + K*POLY_ETA_PACKED_BYTES;

	for(int i = 0; i < K; i++) {
        sk_ptr = sk_ptr + i*POLY_T0_PACKED_BYTES;
		poly_t0_pack(sk_ptr, &t0->vec[i]);
	}   
	// return sk;     
}

void unpack_sk(uint8_t (*rho)[SEED_BYTES],
	uint8_t (*tr)[TR_BYTES],
	uint8_t (*key)[SEED_BYTES],
	PolyVecK *t0,
	PolyVecL *s1,
	PolyVecK *s2,
	uint8_t (*skb)[CRYPTO_SECRET_KEY_BYTES]) {
	copy_array((uint8_t *)rho, SEED_BYTES, skb, CRYPTO_SECRET_KEY_BYTES);
    skb = skb + SEED_BYTES;
	copy_array((uint8_t *)key, SEED_BYTES, skb, CRYPTO_SECRET_KEY_BYTES - SEED_BYTES);
    skb = skb + SEED_BYTES;
	copy_array((uint8_t *)tr, TR_BYTES, skb, CRYPTO_SECRET_KEY_BYTES - 2 * SEED_BYTES);
	skb = skb + TR_BYTES;

	for(int i = 0; i < L; i++) {
        skb = skb + i*POLY_ETA_PACKED_BYTES;
		poly_eta_unpack(&s1->vec[i], skb);
	}
	skb = skb + L*POLY_ETA_PACKED_BYTES;

	for(int i = 0; i < K; i++) {
        skb = skb + i*POLY_ETA_PACKED_BYTES;
		poly_eta_unpack(&s2->vec[i], skb);
	}
	skb = skb + K*POLY_ETA_PACKED_BYTES;

	for(int i = 0; i < K; i++) {
        skb = skb + i*POLY_T0_PACKED_BYTES;
		poly_t0_unpack(&t0->vec[i], skb);
	}
}

ErrorCode pack_sig(uint8_t sig[], size_t sigb_len, uint8_t c[CTILDE_BYTES], PolyVecL *z, PolyVecK *h) {
    if(sigb_len != CRYPTO_BYTES) {
		return ERR_INVALID_SIGB_LENGTH;
	}
	copy_array(sig, CTILDE_BYTES, c, CTILDE_BYTES);
	sig = sig + CTILDE_BYTES;

	for(int i = 0; i < L; i++) {
        sig = sig + i*POLY_Z_PACKED_BYTES;
		poly_z_pack(sig, &z->vec[i]);
	}
	sig = sig + L*POLY_Z_PACKED_BYTES;

	/* Encode h */
	for(int i = 0; i < OMEGA+K; i++) {
		sig[i] = 0;
	}

	int32_t k = 0;
	for(int i = 0; i < K; i++) {
		for(int j = 0; j < N; j++) {
			if(h->vec[i].coeffs[j] != 0) {
				sig[k] = (uint8_t)j;
				k++;
			}
			sig[OMEGA+i] = (uint8_t)k;
		}
	}
	return ERR_NONE;
}

int32_t unpack_sig(uint8_t (*c)[CTILDE_BYTES],
	PolyVecL *z,
	PolyVecK *h,
	uint8_t sig[CRYPTO_BYTES]) {
	copy_array((uint8_t *)c, CTILDE_BYTES, sig, CTILDE_BYTES);
	sig = sig + CTILDE_BYTES;

	for(int i = 0; i < L; i++) {
        sig = sig + i*POLY_Z_PACKED_BYTES;
		poly_z_unpack(&z->vec[i], sig);
	}
	sig = sig + L*POLY_Z_PACKED_BYTES;

	/* Decode h */
	uint32_t k = (uint32_t)0;
	for(int i = 0; i < K; i++) {
		for(int j = 0; j < N; j++) {
			h->vec[i].coeffs[j] = 0;
		}
		if((uint32_t)(sig[OMEGA+i]) < k || sig[OMEGA+i] > OMEGA) {
			return 1;
		}
		for(uint32_t j = k; j < (uint32_t)(sig[OMEGA+i]); j++) {
			/* Coefficients are ordered for strong unforgeability */
			if(j > k && sig[j] <= sig[j-1]) {
				return 1;
			}
			h->vec[i].coeffs[sig[j]] = 1;
		}
		k = (uint32_t)(sig[OMEGA+i]);
	}

	for(int j = k; j < OMEGA; j++) {
		if(sig[j] != 0) {
			return 1;
		}
	}

	return 0;
}